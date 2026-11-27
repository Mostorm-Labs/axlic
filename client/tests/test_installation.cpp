#include "axlic/core/installation.hpp"
#include "axlic/core/license_state.hpp"
#include "axlic/core/runtime_license.hpp"

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include <array>
#include <cstdint>
#include <fstream>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using axlic::core::DeviceReference;
using axlic::core::ILicenseStateStore;
using axlic::core::IMutationLock;
using axlic::core::InstallCode;
using axlic::core::InstallRequest;
using axlic::core::LicenseState;
using axlic::core::LicenseStateRead;
using axlic::core::LockOutcome;
using axlic::core::RegistrationAssociation;
using axlic::core::StateStoreOutcome;

class FakeStateStore final : public ILicenseStateStore {
 public:
  LicenseStateRead read() override {
    if (fail_read != StateStoreOutcome::ok) {
      return {fail_read, std::nullopt};
    }
    LicenseStateRead result{exists ? StateStoreOutcome::ok : StateStoreOutcome::absent, std::nullopt};
    if (exists) {
      result.state = state;
    }
    return result;
  }
  StateStoreOutcome prepare_for_mutation() override { return StateStoreOutcome::ok; }
  StateStoreOutcome write(const LicenseState& value) override {
    state = value;
    exists = true;
    writes += 1U;
    return StateStoreOutcome::ok;
  }

  LicenseState state;
  bool exists{false};
  StateStoreOutcome fail_read{StateStoreOutcome::ok};
  unsigned writes{0U};
};

class FakeLock final : public IMutationLock {
 public:
  LockOutcome acquire() override {
    if (busy) {
      return LockOutcome::busy;
    }
    return denied ? LockOutcome::access_denied : LockOutcome::acquired;
  }
  void release() noexcept override { releases += 1U; }

  bool busy{false};
  bool denied{false};
  unsigned releases{0U};
};

std::vector<std::uint8_t> from_hex(const std::string& value) {
  REQUIRE((value.size() % 2U) == 0U);
  std::vector<std::uint8_t> result;
  result.reserve(value.size() / 2U);
  for (std::size_t index = 0; index < value.size(); index += 2U) {
    result.push_back(static_cast<std::uint8_t>(std::stoul(value.substr(index, 2), nullptr, 16)));
  }
  return result;
}

nlohmann::json corpus() {
  std::ifstream stream(std::string(AXLIC_SOURCE_DIR) + "/reference/vectors/a2/install.json");
  REQUIRE(stream.good());
  return nlohmann::json::parse(stream);
}

struct Fixture {
  std::vector<axlic::wire::TrustedPublicKey> trusted_keys;
  DeviceReference device;
  std::vector<std::uint8_t> artifact(const char* id) const {
    const auto& entry = document.at("artifacts").at(id);
    return from_hex(entry.at("hex").get<std::string>());
  }
  nlohmann::json document;
};

Fixture fixture() {
  auto document = corpus();
  Fixture result{.document = std::move(document)};
  axlic::wire::TrustedPublicKey key{.key_id = result.document.at("key_id").get<std::string>()};
  const auto bytes = from_hex(result.document.at("public_key_xy_hex").get<std::string>());
  REQUIRE(bytes.size() == 64U);
  std::copy(bytes.begin(), bytes.end(), key.public_key_xy.begin());
  result.trusted_keys.push_back(std::move(key));
  const auto device = DeviceReference::parse(result.document.at("device_id").get<std::string>());
  REQUIRE(device.has_value());
  result.device = *device;
  return result;
}

InstallRequest request(const Fixture& source, const char* id) {
  return InstallRequest{.artifact = source.artifact(id)};
}

void install_ok(const Fixture& source, const char* id) {
  FakeStateStore store;
  store.exists = true;
  store.state.registration = RegistrationAssociation{.device_id = source.device};
  FakeLock lock;
  const auto result =
      axlic::core::install_credential(store, lock, source.trusted_keys, request(source, id));
  CHECK(result.code == InstallCode::ok);
  CHECK_FALSE(result.retryable);
}

}  // namespace

TEST_CASE("installation requires an existing registration association") {
  const auto source = fixture();
  FakeStateStore store;
  FakeLock lock;
  const auto result =
      axlic::core::install_credential(store, lock, source.trusted_keys, request(source, "v1"));
  CHECK(result.code == InstallCode::credential_device_mismatch);
  CHECK_FALSE(store.state.credential.has_value());
}

TEST_CASE("installation rejects an artifact bound to another device") {
  const auto source = fixture();
  FakeStateStore store;
  store.exists = true;
  store.state.registration = RegistrationAssociation{.device_id = source.device};
  FakeLock lock;
  const auto result =
      axlic::core::install_credential(store, lock, source.trusted_keys, request(source, "mismatch_device"));
  CHECK(result.code == InstallCode::credential_device_mismatch);
  CHECK_FALSE(store.state.credential.has_value());
}

