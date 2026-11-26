#include "axlic/windows/license_state_repository.hpp"

#include <catch2/catch_test_macros.hpp>

#define NOMINMAX
#include <windows.h>

#include <nlohmann/json.hpp>

#include <chrono>
#include <array>
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
      .device_id = std::string("axldev_") + std::string(32, '7'),
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

std::string encode_hex(const std::vector<std::uint8_t>& bytes) {
  constexpr std::array<char, 16> digits{'0', '1', '2', '3', '4', '5', '6', '7', '8',
                                        '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  std::string hex;
  for (const auto byte : bytes) {
    hex.push_back(digits[byte >> 4U]);
    hex.push_back(digits[byte & 0x0fU]);
  }
  return hex;
}

std::vector<std::uint8_t> decode_hex(const std::string& hex) {
  const auto nibble = [](const char character) {
    return static_cast<std::uint8_t>(character <= '9' ? character - '0' : character - 'a' + 10);
  };
  std::vector<std::uint8_t> bytes;
  for (std::size_t index = 0; index < hex.size(); index += 2U) {
    bytes.push_back(static_cast<std::uint8_t>((nibble(hex[index]) << 4U) | nibble(hex[index + 1U])));
  }
  return bytes;
}

// Wraps a plaintext state document in the state-file envelope the way the
// store does, so a test can plant a document the current writer no longer
// produces.
std::string make_carrier(IDataProtector& protector, const std::string& plaintext_json) {
  const std::vector<std::uint8_t> plaintext(plaintext_json.begin(), plaintext_json.end());
  const auto protected_value = protector.protect(plaintext);
  return "{\"format\":\"axlicense-license-state\",\"envelope_version\":1,\"protected_blob_hex\":\"" +
         encode_hex(protected_value.bytes) + "\"}";
}

// The plaintext state document stored in a state file protected with the
// given protector.
std::string read_plaintext(IDataProtector& protector, const std::string& carrier) {
  const auto protected_hex = nlohmann::json::parse(carrier).at("protected_blob_hex").get<std::string>();
  const auto unprotected = protector.unprotect(decode_hex(protected_hex));
  return std::string(unprotected.bytes.begin(), unprotected.bytes.end());
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

TEST_CASE("license state malformed odd and non-hex envelopes fail closed") {
  TemporaryRoot root;
  XorProtector protector;
  LicenseStateCodec codec(protector);
  AllowTestRoot security;
  WindowsLicenseStateStore store(root.path, codec, security);
  // An odd-length hex blob must be rejected by validation, never reach the
  // pair-consumed decoder.
  write_state_file(store.state_path(),
                   R"json({"format":"axlicense-license-state","envelope_version":1,"protected_blob_hex":"0"})json");
  CHECK(store.read().outcome == StateStoreOutcome::corrupt);
  write_state_file(store.state_path(),
                   R"json({"format":"axlicense-license-state","envelope_version":1,"protected_blob_hex":"zz"})json");
  CHECK(store.read().outcome == StateStoreOutcome::corrupt);
}

TEST_CASE("oversized unsigned timestamps fail closed as corrupt") {
  TemporaryRoot root;
  XorProtector protector;
  LicenseStateCodec codec(protector);
  AllowTestRoot security;
  WindowsLicenseStateStore store(root.path, codec, security);

  // issued_at above INT64_MAX would wrap to a negative value when read as a
  // static_cast int64.
  write_state_file(store.state_path(), make_carrier(protector,
      R"json({"schema_version":{"major":1,"minor":0},"state_generation":1,"credential":{"credential_id":"cred-1","license_grant_id":"grant-1","binding_id":"binding-1","device_id":"axldev_00000000000000000000000000000007","authority_revision":3,"credential_generation":7,"issued_at":18446744073709551615,"entitlements":[]}})json"));
  CHECK(store.read().outcome == StateStoreOutcome::corrupt);

  // not_before above INT64_MAX would wrap to -1 and fail open.
  write_state_file(store.state_path(), make_carrier(protector,
      R"json({"schema_version":{"major":1,"minor":0},"state_generation":1,"credential":{"credential_id":"cred-1","license_grant_id":"grant-1","binding_id":"binding-1","device_id":"axldev_00000000000000000000000000000007","authority_revision":3,"credential_generation":7,"issued_at":100,"entitlements":[{"entitlement_id":"e","product_id":"p","right_kind":1,"grant_semantics":1,"validity":{"kind":2,"not_before":18446744073709551615,"not_after":20}}]}})json"));
  CHECK(store.read().outcome == StateStoreOutcome::corrupt);
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

TEST_CASE("written state file carries no channel policy version") {
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

  const auto document = nlohmann::json::parse(read_plaintext(protector, read_state_file(store.state_path())));
  const auto& credential_document = document.at("credential");
  CHECK_FALSE(credential_document.contains("policy_version"));
  CHECK(credential_document.size() == 8U);
}
