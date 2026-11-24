#include "axlic/core/runtime_license.hpp"

#include <utility>

namespace axlic::core {

RuntimeLicense RuntimeLicense::absent() { return {}; }

RuntimeLicense RuntimeLicense::from_installed(const InstalledCredential& installed) {
  RuntimeLicense runtime;
  runtime.state_ = CredentialState::valid;
  wire::Credential credential;
  credential.credential_id = installed.credential_id;
  credential.license_grant_id = installed.license_grant_id;
  credential.authority_revision = installed.authority_revision;
  credential.binding_id = installed.binding_id;
  credential.device_id = installed.device_id;
  credential.credential_generation = installed.credential_generation;
  credential.issued_at = installed.issued_at;
  credential.entitlements = installed.entitlements;
  runtime.credential_ = std::move(credential);
  return runtime;
}

RuntimeLicense RuntimeLicense::from_verification(wire::VerificationResult result) {
  RuntimeLicense runtime;
  switch (result.status) {
    case wire::VerificationStatus::valid:
      if (result.credential) {
        runtime.state_ = CredentialState::valid;
        runtime.credential_ = std::move(result.credential);
      } else {
        runtime.state_ = CredentialState::invalid;
      }
      break;
    case wire::VerificationStatus::invalid:
      runtime.state_ = CredentialState::invalid;
      break;
    case wire::VerificationStatus::unsupported:
      runtime.state_ = CredentialState::unsupported;
      break;
  }
  return runtime;
}

StatusView RuntimeLicense::status() const {
  StatusView result{.credential_state = state_};
  if (state_ == CredentialState::valid && credential_) {
    result.authority_revision = credential_->authority_revision;
    result.credential_generation = credential_->credential_generation;
  }
  return result;
}

EntitlementView RuntimeLicense::entitlement(const std::string_view entitlement_id, const std::int64_t now) const {
  if (state_ != CredentialState::valid || !credential_) {
    return {};
  }
  for (const auto& entitlement : credential_->entitlements) {
    if (entitlement.entitlement_id != entitlement_id) {
      continue;
    }
    if (entitlement.validity.not_before && now < *entitlement.validity.not_before) {
      return {};
    }
    if (entitlement.validity.kind == wire::ValidityKind::bounded &&
        (!entitlement.validity.not_after || now >= *entitlement.validity.not_after)) {
      return {};
    }
    return {.granted = true, .value_u64 = entitlement.max_u64};
  }
  return {};
}

}  // namespace axlic::core
