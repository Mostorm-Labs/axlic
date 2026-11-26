#include "axlic/core/cli_numbers.hpp"
#include "axlic/core/command_envelope.hpp"
#include "axlic/core/identity.hpp"
#include "axlic/core/identity_command.hpp"
#include "axlic/core/installation.hpp"
#include "axlic/core/license_command.hpp"
#include "axlic/core/license_state.hpp"
#include "axlic/core/runtime_license.hpp"
#include "axlic/core/trust_keyring.hpp"
#include "axlic/windows/cng_identity_provider.hpp"
#include "axlic/windows/license_state_repository.hpp"
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
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using Json = nlohmann::json;

int emit(const Json& result, const int exit_code) {
  std::cout << result.dump() << '\n';
  return exit_code;
}

int emit(const axlic::core::RenderedCommand& result) {
  std::cout << result.json << '\n';
  return result.exit_code;
}

// Shared wiring for every license-plane command: protected state root,
// license state and the machine-wide mutation lock. The machine-identity
// store and trust keyring are assembled by the commands that need them.
struct LicensePlane {
  axlic::windows::DpapiProtector protector{};
  axlic::windows::LicenseStateCodec license_codec{protector};
  axlic::windows::WindowsStateRootSecurity security{};
  std::filesystem::path root{};
  std::optional<axlic::windows::WindowsLicenseStateStore> license_state{};
  axlic::windows::WindowsMutationLock lock{};
};

bool open_license_plane(LicensePlane& plane) {
  plane.root = axlic::windows::program_data_state_root();
  if (plane.root.empty()) {
    return false;
  }
  plane.license_state.emplace(plane.root, plane.license_codec, plane.security);
  return true;
}

int status_command() {
  LicensePlane plane;
  if (!open_license_plane(plane)) {
    return emit(axlic::core::command_result("a1-local", false, "INTERNAL_ERROR", "internal"), axlic::core::kExitInternal);
  }
  return emit(axlic::core::render_status(*plane.license_state));
}

int entitlement_command(const std::string_view entitlement_id) {
  LicensePlane plane;
  if (!open_license_plane(plane)) {
    return emit(axlic::core::command_result("a1-local", false, "INTERNAL_ERROR", "internal"), axlic::core::kExitInternal);
  }
  const auto now = std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();
  return emit(axlic::core::render_entitlement(*plane.license_state, entitlement_id, now));
}

int device_register_command(const std::vector<std::string_view>& args) {
  LicensePlane plane;
  if (!open_license_plane(plane)) {
    return emit(axlic::core::command_result("a1-local", false, "INTERNAL_ERROR", "internal"), axlic::core::kExitInternal);
  }
  axlic::windows::MachineStateCodec machine_codec{plane.protector};
  axlic::windows::WindowsMachineStateStore machine_state(plane.root, machine_codec, plane.security);
  if (args.empty()) {
    return emit(axlic::core::render_device_register(machine_state, *plane.license_state));
  }
  if (args[0] == "adopt" && args.size() == 2U) {
    return emit(axlic::core::render_device_register_adopt(*plane.license_state, plane.lock, args[1]));
  }
  if (args[0] == "reset" && args.size() == 1U) {
    return emit(axlic::core::render_device_register_reset(*plane.license_state, plane.lock));
  }
  return emit(axlic::core::command_result("a1-local", false, "COMMAND_INVALID", "command"), axlic::core::kExitCommandInvalid);
}

int install_command(const std::string& path) {
  const auto artifact = axlic::core::read_artifact_file(path);
  if (!artifact) {
    return emit(axlic::core::command_result("a1-local", false, "CREDENTIAL_INVALID", "credential", false,
                               Json{{"reason", "artifact_unreadable_or_oversize"}}),
                axlic::core::kExitCredentialFailure);
  }
  LicensePlane plane;
  if (!open_license_plane(plane)) {
    return emit(axlic::core::command_result("a1-local", false, "INTERNAL_ERROR", "internal"), axlic::core::kExitInternal);
  }
  const auto keyring = axlic::core::load_trust_keyring(plane.root);
  const axlic::core::InstallRequest request{.artifact = *artifact};
  return emit(axlic::core::render_install(*plane.license_state, plane.lock, keyring.keys, request));
}

int held_version_command() {
  LicensePlane plane;
  if (!open_license_plane(plane)) {
    return emit(axlic::core::command_result("a1-local", false, "INTERNAL_ERROR", "internal"), axlic::core::kExitInternal);
  }
  return emit(axlic::core::render_held_version(*plane.license_state));
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
    return emit(axlic::core::command_result("a1-local", false, "INTERNAL_ERROR", "internal"), axlic::core::kExitInternal);
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
      return emit(axlic::core::command_result("a1-local", false, "COMMAND_INVALID", "command"), axlic::core::kExitCommandInvalid);
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
    if (command == "device-register") {
      const std::vector<std::string_view> args(argv + 2, argv + argc);
      return device_register_command(args);
    }
    // The retired --policy-version flag is never an artifact path.
    if (command == "install" && argc == 3 && std::string_view(argv[2]) != "--policy-version") {
      return install_command(argv[2]);
    }
    if (command == "held-version" && argc == 2) {
      return held_version_command();
    }
    return emit(axlic::core::command_result("a1-local", false, "COMMAND_INVALID", "command"), axlic::core::kExitCommandInvalid);
  } catch (...) {
    return emit(axlic::core::command_result("a1-local", false, "INTERNAL_ERROR", "internal"), axlic::core::kExitInternal);
  }
}
