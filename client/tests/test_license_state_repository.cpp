#include "axlic/windows/license_state_repository.hpp"

#include <catch2/catch_test_macros.hpp>

#define NOMINMAX
#include <windows.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using axlic::core::DeviceReference;
using axlic::core::InstalledCredential;
using axlic::core::LicenseState;
using axlic::core::RegistrationAssociation;
using axlic::core::StateStoreOutcome;
using axlic::windows::CommitPoint;
using axlic::windows::IDataProtector;
using axlic::windows::IStateRootSecurity;
using axlic::windows::LicenseStateCodec;
using axlic::windows::ProtectionOutcome;
using axlic::windows::ProtectionResult;
using axlic::windows::WindowsLicenseStateStore;

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

class CorruptingProtector final : public IDataProtector {
 public:
  ProtectionResult protect(const std::vector<std::uint8_t>& plaintext) override {
    return {ProtectionOutcome::ok, plaintext};
  }
  ProtectionResult unprotect(const std::vector<std::uint8_t>&) override {
    return {ProtectionOutcome::corrupt, {}};
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
    path = std::filesystem::temp_directory_path() / ("axlic-a2-license-state-" + suffix);
    std::filesystem::create_directories(path);
  }
  ~TemporaryRoot() { std::filesystem::remove_all(path); }
  TemporaryRoot(const TemporaryRoot&) = delete;
  TemporaryRoot& operator=(const TemporaryRoot&) = delete;
  std::filesystem::path path;
};

DeviceReference reference(const char digit) {
  const auto parsed = DeviceReference::parse(std::string("axldev_") + std::string(32, digit));
  if (!parsed) {
    throw std::runtime_error("reference fixture");
  }
  return *parsed;
}

InstalledCredential credential(const std::uint64_t revision, const std::uint64_t generation) {
  axlic::wire::Entitlement runtime{
      .entitlement_id = "launcher.runtime",
      .product_id = "launcher",
  };
  axlic::wire::Entitlement bounded{
      .entitlement_id = "launcher.seats",
      .product_id = "launcher",
      .grant_semantics = axlic::wire::GrantSemantics::bounded_u64,
      .validity{.kind = axlic::wire::ValidityKind::bounded, .not_before = 10, .not_after = 20},
      .max_u64 = 5,
  };
  return {
      .credential_id = "cred-1",
      .license_grant_id = "grant-1",
      .binding_id = "binding-1",
      .authority_revision = revision,
      .credential_generation = generation,
      .issued_at = 100,
      .entitlements = {std::move(runtime), std::move(bounded)},
      .policy_version = 2,
  };
}

