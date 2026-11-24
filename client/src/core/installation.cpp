#include "axlic/core/installation.hpp"

#include "axlic/core/device_reference.hpp"
#include "axlic/core/mutation_lock_guard.hpp"
#include "axlic/core/registration.hpp"

#include <utility>

namespace axlic::core {
namespace {

InstallCode state_code(const StateStoreOutcome outcome) {
  switch (outcome) {
    case StateStoreOutcome::ok:
    case StateStoreOutcome::absent:
      return InstallCode::ok;
    case StateStoreOutcome::corrupt:
      return InstallCode::local_state_corrupt;
    case StateStoreOutcome::unsupported:
      return InstallCode::local_state_unsupported;
    case StateStoreOutcome::privilege_required:
      return InstallCode::local_write_privilege_required;
    case StateStoreOutcome::unexpected_failure:
      break;
  }
  return InstallCode::internal_error;
}

InstalledCredential to_installed(const wire::Credential& credential, const std::uint64_t policy_version) {
  return InstalledCredential{
      .credential_id = credential.credential_id,
      .license_grant_id = credential.license_grant_id,
      .binding_id = credential.binding_id,
      .device_id = credential.device_id,
      .authority_revision = credential.authority_revision,
      .credential_generation = credential.credential_generation,
      .issued_at = credential.issued_at,
      .entitlements = credential.entitlements,
      .policy_version = policy_version,
  };
}

}  // namespace

InstallResult install_credential(
    ILicenseStateStore& state,
    IMutationLock& lock,
    const std::span<const wire::TrustedPublicKey> trusted_keys,
    const InstallRequest& request) {
  InstallResult result{InstallCode::internal_error, false};
  const MutationLockGuard guard(lock, result.retryable);
  if (!guard.acquired()) {
    if (result.retryable) {
      return {InstallCode::local_state_busy, true};
    }
    if (guard.denied()) {
      return {InstallCode::local_write_privilege_required, false};
    }
    return {InstallCode::internal_error, false};
  }
  LicenseState current;
  const auto read = state.read();
  if (read.outcome == StateStoreOutcome::ok && read.state) {
    current = *read.state;
  } else if (read.outcome != StateStoreOutcome::ok && read.outcome != StateStoreOutcome::absent) {
    return {state_code(read.outcome), false};
  }
  const auto prepared = state.prepare_for_mutation();
  if (prepared != StateStoreOutcome::ok) {
    return {state_code(prepared), false};
  }

  const auto verification = wire::verify_credential(request.artifact, trusted_keys);
  if (verification.status == wire::VerificationStatus::invalid) {
    return {InstallCode::credential_invalid, false};
  }
  if (verification.status == wire::VerificationStatus::unsupported) {
    return {InstallCode::credential_unsupported, false};
  }
  if (!verification.credential) {
    return {InstallCode::credential_invalid, false};
  }
  const auto& candidate = *verification.credential;

  const auto candidate_device = DeviceReference::parse(candidate.device_id);
  if (!candidate_device || !current.registration ||
      current.registration->device_id != *candidate_device) {
    return {InstallCode::credential_device_mismatch, false};
  }

  InstallCode verdict = InstallCode::ok;
  if (const auto& stored = current.credential) {
    if (candidate.authority_revision < stored->authority_revision ||
        (candidate.authority_revision == stored->authority_revision &&
         candidate.credential_generation < stored->credential_generation)) {
      return {InstallCode::credential_stale, false};
    }
    if (candidate.authority_revision == stored->authority_revision &&
        candidate.credential_generation == stored->credential_generation) {
      verdict = InstallCode::ok_idempotent;
    }
  }

  auto next = current;
  next.credential = to_installed(candidate, request.policy_version);
  const auto written = state.write(next);
  if (written != StateStoreOutcome::ok) {
    return {state_code(written), false};
  }
  return {verdict, false, next.credential};
}

std::optional<HeldVersion> held_version(const LicenseState& state) {
  if (!state.credential) {
    return std::nullopt;
  }
  return HeldVersion{
      .authority_revision = state.credential->authority_revision,
      .credential_generation = state.credential->credential_generation,
      .policy_version = state.credential->policy_version,
  };
}

std::string_view install_code_name(const InstallCode code) noexcept {
  switch (code) {
    case InstallCode::ok:
      return "OK";
    case InstallCode::ok_idempotent:
      return "OK_IDEMPOTENT";
    case InstallCode::credential_stale:
      return "CREDENTIAL_STALE";
    case InstallCode::credential_device_mismatch:
      return "CREDENTIAL_DEVICE_MISMATCH";
    case InstallCode::credential_invalid:
      return "CREDENTIAL_INVALID";
    case InstallCode::credential_unsupported:
      return "CREDENTIAL_UNSUPPORTED";
    case InstallCode::local_state_busy:
      return "LOCAL_STATE_BUSY";
    case InstallCode::local_state_corrupt:
      return "LOCAL_STATE_CORRUPT";
    case InstallCode::local_state_unsupported:
      return "LOCAL_STATE_UNSUPPORTED";
    case InstallCode::local_write_privilege_required:
      return "LOCAL_WRITE_PRIVILEGE_REQUIRED";
    case InstallCode::internal_error:
      break;
  }
  return "INTERNAL_ERROR";
}

}  // namespace axlic::core
