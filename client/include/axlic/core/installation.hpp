#pragma once

#include "axlic/core/identity.hpp"
#include "axlic/core/license_state.hpp"
#include "axlic/wire/credential.hpp"

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace axlic::core {

enum class InstallCode {
  ok,
  ok_idempotent,
  credential_stale,
  credential_device_mismatch,
  credential_invalid,
  credential_unsupported,
  local_state_busy,
  local_state_corrupt,
  local_state_unsupported,
  local_write_privilege_required,
  internal_error,
};

struct InstallResult {
  InstallCode code{InstallCode::internal_error};
  bool retryable{};
  // Echo of the committed credential on ok/ok_idempotent so callers render
  // the response without re-reading state after the mutation lock released.
  std::optional<InstalledCredential> installed;
};

struct InstallRequest {
  // Owned artifact bytes: the request outlives temporary buffers at the call
  // site, so the payload must not be a borrowed span.
  std::vector<std::uint8_t> artifact;
  std::uint64_t policy_version{};
};

// Held-version triple reported to the host for version probes and
// reconciliation.
struct HeldVersion {
  std::uint64_t authority_revision{};
  std::uint64_t credential_generation{};
  std::uint64_t policy_version{};
};

// Frozen installation pipeline: mutation lock -> read/prepare -> A0 verify ->
// payload device_id vs local registration association -> (authority_revision,
// credential_generation) lexicographic high-watermark (same version is an
// idempotent reinstall refreshing policy_version; an empty-entitlements
// revocation credential is a legal install object) -> atomic commit.
// The sole version-rollback rejection class is credential_stale.
[[nodiscard]] InstallResult install_credential(
    ILicenseStateStore& state,
    IMutationLock& lock,
    std::span<const wire::TrustedPublicKey> trusted_keys,
    const InstallRequest& request);

[[nodiscard]] std::optional<HeldVersion> held_version(const LicenseState& state);

[[nodiscard]] std::string_view install_code_name(InstallCode code) noexcept;

}  // namespace axlic::core
