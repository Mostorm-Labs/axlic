#include "axlic/windows/machine_state_repository.hpp"

#include <catch2/catch_test_macros.hpp>

#define NOMINMAX
#include <windows.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using axlic::core::IdentityRecord;
using axlic::core::ProviderKind;
using axlic::core::StateStoreOutcome;
using axlic::windows::CommitPoint;
using axlic::windows::IDataProtector;
using axlic::windows::IStateRootSecurity;
using axlic::windows::MachineStateCodec;
using axlic::windows::ProtectionOutcome;
using axlic::windows::ProtectionResult;
using axlic::windows::WindowsMachineStateStore;

class XorProtector final : public IDataProtector {
 public:
  ProtectionResult protect(const std::vector<std::uint8_t>& plaintext) override {
    auto bytes = plaintext;
    for (auto& byte : bytes) {
      byte ^= 0x5aU;
    }
    return {ProtectionOutcome::ok, std::move(bytes)};
  }
  ProtectionResult unprotect(const std::vector<std::uint8_t>& protected_bytes) override {
    return protect(protected_bytes);
  }
};

class AllowTestRoot final : public IStateRootSecurity {
 public:
  StateStoreOutcome prepare(const std::filesystem::path& root) override {
    std::filesystem::create_directories(root);
    return StateStoreOutcome::ok;
  }
};

class TemporaryRoot {
 public:
  TemporaryRoot() {
    const auto suffix = std::to_string(GetCurrentProcessId()) + "-" +
                        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    path = std::filesystem::temp_directory_path() / ("axlic-a1-state-" + suffix);
    std::filesystem::create_directories(path);
  }
  ~TemporaryRoot() { std::filesystem::remove_all(path); }
  std::filesystem::path path;
};

IdentityRecord identity(const char digit) {
  return {
      .scheme_id = "axl-win-cng-software-p256-v1",
      .identity_epoch = 1,
      .identity_value = std::string(128, digit),
      .provider_kind = ProviderKind::software,
      .provider_name = "Microsoft Software Key Storage Provider",
      .key_name = std::string("Auditoryworks.AxLicense.Identity.v1.") + std::string(32, digit),
  };
}

}  // namespace

TEST_CASE("machine state repository ignores stale temporary candidates") {
  TemporaryRoot root;
  XorProtector protector;
  MachineStateCodec codec(protector);
  AllowTestRoot security;
  WindowsMachineStateStore store(root.path, codec, security);
  REQUIRE(store.prepare_for_mutation() == StateStoreOutcome::ok);
  REQUIRE(store.write(identity('1')) == StateStoreOutcome::ok);

  std::ofstream(root.path / ".machine-state.v1.999.stale.tmp") << "partial";

  const auto result = store.read();
  REQUIRE(result.outcome == StateStoreOutcome::ok);
  REQUIRE(result.identity == identity('1'));
}

TEST_CASE("a missing state root is an absent state rather than an internal failure") {
  TemporaryRoot root;
  const auto missing = root.path / "not-created";
  XorProtector protector;
  MachineStateCodec codec(protector);
  AllowTestRoot security;
  WindowsMachineStateStore store(missing, codec, security);

  REQUIRE(store.read().outcome == StateStoreOutcome::absent);
}

TEST_CASE("failure before atomic replace preserves the old complete state") {
  TemporaryRoot root;
  XorProtector protector;
  MachineStateCodec codec(protector);
  AllowTestRoot security;
  WindowsMachineStateStore initial(root.path, codec, security);
  REQUIRE(initial.prepare_for_mutation() == StateStoreOutcome::ok);
  REQUIRE(initial.write(identity('1')) == StateStoreOutcome::ok);

  WindowsMachineStateStore interrupted(root.path, codec, security, [](const CommitPoint point) {
    if (point == CommitPoint::before_replace) {
      throw std::runtime_error("simulated abrupt stop");
    }
  });
  REQUIRE(interrupted.write(identity('2')) == StateStoreOutcome::unexpected_failure);

  const auto result = initial.read();
  REQUIRE(result.outcome == StateStoreOutcome::ok);
  REQUIRE(result.identity == identity('1'));
}

TEST_CASE("failure after atomic replace exposes only the new complete state") {
  TemporaryRoot root;
  XorProtector protector;
  MachineStateCodec codec(protector);
  AllowTestRoot security;
  WindowsMachineStateStore initial(root.path, codec, security);
  REQUIRE(initial.prepare_for_mutation() == StateStoreOutcome::ok);
  REQUIRE(initial.write(identity('1')) == StateStoreOutcome::ok);

  WindowsMachineStateStore interrupted(root.path, codec, security, [](const CommitPoint point) {
    if (point == CommitPoint::after_replace) {
      throw std::runtime_error("simulated abrupt stop");
    }
  });
  REQUIRE(interrupted.write(identity('2')) == StateStoreOutcome::unexpected_failure);

  const auto result = initial.read();
  REQUIRE(result.outcome == StateStoreOutcome::ok);
  REQUIRE(result.identity == identity('2'));
}