std::string read_state_file(const std::filesystem::path& path) {
  std::ifstream stream(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

void write_state_file(const std::filesystem::path& path, const std::string& content) {
  std::ofstream stream(path, std::ios::binary | std::ios::trunc);
  stream << content;
}

}  // namespace

TEST_CASE("license state file is absent until first write") {
  TemporaryRoot root;
  XorProtector protector;
  LicenseStateCodec codec(protector);
  AllowTestRoot security;
  WindowsLicenseStateStore store(root.path, codec, security);
  const auto read = store.read();
  CHECK(read.outcome == StateStoreOutcome::absent);
  CHECK_FALSE(read.state.has_value());
}

TEST_CASE("license state round-trips registration and credential") {
  TemporaryRoot root;
  XorProtector protector;
  LicenseStateCodec codec(protector);
  AllowTestRoot security;
  WindowsLicenseStateStore store(root.path, codec, security);

  LicenseState written;
  written.registration = RegistrationAssociation{.device_id = reference('7')};
  written.credential = credential(3, 7);
  REQUIRE(store.prepare_for_mutation() == StateStoreOutcome::ok);
  REQUIRE(store.write(written) == StateStoreOutcome::ok);

  const auto read = store.read();
  REQUIRE(read.outcome == StateStoreOutcome::ok);
  REQUIRE(read.state.has_value());
  CHECK(read.state->registration->device_id == written.registration->device_id);
  const auto& restored = *read.state->credential;
  const auto& original = *written.credential;
  CHECK(restored.credential_id == original.credential_id);
  CHECK(restored.license_grant_id == original.license_grant_id);
  CHECK(restored.binding_id == original.binding_id);
  CHECK(restored.authority_revision == original.authority_revision);
  CHECK(restored.credential_generation == original.credential_generation);
  CHECK(restored.issued_at == original.issued_at);
  CHECK(restored.policy_version == original.policy_version);
  REQUIRE(restored.entitlements.size() == 2U);
  CHECK(restored.entitlements[0].entitlement_id == "launcher.runtime");
  CHECK(restored.entitlements[0].grant_semantics == axlic::wire::GrantSemantics::presence);
  CHECK(restored.entitlements[1].grant_semantics == axlic::wire::GrantSemantics::bounded_u64);
  CHECK(restored.entitlements[1].max_u64 == 5U);
  CHECK(restored.entitlements[1].validity.kind == axlic::wire::ValidityKind::bounded);
  CHECK(restored.entitlements[1].validity.not_before == 10);
  CHECK(restored.entitlements[1].validity.not_after == 20);
}

TEST_CASE("empty entitlements revocation credential round-trips") {
  TemporaryRoot root;
  XorProtector protector;
  LicenseStateCodec codec(protector);
  AllowTestRoot security;
  WindowsLicenseStateStore store(root.path, codec, security);

  LicenseState written;
  written.registration = RegistrationAssociation{.device_id = reference('7')};
  written.credential = credential(4, 9);
  written.credential->entitlements.clear();
  REQUIRE(store.prepare_for_mutation() == StateStoreOutcome::ok);
  REQUIRE(store.write(written) == StateStoreOutcome::ok);

  const auto read = store.read();
  REQUIRE(read.outcome == StateStoreOutcome::ok);
  CHECK(read.state->credential->entitlements.empty());
}

TEST_CASE("clearing the credential is persisted as absent") {
  TemporaryRoot root;
  XorProtector protector;
  LicenseStateCodec codec(protector);
  AllowTestRoot security;
  WindowsLicenseStateStore store(root.path, codec, security);

  LicenseState written;
  written.registration = RegistrationAssociation{.device_id = reference('7')};
  written.credential = credential(3, 7);
  REQUIRE(store.prepare_for_mutation() == StateStoreOutcome::ok);
  REQUIRE(store.write(written) == StateStoreOutcome::ok);

  written.credential.reset();
  REQUIRE(store.write(written) == StateStoreOutcome::ok);
  const auto read = store.read();
  REQUIRE(read.outcome == StateStoreOutcome::ok);
  CHECK(read.state->registration.has_value());
  CHECK_FALSE(read.state->credential.has_value());
}

TEST_CASE("corrupt carrier is classified as corrupt") {
  TemporaryRoot root;
  XorProtector protector;
  LicenseStateCodec codec(protector);
  AllowTestRoot security;
  WindowsLicenseStateStore store(root.path, codec, security);
  write_state_file(store.state_path(), "not json");
  CHECK(store.read().outcome == StateStoreOutcome::corrupt);
}

TEST_CASE("unprotectable carrier is classified as corrupt") {
  TemporaryRoot root;
  // Encode with a working protector so the file is structurally valid, then
  // read through one whose unprotect fails.
  XorProtector writer;
  LicenseStateCodec writer_codec(writer);
  CorruptingProtector reader;
  LicenseStateCodec reader_codec(reader);
  AllowTestRoot security;
  WindowsLicenseStateStore write_store(root.path, writer_codec, security);
  LicenseState written;
  written.registration = RegistrationAssociation{.device_id = reference('7')};
  REQUIRE(write_store.prepare_for_mutation() == StateStoreOutcome::ok);
  REQUIRE(write_store.write(written) == StateStoreOutcome::ok);

  WindowsLicenseStateStore read_store(root.path, reader_codec, security);
  CHECK(read_store.read().outcome == StateStoreOutcome::corrupt);
}

TEST_CASE("future envelope version is classified as unsupported") {
  TemporaryRoot root;
  XorProtector protector;
  LicenseStateCodec codec(protector);
  AllowTestRoot security;
  WindowsLicenseStateStore store(root.path, codec, security);
  write_state_file(store.state_path(), R"json({"format":"axlicense-license-state","envelope_version":2,"protected_blob_hex":"00"})json");
  CHECK(store.read().outcome == StateStoreOutcome::unsupported);
}

TEST_CASE("write failure at a commit point leaves the previous state intact") {
  TemporaryRoot root;
  XorProtector protector;
  LicenseStateCodec codec(protector);
  AllowTestRoot security;

  LicenseState original;
  original.registration = RegistrationAssociation{.device_id = reference('7')};
  original.credential = credential(3, 7);
  {
    WindowsLicenseStateStore store(root.path, codec, security);
    REQUIRE(store.prepare_for_mutation() == StateStoreOutcome::ok);
    REQUIRE(store.write(original) == StateStoreOutcome::ok);
  }

  LicenseState next;
  next.registration = original.registration;
  next.credential = credential(4, 8);
  bool committed = false;
  const auto observer = [&committed](const CommitPoint point) {
    if (point == CommitPoint::before_replace) {
      throw std::runtime_error("simulated crash");
    }
    if (point == CommitPoint::after_replace) {
      committed = true;
    }
  };
  {
    WindowsLicenseStateStore store(root.path, codec, security, observer);
    REQUIRE(store.write(next) == StateStoreOutcome::unexpected_failure);
  }
  CHECK_FALSE(committed);
  WindowsLicenseStateStore verify(root.path, codec, security);
  const auto read = verify.read();
  REQUIRE(read.outcome == StateStoreOutcome::ok);
  CHECK(read.state->credential->authority_revision == 3U);
  CHECK(read.state->credential->credential_generation == 7U);
  // No temporary candidates survive a failed write.
  std::size_t temporaries = 0;
  for (const auto& entry : std::filesystem::directory_iterator(root.path)) {
    if (entry.path().extension() == ".tmp") {
      ++temporaries;
    }
  }
  CHECK(temporaries == 0U);
}

TEST_CASE("state file name and protected envelope format are stable") {
  TemporaryRoot root;
  XorProtector protector;
  LicenseStateCodec codec(protector);
  AllowTestRoot security;
  WindowsLicenseStateStore store(root.path, codec, security);
  LicenseState written;
  written.registration = RegistrationAssociation{.device_id = reference('7')};
  REQUIRE(store.prepare_for_mutation() == StateStoreOutcome::ok);
  REQUIRE(store.write(written) == StateStoreOutcome::ok);
  CHECK(store.state_path().filename() == "license-state.v1.json");
  const auto carrier = read_state_file(store.state_path());
  CHECK(carrier.find("\"axlicense-license-state\"") != std::string::npos);
  CHECK(carrier.find("\"envelope_version\":1") != std::string::npos);
}

TEST_CASE("invalid credential payload is refused at encode") {
  TemporaryRoot root;
  XorProtector protector;
  LicenseStateCodec codec(protector);
  AllowTestRoot security;
  WindowsLicenseStateStore store(root.path, codec, security);
  LicenseState written;
  written.registration = RegistrationAssociation{.device_id = reference('7')};
  written.credential = credential(0, 7);
  CHECK(store.write(written) == StateStoreOutcome::unexpected_failure);
  written.credential = credential(3, 0);
  CHECK(store.write(written) == StateStoreOutcome::unexpected_failure);
}
