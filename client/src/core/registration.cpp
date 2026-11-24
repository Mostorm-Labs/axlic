#include "axlic/core/registration.hpp"

#include "axlic/core/mutation_lock_guard.hpp"

#include <utility>

namespace axlic::core {
namespace {

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

RegistrationResult mutate_registration(
    ILicenseStateStore& state,
    IMutationLock& lock,
    const std::optional<DeviceReference>& replacement) {
  RegistrationResult result{RegistrationCode::internal_error, false};
  const MutationLockGuard guard(lock, result.retryable);
  if (!guard.acquired()) {
    if (result.retryable) {
      return {RegistrationCode::local_state_busy, true};
    }
    if (guard.denied()) {
      return {RegistrationCode::local_write_privilege_required, false};
    }
    return {RegistrationCode::internal_error, false};
  }
  // Single read both validates and loads the state; an absent file is an empty
  // state, and any read failure must abort instead of committing a default
  // LicenseState that would drop the installed credential.
  LicenseState next;
  const auto read = state.read();
  if (read.outcome == StateStoreOutcome::ok && read.state) {
    next = *read.state;
  } else if (read.outcome != StateStoreOutcome::ok && read.outcome != StateStoreOutcome::absent) {
    return {state_code(read.outcome), false};
  }
  const auto prepared = state.prepare_for_mutation();
  if (prepared != StateStoreOutcome::ok) {
    return {state_code(prepared), false};
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
