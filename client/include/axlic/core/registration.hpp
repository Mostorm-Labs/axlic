#pragma once

#include "axlic/core/device_reference.hpp"
#include "axlic/core/identity.hpp"
#include "axlic/core/license_state.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace axlic::core {

enum class RegistrationCode {
  ok,
  local_state_busy,
  local_state_corrupt,
  local_state_unsupported,
  local_write_privilege_required,
  internal_error,
};

struct RegistrationResult {
  RegistrationCode code{RegistrationCode::internal_error};
  bool retryable{};
};

// Dual-state self-report value for the connection identity: the local
// identity_value (128-hex public key) when unregistered or reference-lost,
// the platform-assigned device reference text when registered.
struct SelfReportValue {
  bool registered{};
  std::string value;
};

[[nodiscard]] SelfReportValue resolve_self_report(
    const std::optional<RegistrationAssociation>& registration,
    const std::string& identity_value);

// Persists the platform-assigned reference after a valid platform result.
// The installed credential is preserved.
[[nodiscard]] RegistrationResult adopt_registration(
    ILicenseStateStore& state,
    IMutationLock& lock,
    const DeviceReference& device_id);

// Terminal-rejection handling: drops the local reference so the self-report
// falls back to identity_value. The installed credential is preserved until a
// new-subject credential replaces it.
[[nodiscard]] RegistrationResult reset_registration(
    ILicenseStateStore& state,
    IMutationLock& lock);

[[nodiscard]] std::string_view registration_code_name(RegistrationCode code) noexcept;

}  // namespace axlic::core
