#pragma once

#include "axlic/core/identity_command.hpp"
#include "axlic/core/identity.hpp"
#include "axlic/core/installation.hpp"
#include "axlic/core/license_state.hpp"
#include "axlic/core/registration.hpp"
#include "axlic/wire/credential.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

namespace axlic::core {

// Host-facing command surfaces over injected stores/locks so the production
// CLI (protected state root) and the scenario executable (temporary state
// root) share one implementation. All commands emit the stable single-line
// JSON envelope (contract_version 1.0) with the frozen exit-code family:
// 0 success / 2 command-invalid / 3 credential-class or local-state failure /
// 4 internal / 5 retryable.

[[nodiscard]] RenderedCommand render_status(ILicenseStateStore& license_state);

[[nodiscard]] RenderedCommand render_entitlement(
    ILicenseStateStore& license_state,
    std::string_view entitlement_id,
    std::int64_t now);

[[nodiscard]] RenderedCommand render_device_register(
    IMachineStateStore& machine_state,
    ILicenseStateStore& license_state);

[[nodiscard]] RenderedCommand render_device_register_adopt(
    ILicenseStateStore& license_state,
    IMutationLock& lock,
    std::string_view device_reference_text);

[[nodiscard]] RenderedCommand render_device_register_reset(
    ILicenseStateStore& license_state,
    IMutationLock& lock);

[[nodiscard]] RenderedCommand render_install(
    ILicenseStateStore& license_state,
    IMutationLock& lock,
    const std::vector<wire::TrustedPublicKey>& trusted_keys,
    const InstallRequest& request);

[[nodiscard]] RenderedCommand render_held_version(ILicenseStateStore& license_state);

}  // namespace axlic::core
