// Scenario executable: drives the real A2 command implementations (same
// render functions as the production CLI) against an injected temporary
// state root and optional trust-floor document, so the Python contract
// oracle can exercise the full pipeline hermetically.
//
// usage: a2_cli_scenario <state_root> [--floor <floor.json>] <command ...>
//   status
//   entitlement <entitlement_id> <unix_now>
//   device-register
//   device-register adopt <axldev_reference>
//   device-register reset
//   install <artifact_path> --policy-version <n>
//   held-version

#include "axlic/core/installation.hpp"
#include "axlic/core/license_command.hpp"
#include "axlic/core/license_state.hpp"
#include "axlic/core/trust_keyring.hpp"
#include "axlic/windows/license_state_repository.hpp"
#include "axlic/windows/machine_state.hpp"
#include "axlic/windows/machine_state_repository.hpp"
#include "axlic/windows/mutation_lock.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::size_t kMaximumArtifactSize = 64U * 1024U;

std::optional<std::string> read_text_file(const std::filesystem::path& path) {
  std::ifstream stream(path, std::ios::binary);
  if (!stream) {
    return std::nullopt;
  }
  return std::string{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

std::optional<std::uint64_t> parse_u64(const std::string_view text) {
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
  return value;
}

// Test-root security: creates the directory but does not apply the
// production ProgramData DACL, so the driving test process retains ownership
// and can clean the temporary state root up.
class AllowRootSecurity final : public axlic::windows::IStateRootSecurity {
 public:
  axlic::core::StateStoreOutcome prepare(const std::filesystem::path& root) override {
    std::error_code error;
    std::filesystem::create_directories(root, error);
    if (error) {
      return axlic::core::StateStoreOutcome::unexpected_failure;
    }
    return axlic::core::StateStoreOutcome::ok;
  }
};

// Establishes a machine identity record in the temp root so device-register
// has an identity_value to report. Uses the software-provider record shape
// accepted by MachineStateCodec validation.
bool seed_identity(const std::filesystem::path& root) {
  axlic::windows::DpapiProtector protector;
  axlic::windows::MachineStateCodec codec(protector);
  AllowRootSecurity security;
  axlic::windows::WindowsMachineStateStore store(root, codec, security);
  if (store.prepare_for_mutation() != axlic::core::StateStoreOutcome::ok) {
    return false;
  }
  axlic::core::IdentityRecord record{
      .scheme_id = "axl-win-cng-software-p256-v1",
      .identity_epoch = 1,
      .identity_value = std::string(128, 'c'),
      .provider_kind = axlic::core::ProviderKind::software,
      .provider_name = "Microsoft Software Key Storage Provider",
      .key_name = std::string("Auditoryworks.AxLicense.Identity.v1.") + std::string(32, 'c'),
  };
  return store.write(record) == axlic::core::StateStoreOutcome::ok;
}

int invalid_command() {
  std::cout << "{\"category\":\"command\",\"code\":\"COMMAND_INVALID\",\"contract_version\":\"1.0\","
               "\"correlation_ref\":\"a2-scenario\",\"ok\":false,\"retryable\":false}\n";
  return 2;
}

}  // namespace

int main(const int argc, const char* const argv[]) {
  if (argc < 3) {
    return invalid_command();
  }
  std::vector<std::string_view> args(argv + 1, argv + argc);
  const std::filesystem::path root(args[0]);
  std::string floor_document;
  std::size_t index = 1;
  if (args[index] == "--floor" && args.size() > static_cast<std::size_t>(index) + 2U) {
    const auto floor = read_text_file(std::filesystem::path(args[index + 1]));
    if (!floor) {
      return invalid_command();
    }
    floor_document = *floor;
    index += 2U;
  }
  const std::string_view command = args[index];
  args.erase(args.begin(), args.begin() + static_cast<std::ptrdiff_t>(index) + 1);

  axlic::windows::DpapiProtector protector;
  axlic::windows::MachineStateCodec machine_codec(protector);
  axlic::windows::LicenseStateCodec license_codec(protector);
  AllowRootSecurity security;
  axlic::windows::WindowsMachineStateStore machine_state(root, machine_codec, security);
  axlic::windows::WindowsLicenseStateStore license_state(root, license_codec, security);
  axlic::windows::WindowsMutationLock lock;

  if (command == "seed-identity") {
    if (!seed_identity(root)) {
      std::cout << "{\"category\":\"internal\",\"code\":\"INTERNAL_ERROR\",\"contract_version\":\"1.0\","
                   "\"correlation_ref\":\"a2-scenario\",\"ok\":false,\"retryable\":false}\n";
      return 4;
    }
    std::cout << "{\"category\":\"none\",\"code\":\"OK\",\"contract_version\":\"1.0\","
                 "\"correlation_ref\":\"a2-scenario\",\"ok\":true,\"retryable\":false}\n";
    return 0;
  }
  if (command == "status" && args.empty()) {
    const auto rendered = axlic::core::render_status(license_state);
    std::cout << rendered.json << '\n';
    return rendered.exit_code;
  }
  if (command == "entitlement" && args.size() == 2U) {
    const auto now = parse_u64(args[1]);
    if (!now) {
      return invalid_command();
    }
    const auto rendered = axlic::core::render_entitlement(
        license_state, args[0], static_cast<std::int64_t>(*now));
    std::cout << rendered.json << '\n';
    return rendered.exit_code;
  }
  if (command == "device-register" && args.empty()) {
    const auto rendered = axlic::core::render_device_register(machine_state, license_state);
    std::cout << rendered.json << '\n';
    return rendered.exit_code;
  }
  if (command == "device-register" && args[0] == "adopt" && args.size() == 2U) {
    const auto rendered = axlic::core::render_device_register_adopt(license_state, lock, args[1]);
    std::cout << rendered.json << '\n';
    return rendered.exit_code;
  }
  if (command == "device-register" && args[0] == "reset" && args.size() == 1U) {
    const auto rendered = axlic::core::render_device_register_reset(license_state, lock);
    std::cout << rendered.json << '\n';
    return rendered.exit_code;
  }
  if (command == "install" && args.size() == 3U && args[1] == "--policy-version") {
    const auto policy_version = parse_u64(args[2]);
    const auto artifact_text = read_text_file(std::filesystem::path(args[0]));
    if (!policy_version || !artifact_text || artifact_text->empty() ||
        artifact_text->size() > kMaximumArtifactSize) {
      return invalid_command();
    }
    const std::vector<std::uint8_t> artifact(artifact_text->begin(), artifact_text->end());
    const auto keyring = floor_document.empty()
                             ? axlic::core::load_trust_keyring(root)
                             : axlic::core::load_trust_keyring_from(floor_document, root);
    const axlic::core::InstallRequest request{.artifact = artifact, .policy_version = *policy_version};
    const auto rendered =
        axlic::core::render_install(license_state, lock, keyring.keys, request);
    std::cout << rendered.json << '\n';
    return rendered.exit_code;
  }
  if (command == "held-version" && args.empty()) {
    const auto rendered = axlic::core::render_held_version(license_state);
    std::cout << rendered.json << '\n';
    return rendered.exit_code;
  }
  return invalid_command();
}
