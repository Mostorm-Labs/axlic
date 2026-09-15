#include "axlic/core/identity.hpp"
#include "axlic/core/identity_command.hpp"
#include "axlic/core/runtime_license.hpp"
#include "axlic/windows/cng_identity_provider.hpp"
#include "axlic/windows/machine_state.hpp"
#include "axlic/windows/machine_state_repository.hpp"
#include "axlic/windows/mutation_lock.hpp"

#include <nlohmann/json.hpp>

#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
using Json = nlohmann::json;

Json command_result(
    const bool ok,
    const std::string_view code,
    const std::string_view category,
    const bool retryable = false,
    std::optional<Json> data = std::nullopt) {
  Json result = {{"contract_version", "1.0"},
                 {"ok", ok},
                 {"code", code},
                 {"category", category},
                 {"retryable", retryable},
                 {"correlation_ref", "a1-local"}};
  if (data) {
    result["data"] = std::move(*data);
  }
  return result;
}

int emit(const Json& result, const int exit_code) {
  std::cout << result.dump() << '\n';
  return exit_code;
}

int emit(const axlic::core::RenderedCommand& result) {
  std::cout << result.json << '\n';
  return result.exit_code;
}

int credential_failure(const axlic::core::CredentialState state) {
  const auto code = state == axlic::core::CredentialState::absent
                        ? "CREDENTIAL_NOT_FOUND"
                        : (state == axlic::core::CredentialState::unsupported ? "CREDENTIAL_UNSUPPORTED"
                                                                              : "CREDENTIAL_INVALID");
  const auto name = state == axlic::core::CredentialState::absent ? "absent" : "invalid";
  return emit(command_result(false, code, "credential", false, Json{{"credential_state", name}}), 3);
}

int status_command() {
  return credential_failure(axlic::core::RuntimeLicense::absent().status().credential_state);
}

int entitlement_command(const std::string_view entitlement_id) {
  const auto runtime = axlic::core::RuntimeLicense::absent();
  const auto status = runtime.status();
  if (status.credential_state != axlic::core::CredentialState::valid) {
    return credential_failure(status.credential_state);
  }
  const auto now = std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();
  const auto entitlement = runtime.entitlement(entitlement_id, now);
  return emit(
      command_result(
          true,
          "OK",
          "none",
          false,
          Json{{"entitlement_id", entitlement_id}, {"granted", entitlement.granted}}),
      0);
}

std::string generate_key_name() {
  std::array<std::uint8_t, 16> bytes{};
  if (BCryptGenRandom(nullptr, bytes.data(), static_cast<ULONG>(bytes.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) {
    throw std::runtime_error("system RNG unavailable");
  }
  constexpr std::array<char, 16> digits{'0', '1', '2', '3', '4', '5', '6', '7', '8',
                                        '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  std::string name = "Auditoryworks.AxLicense.Identity.v1.";
  name.reserve(name.size() + bytes.size() * 2U);
  for (const auto byte : bytes) {
    name.push_back(digits[byte >> 4U]);
    name.push_back(digits[byte & 0x0fU]);
  }
  return name;
}

int identity_command() {
  axlic::windows::DpapiProtector protector;
  axlic::windows::MachineStateCodec codec(protector);
  axlic::windows::WindowsStateRootSecurity security;
  const auto root = axlic::windows::program_data_state_root();
  if (root.empty()) {
    return emit(command_result(false, "INTERNAL_ERROR", "internal"), 4);
  }
  axlic::windows::WindowsMachineStateStore state(root, codec, security);
  axlic::windows::WindowsMutationLock lock;
  axlic::windows::CngIdentityProvider tpm(axlic::core::ProviderKind::tpm);
  axlic::windows::CngIdentityProvider software(axlic::core::ProviderKind::software);
  axlic::core::IdentityManager manager(
      tpm,
      software,
      state,
      lock,
      axlic::core::IdentityPolicy{.allow_software_fallback = true},
      generate_key_name);
  const auto result = manager.ensure_identity();
  return emit(axlic::core::render_identity_result(result));
}
}  // namespace

int main(const int argc, const char* const argv[]) {
  try {
    if (argc < 2) {
      return emit(command_result(false, "COMMAND_INVALID", "command"), 2);
    }
    const std::string_view command(argv[1]);
    if (command == "status" && argc == 2) {
      return status_command();
    }
    if (command == "entitlement" && argc == 3) {
      return entitlement_command(argv[2]);
    }
    if (command == "identity" && argc == 2) {
      return identity_command();
    }
    return emit(command_result(false, "COMMAND_INVALID", "command"), 2);
  } catch (...) {
    return emit(command_result(false, "INTERNAL_ERROR", "internal"), 4);
  }
}
