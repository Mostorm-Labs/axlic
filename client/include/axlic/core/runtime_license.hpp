#pragma once

#include "axlic/core/license_state.hpp"
#include "axlic/wire/credential.hpp"

#include <cstdint>
#include <optional>
#include <string_view>

namespace axlic::core {

enum class CredentialState {
  valid,
  absent,
  invalid,
  unsupported,
};

struct StatusView {
  CredentialState credential_state{CredentialState::absent};
  std::optional<std::uint64_t> authority_revision;
  std::optional<std::uint64_t> credential_generation;
};

struct EntitlementView {
  bool granted{};
  std::optional<std::uint64_t> value_u64;
};

class RuntimeLicense {
 public:
  static RuntimeLicense absent();
  static RuntimeLicense from_verification(wire::VerificationResult result);
  // Offline truth from the locally installed credential. The artifact was
  // verified by the frozen A0 verifier at install time, so the stored copy is
  // trusted without re-verification.
  static RuntimeLicense from_installed(const InstalledCredential& installed);

  [[nodiscard]] StatusView status() const;
  [[nodiscard]] EntitlementView entitlement(std::string_view entitlement_id, std::int64_t now) const;

 private:
  CredentialState state_{CredentialState::absent};
  std::optional<wire::Credential> credential_;
};

}  // namespace axlic::core