TEST_CASE("installation commits the payload fields") {
  const auto source = fixture();
  install_ok(source, "v1");
}

TEST_CASE("installed credential carries payload identity and entitlements") {
  const auto source = fixture();
  FakeStateStore store;
  store.exists = true;
  store.state.registration = RegistrationAssociation{.device_id = source.device};
  FakeLock lock;
  REQUIRE(axlic::core::install_credential(
              store, lock, source.trusted_keys, request(source, "v1"))
              .code == InstallCode::ok);
  const auto& stored = store.state.credential;
  REQUIRE(stored.has_value());
  CHECK(stored->credential_id == "cred-a2-v1");
  CHECK(stored->authority_revision == 3U);
  CHECK(stored->credential_generation == 7U);
  CHECK(stored->device_id == source.device.to_string());
  REQUIRE(stored->entitlements.size() == 2U);
  CHECK(stored->entitlements[0].entitlement_id == "nearhub.runtime");
  CHECK(stored->entitlements[1].entitlement_id == "nearhub.seats");
  CHECK(lock.releases == 1U);
}

TEST_CASE("empty entitlements revocation credential installs legally") {
  const auto source = fixture();
  FakeStateStore store;
  store.exists = true;
  store.state.registration = RegistrationAssociation{.device_id = source.device};
  FakeLock lock;
  const auto result =
      axlic::core::install_credential(store, lock, source.trusted_keys, request(source, "clear"));
  CHECK(result.code == InstallCode::ok);
  REQUIRE(store.state.credential.has_value());
  CHECK(store.state.credential->entitlements.empty());
  const auto runtime = axlic::core::RuntimeLicense::from_installed(*store.state.credential);
  CHECK(runtime.status().credential_state == axlic::core::CredentialState::valid);
  CHECK_FALSE(runtime.entitlement("nearhub.runtime", 100).granted);
}

TEST_CASE("lower revision is rejected as stale regardless of generation") {
  const auto source = fixture();
  FakeStateStore store;
  store.exists = true;
  store.state.registration = RegistrationAssociation{.device_id = source.device};
  FakeLock lock;
  REQUIRE(axlic::core::install_credential(
              store, lock, source.trusted_keys, request(source, "v1"))
              .code == InstallCode::ok);
  const auto result = axlic::core::install_credential(
      store, lock, source.trusted_keys, request(source, "stale_lower_revision"));
  CHECK(result.code == InstallCode::credential_stale);
  CHECK(store.state.credential->credential_id == "cred-a2-v1");
}

TEST_CASE("same revision with lower generation is rejected as stale") {
  const auto source = fixture();
  FakeStateStore store;
  store.exists = true;
  store.state.registration = RegistrationAssociation{.device_id = source.device};
  FakeLock lock;
  REQUIRE(axlic::core::install_credential(
              store, lock, source.trusted_keys, request(source, "v1"))
              .code == InstallCode::ok);
  const auto result = axlic::core::install_credential(
      store, lock, source.trusted_keys, request(source, "stale_lower_generation"));
  CHECK(result.code == InstallCode::credential_stale);
}

TEST_CASE("same version reinstall is idempotent") {
  const auto source = fixture();
  FakeStateStore store;
  store.exists = true;
  store.state.registration = RegistrationAssociation{.device_id = source.device};
  FakeLock lock;
  REQUIRE(axlic::core::install_credential(
              store, lock, source.trusted_keys, request(source, "v1"))
              .code == InstallCode::ok);
  const auto committed = store.state;
  REQUIRE(store.writes == 1U);
  const auto result =
      axlic::core::install_credential(store, lock, source.trusted_keys, request(source, "v1"));
  CHECK(result.code == InstallCode::ok_idempotent);
  // The equal-version path is a reinstall: it commits again, now that there is
  // no channel value left to refresh.
  CHECK(store.writes == 2U);
  CHECK(store.state == committed);
}

TEST_CASE("higher revision and same-revision higher generation both install") {
  const auto source = fixture();
  FakeStateStore store;
  store.exists = true;
  store.state.registration = RegistrationAssociation{.device_id = source.device};
  FakeLock lock;
  REQUIRE(axlic::core::install_credential(
              store, lock, source.trusted_keys, request(source, "v1"))
              .code == InstallCode::ok);
  SECTION("same revision higher generation") {
    CHECK(axlic::core::install_credential(
              store, lock, source.trusted_keys, request(source, "same_rev_higher_gen"))
              .code == InstallCode::ok);
    CHECK(store.state.credential->credential_generation == 8U);
  }
  SECTION("higher revision") {
    CHECK(axlic::core::install_credential(
              store, lock, source.trusted_keys, request(source, "v2_new"))
              .code == InstallCode::ok);
    CHECK(store.state.credential->authority_revision == 4U);
  }
}

