#include "axlic/core/identity_command.hpp"

#include <iostream>
#include <optional>
#include <string_view>

namespace {

std::optional<axlic::core::IdentityResult> scenario(const std::string_view name) {
  using axlic::core::IdentityCode;
  if (name == "provider-unavailable") {
    return axlic::core::IdentityResult{IdentityCode::identity_provider_unavailable, true, std::nullopt};
  }
  if (name == "provider-unsupported") {
    return axlic::core::IdentityResult{IdentityCode::identity_provider_unsupported, false, std::nullopt};
  }
  if (name == "recovery-required") {
    return axlic::core::IdentityResult{IdentityCode::identity_recovery_required, false, std::nullopt};
  }
  if (name == "state-corrupt") {
    return axlic::core::IdentityResult{IdentityCode::local_state_corrupt, false, std::nullopt};
  }
  if (name == "privilege-required") {
    return axlic::core::IdentityResult{IdentityCode::local_write_privilege_required, false, std::nullopt};
  }
  if (name == "state-busy") {
    return axlic::core::IdentityResult{IdentityCode::local_state_busy, true, std::nullopt};
  }
  return std::nullopt;
}

}  // namespace

int main(const int argc, const char* const argv[]) {
  if (argc != 2) {
    return 2;
  }
  const auto result = scenario(argv[1]);
  if (!result) {
    return 2;
  }
  const auto rendered = axlic::core::render_identity_result(*result);
  std::cout << rendered.json << '\n';
  return rendered.exit_code;
}
