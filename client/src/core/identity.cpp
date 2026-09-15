#include "axlic/core/identity.hpp"

#include <utility>

namespace axlic::core {
namespace {

IdentityResult state_failure(const StateStoreOutcome outcome) {
  switch (outcome) {
    case StateStoreOutcome::corrupt:
      return {IdentityCode::local_state_corrupt, false, std::nullopt};
    case StateStoreOutcome::unsupported:
      return {IdentityCode::local_state_unsupported, false, std::nullopt};
    case StateStoreOutcome::privilege_required:
      return {IdentityCode::local_write_privilege_required, false, std::nullopt};
    case StateStoreOutcome::ok:
    case StateStoreOutcome::absent:
    case StateStoreOutcome::unexpected_failure:
      return {IdentityCode::internal_error, false, std::nullopt};
  }
  return {IdentityCode::internal_error, false, std::nullopt};
}

class LockGuard {
 public:
  explicit LockGuard(IMutationLock& lock) : lock_(lock) {}
  ~LockGuard() { lock_.release(); }
  LockGuard(const LockGuard&) = delete;
  LockGuard& operator=(const LockGuard&) = delete;

 private:
  IMutationLock& lock_;
};

bool matches_committed(const IdentityRecord& actual, const IdentityRecord& committed) {
  return actual == committed;
}

}  // namespace

IdentityManager::IdentityManager(
    IIdentityProvider& tpm,
    IIdentityProvider& software,
    IMachineStateStore& state,
    IMutationLock& lock,
    const IdentityPolicy policy,
    KeyNameGenerator key_name_generator)
    : tpm_(tpm),
      software_(software),
      state_(state),
      lock_(lock),
      policy_(policy),
      key_name_generator_(std::move(key_name_generator)) {}

IdentityResult IdentityManager::load_committed(const IdentityRecord& committed) {
  IIdentityProvider& provider = committed.provider_kind == ProviderKind::tpm ? tpm_ : software_;
  const auto loaded = provider.load(committed.key_name);
  switch (loaded.outcome) {
    case ProviderOutcome::usable:
      if (loaded.identity && matches_committed(*loaded.identity, committed)) {
        return {IdentityCode::ok, false, loaded.identity};
      }
      return {IdentityCode::identity_recovery_required, false, std::nullopt};
    case ProviderOutcome::temporarily_unavailable:
      return {IdentityCode::identity_provider_unavailable, true, std::nullopt};
    case ProviderOutcome::key_not_found_or_corrupt:
      return {IdentityCode::identity_recovery_required, false, std::nullopt};
    case ProviderOutcome::absent_or_permanently_unusable:
    case ProviderOutcome::access_denied:
    case ProviderOutcome::unexpected_failure:
      return {IdentityCode::identity_provider_unavailable, false, std::nullopt};
  }
  return {IdentityCode::internal_error, false, std::nullopt};
}

IdentityResult IdentityManager::establish_with(IIdentityProvider& provider) {
  const auto key_name = key_name_generator_();
  const auto established = provider.establish(key_name);
  if (established.outcome != ProviderOutcome::usable || !established.identity) {
    if (established.outcome == ProviderOutcome::temporarily_unavailable) {
      return {IdentityCode::identity_provider_unavailable, true, std::nullopt};
    }
    if (established.outcome == ProviderOutcome::absent_or_permanently_unusable) {
      return {IdentityCode::identity_provider_unsupported, false, std::nullopt};
    }
    return {IdentityCode::identity_provider_unavailable, false, std::nullopt};
  }
  const auto& identity = *established.identity;
  if (identity.key_name != key_name || identity.provider_kind != provider.provider_kind() ||
      identity.provider_name != provider.provider_name() || identity.scheme_id != provider.scheme_id() ||
      identity.identity_epoch != 1 || identity.identity_value.size() != 128U) {
    provider.remove(key_name);
    return {IdentityCode::internal_error, false, std::nullopt};
  }
  const auto written = state_.write(identity);
  if (written != StateStoreOutcome::ok) {
    provider.remove(key_name);
    return state_failure(written);
  }
  return {IdentityCode::ok, false, identity};
}

IdentityResult IdentityManager::ensure_identity() {
  const auto initial = state_.read();
  if (initial.outcome == StateStoreOutcome::ok) {
    return initial.identity ? load_committed(*initial.identity)
                            : IdentityResult{IdentityCode::local_state_corrupt, false, std::nullopt};
  }
  if (initial.outcome != StateStoreOutcome::absent) {
    return state_failure(initial.outcome);
  }

  const auto lock_outcome = lock_.acquire();
  if (lock_outcome == LockOutcome::busy) {
    return {IdentityCode::local_state_busy, true, std::nullopt};
  }
  if (lock_outcome == LockOutcome::access_denied) {
    return {IdentityCode::local_write_privilege_required, false, std::nullopt};
  }
  if (lock_outcome != LockOutcome::acquired) {
    return {IdentityCode::internal_error, false, std::nullopt};
  }
  LockGuard guard(lock_);

  const auto current = state_.read();
  if (current.outcome == StateStoreOutcome::ok) {
    return current.identity ? load_committed(*current.identity)
                            : IdentityResult{IdentityCode::local_state_corrupt, false, std::nullopt};
  }
  if (current.outcome != StateStoreOutcome::absent) {
    return state_failure(current.outcome);
  }
  const auto prepared = state_.prepare_for_mutation();
  if (prepared != StateStoreOutcome::ok) {
    return state_failure(prepared);
  }

  const auto tpm_outcome = tpm_.probe();
  if (tpm_outcome == ProviderOutcome::usable) {
    const auto tpm_result = establish_with(tpm_);
    if (tpm_result.code == IdentityCode::ok) {
      return tpm_result;
    }
    if (tpm_result.code != IdentityCode::identity_provider_unsupported) {
      return tpm_result;
    }
  }
  if (tpm_outcome == ProviderOutcome::temporarily_unavailable) {
    return {IdentityCode::identity_provider_unavailable, true, std::nullopt};
  }
  if (tpm_outcome != ProviderOutcome::usable &&
      tpm_outcome != ProviderOutcome::absent_or_permanently_unusable) {
    return {IdentityCode::identity_provider_unavailable, false, std::nullopt};
  }
  if (!policy_.allow_software_fallback) {
    return {IdentityCode::identity_provider_unsupported, false, std::nullopt};
  }
  const auto software_outcome = software_.probe();
  if (software_outcome == ProviderOutcome::usable) {
    return establish_with(software_);
  }
  if (software_outcome == ProviderOutcome::temporarily_unavailable) {
    return {IdentityCode::identity_provider_unavailable, true, std::nullopt};
  }
  return {IdentityCode::identity_provider_unsupported, false, std::nullopt};
}

std::string_view identity_code_name(const IdentityCode code) noexcept {
  switch (code) {
    case IdentityCode::ok:
      return "OK";
    case IdentityCode::local_write_privilege_required:
      return "LOCAL_WRITE_PRIVILEGE_REQUIRED";
    case IdentityCode::local_state_busy:
      return "LOCAL_STATE_BUSY";
    case IdentityCode::local_state_corrupt:
      return "LOCAL_STATE_CORRUPT";
    case IdentityCode::local_state_unsupported:
      return "LOCAL_STATE_UNSUPPORTED";
    case IdentityCode::identity_provider_unavailable:
      return "IDENTITY_PROVIDER_UNAVAILABLE";
    case IdentityCode::identity_provider_unsupported:
      return "IDENTITY_PROVIDER_UNSUPPORTED";
    case IdentityCode::identity_recovery_required:
      return "IDENTITY_RECOVERY_REQUIRED";
    case IdentityCode::internal_error:
      return "INTERNAL_ERROR";
  }
  return "INTERNAL_ERROR";
}

}  // namespace axlic::core