TEST_CASE("tampered signature is rejected as invalid") {
  const auto source = fixture();
  FakeStateStore store;
  store.exists = true;
  store.state.registration = RegistrationAssociation{.device_id = source.device};
  FakeLock lock;
  const auto result = axlic::core::install_credential(
      store, lock, source.trusted_keys, request(source, "invalid_signature"));
  CHECK(result.code == InstallCode::credential_invalid);
  CHECK_FALSE(store.state.credential.has_value());
}

TEST_CASE("empty trust keyring fails closed as unsupported") {
  const auto source = fixture();
  FakeStateStore store;
  store.exists = true;
  store.state.registration = RegistrationAssociation{.device_id = source.device};
  FakeLock lock;
  const std::vector<axlic::wire::TrustedPublicKey> empty;
  const auto result = axlic::core::install_credential(store, lock, empty, request(source, "v1"));
  CHECK(result.code == InstallCode::credential_unsupported);
  CHECK_FALSE(store.state.credential.has_value());
}

TEST_CASE("busy lock maps to a retryable result before any verification") {
  const auto source = fixture();
  FakeStateStore store;
  store.exists = true;
  store.state.registration = RegistrationAssociation{.device_id = source.device};
  FakeLock lock;
  lock.busy = true;
  const auto result =
      axlic::core::install_credential(store, lock, source.trusted_keys, request(source, "v1"));
  CHECK(result.code == InstallCode::local_state_busy);
  CHECK(result.retryable);
  CHECK(lock.releases == 0U);
}

TEST_CASE("access-denied lock maps to the privilege result") {
  const auto source = fixture();
  FakeStateStore store;
  store.exists = true;
  store.state.registration = RegistrationAssociation{.device_id = source.device};
  FakeLock lock;
  lock.denied = true;
  const auto result =
      axlic::core::install_credential(store, lock, source.trusted_keys, request(source, "v1"));
  CHECK(result.code == InstallCode::local_write_privilege_required);
  CHECK_FALSE(result.retryable);
  CHECK(lock.releases == 0U);
}

TEST_CASE("install result echoes the committed credential") {
  const auto source = fixture();
  FakeStateStore store;
  store.exists = true;
  store.state.registration = RegistrationAssociation{.device_id = source.device};
  FakeLock lock;
  const auto first =
      axlic::core::install_credential(store, lock, source.trusted_keys, request(source, "v1"));
  REQUIRE(first.code == InstallCode::ok);
  REQUIRE(first.installed.has_value());
  CHECK(first.installed->authority_revision == 3U);
  CHECK(first.installed->credential_generation == 7U);
  REQUIRE(store.state.credential.has_value());
  CHECK(*first.installed == *store.state.credential);

  // Idempotent same-version reinstall still echoes the committed credential
  // so the CLI response never re-reads state.
  const auto again =
      axlic::core::install_credential(store, lock, source.trusted_keys, request(source, "v1"));
  REQUIRE(again.code == InstallCode::ok_idempotent);
  REQUIRE(again.installed.has_value());
  CHECK(*again.installed == *store.state.credential);
}

TEST_CASE("held version reflects the stored pair or nothing") {
  const auto source = fixture();
  FakeStateStore store;
  store.exists = true;
  store.state.registration = RegistrationAssociation{.device_id = source.device};
  FakeLock lock;
  CHECK_FALSE(axlic::core::held_version(store.state).has_value());
  REQUIRE(axlic::core::install_credential(
              store, lock, source.trusted_keys, request(source, "v1"))
              .code == InstallCode::ok);
  const auto held = axlic::core::held_version(store.state);
  REQUIRE(held.has_value());
  CHECK(held->authority_revision == 3U);
  CHECK(held->credential_generation == 7U);
}

TEST_CASE("runtime license from installed credential answers entitlements offline") {
  const auto source = fixture();
  FakeStateStore store;
  store.exists = true;
  store.state.registration = RegistrationAssociation{.device_id = source.device};
  FakeLock lock;
  REQUIRE(axlic::core::install_credential(
              store, lock, source.trusted_keys, request(source, "v1"))
              .code == InstallCode::ok);
  const auto runtime = axlic::core::RuntimeLicense::from_installed(*store.state.credential);
  const auto status = runtime.status();
  CHECK(status.credential_state == axlic::core::CredentialState::valid);
  CHECK(status.authority_revision == 3U);
  CHECK(status.credential_generation == 7U);
  const auto granted = runtime.entitlement("nearhub.runtime", 100);
  CHECK(granted.granted);
  const auto bounded = runtime.entitlement("nearhub.seats", 15);
  CHECK(bounded.granted);
  CHECK(bounded.value_u64 == 5U);
  CHECK_FALSE(runtime.entitlement("nearhub.seats", 2000000001).granted);
  CHECK_FALSE(runtime.entitlement("missing.right", 100).granted);
}
