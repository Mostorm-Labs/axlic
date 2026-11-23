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
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

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

// Shared wiring for every license-plane command: protected state root,
// machine identity state, license state and the machine-wide mutation lock.
struct LicensePlane {
  axlic::windows::DpapiProtector protector{};
  axlic::windows::MachineStateCodec machine_codec{protector};
  axlic::windows::LicenseStateCodec license_codec{protector};
  axlic::windows::WindowsStateRootSecurity security{};
  std::filesystem::path root{};
  std::optional<axlic::windows::WindowsMachineStateStore> machine_state{};
  std::optional<axlic::windows::WindowsLicenseStateStore> license_state{};
  axlic::windows::WindowsMutationLock lock{};
  std::vector<axlic::wire::TrustedPublicKey> trusted_keys{};

  bool valid{false};
};

bool open_license_plane(LicensePlane& plane) {
  plane.root = axlic::windows::program_data_state_root();
  if (plane.root.empty()) {
    return false;
  }
  plane.machine_state.emplace(plane.root, plane.machine_codec, plane.security);
  plane.license_state.emplace(plane.root, plane.license_codec, plane.security);
  const auto keyring = axlic::core::load_trust_keyring(plane.root);
  plane.trusted_keys = std::move(keyring.keys);
  plane.valid = true;
  return true;
}

int status_command() {
  LicensePlane plane;
  if (!open_license_plane(plane)) {
    return emit(command_result(false, "INTERNAL_ERROR", "internal"), 4);
  }
  return emit(axlic::core::render_status(*plane.license_state));
}

int entitlement_command(const std::string_view entitlement_id) {
  LicensePlane plane;
  if (!open_license_plane(plane)) {
    return emit(command_result(false, "INTERNAL_ERROR", "internal"), 4);
  }
  const auto now = std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();
  return emit(axlic::core::render_entitlement(*plane.license_state, entitlement_id, now));
}

int device_register_command(const std::vector<std::string_view>& args) {
  LicensePlane plane;
  if (!open_license_plane(plane)) {
    return emit(command_result(false, "INTERNAL_ERROR", "internal"), 4);
  }
  if (args.empty()) {
    return emit(axlic::core::render_device_register(*plane.machine_state, *plane.license_state));
  }
  if (args[0] == "adopt" && args.size() == 2U) {
    return emit(axlic::core::render_device_register_adopt(*plane.license_state, plane.lock, args[1]));
  }
  if (args[0] == "reset" && args.size() == 1U) {
    return emit(axlic::core::render_device_register_reset(*plane.license_state, plane.lock));
  }
  return emit(command_result(false, "COMMAND_INVALID", "command"), 2);
}

std::optional<std::vector<std::uint8_t>> read_artifact_file(const std::string& path) {
  constexpr std::size_t kMaximumArtifactSize = 64U * 1024U;
  std::ifstream stream(path, std::ios::binary);
  if (!stream) {
    return std::nullopt;
  }
  std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(stream),
                                  std::istreambuf_iterator<char>()};
  if (bytes.empty() || bytes.size() > kMaximumArtifactSize) {
    return std::nullopt;
  }
  return bytes;
}

std::optional<std::uint64_t> parse_policy_version(const std::string_view text) {
  if (text.empty() || text.size() > 20U) {
    return std::nullopt;
  }
  std::uint64_t value{};
  for (const char character : text) {
    if (character < '0' || character > '9') {
      return std::nullopt;
    }
    value = value * 10U + static_cast<std::uint64_t>(character - '0');
  }
  return value == 0U ? std::nullopt : std::optional<std::uint64_t>(value);
}

int install_command(const std::string& path, const std::uint64_t policy_version) {
  const auto artifact = read_artifact_file(path);
  if (!artifact) {
    return emit(command_result(false, "CREDENTIAL_INVALID", "credential", false,
                               Json{{"reason", "artifact_unreadable_or_oversize"}}),
                3);
  }
  LicensePlane plane;
  if (!open_license_plane(plane)) {
    return emit(command_result(false, "INTERNAL_ERROR", "internal"), 4);
  }
  const axlic::core::InstallRequest request{.artifact = *artifact, .policy_version = policy_version};
  return emit(axlic::core::render_install(*plane.license_state, plane.lock, plane.trusted_keys, request));
}

int held_version_command() {
  LicensePlane plane;
  if (!open_license_plane(plane)) {
    return emit(command_result(false, "INTERNAL_ERROR", "internal"), 4);
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
    if (command == "device-register" && argc >= 2) {
      const std::vector<std::string_view> args(argv + 2, argv + argc);
      return device_register_command(args);
    }
    if (command == "install" && argc == 5 && std::string_view(argv[3]) == "--policy-version") {
      const auto policy_version = parse_policy_version(argv[4]);
      if (!policy_version) {
        return emit(command_result(false, "COMMAND_INVALID", "command"), 2);
      }
      return install_command(argv[2], *policy_version);
    }
    if (command == "held-version" && argc == 2) {
      return held_version_command();
    }
    return emit(command_result(false, "COMMAND_INVALID", "command"), 2);
  } catch (...) {
    return emit(command_result(false, "INTERNAL_ERROR", "internal"), 4);
  }
}
