#include "axlic/core/registration.hpp"

#include <utility>

namespace axlic::core {
namespace {

class LockGuard {
 public:
  LockGuard(IMutationLock& lock, bool& retryable) : lock_(lock), retryable_(retryable) {
    const auto outcome = lock.acquire();
    acquired_ = outcome == LockOutcome::acquired;
    retryable = outcome == LockOutcome::busy;
  }
  ~LockGuard() {
    if (acquired_) {
      lock_.release();
    }
  }
  LockGuard(const LockGuard&) = delete;
  LockGuard& operator=(const LockGuard&) = delete;
  [[nodiscard]] bool acquired() const noexcept { return acquired_; }

 private:
  IMutationLock& lock_;
  bool& retryable_;
  bool acquired_{};
};

RegistrationCode state_code(const StateStoreOutcome outcome) {
  switch (outcome) {
    case StateStoreOutcome::ok:
      return RegistrationCode::ok;
    case StateStoreOutcome::absent:
      return RegistrationCode::ok;
    case StateStoreOutcome::corrupt:
      return RegistrationCode::local_state_corrupt;
    case StateStoreOutcome::unsupported:
      return RegistrationCode::local_state_unsupported;
    case StateStoreOutcome::privilege_required:
      return RegistrationCode::local_write_privilege_required;
    case StateStoreOutcome::unexpected_failure:
      break;
  }
  return RegistrationCode::internal_error;
}

// Reads current state under the already-held lock; an absent file is an empty
// state, not an error.
RegistrationResult read_state(ILicenseStateStore& state) {
  const auto read = state.read();
  if (read.outcome == StateStoreOutcome::absent) {
    return {RegistrationCode::ok, false};
  }
  if (read.outcome != StateStoreOutcome::ok) {
    return {state_code(read.outcome), false};
  }
  return {RegistrationCode::ok, false};
}

RegistrationResult mutate_registration(
    ILicenseStateStore& state,
    IMutationLock& lock,
    const std::optional<DeviceReference>& replacement) {
  RegistrationResult result{RegistrationCode::internal_error, false};
  const LockGuard guard(lock, result.retryable);
  if (!guard.acquired()) {
    return result.retryable ? RegistrationResult{RegistrationCode::local_state_busy, true}
                            : RegistrationResult{RegistrationCode::internal_error, false};
  }
  const auto current = read_state(state);
  if (current.code != RegistrationCode::ok) {
    return {current.code, false};
  }
  const auto prepared = state.prepare_for_mutation();
  if (prepared != StateStoreOutcome::ok) {
    return {state_code(prepared), false};
  }
  LicenseState next;
  if (const auto read = state.read(); read.outcome == StateStoreOutcome::ok && read.state) {
    next = *read.state;
  }
  if (replacement) {
    next.registration = RegistrationAssociation{.device_id = *replacement};
  } else {
    next.registration.reset();
  }
  const auto written = state.write(next);
  if (written != StateStoreOutcome::ok) {
    return {state_code(written), false};
  }
  return {RegistrationCode::ok, false};
}

}  // namespace

SelfReportValue resolve_self_report(
    const std::optional<RegistrationAssociation>& registration,
    const std::string& identity_value) {
  if (registration) {
    return {true, registration->device_id.to_string()};
  }
  return {false, identity_value};
}

RegistrationResult adopt_registration(
    ILicenseStateStore& state,
    IMutationLock& lock,
    const DeviceReference& device_id) {
  return mutate_registration(state, lock, device_id);
}

RegistrationResult reset_registration(ILicenseStateStore& state, IMutationLock& lock) {
  return mutate_registration(state, lock, std::nullopt);
}

std::string_view registration_code_name(const RegistrationCode code) noexcept {
  switch (code) {
    case RegistrationCode::ok:
      return "OK";
    case RegistrationCode::local_state_busy:
      return "LOCAL_STATE_BUSY";
    case RegistrationCode::local_state_corrupt:
      return "LOCAL_STATE_CORRUPT";
    case RegistrationCode::local_state_unsupported:
      return "LOCAL_STATE_UNSUPPORTED";
    case RegistrationCode::local_write_privilege_required:
      return "LOCAL_WRITE_PRIVILEGE_REQUIRED";
    case RegistrationCode::internal_error:
      break;
  }
  return "INTERNAL_ERROR";
}

}  // namespace axlic::core
