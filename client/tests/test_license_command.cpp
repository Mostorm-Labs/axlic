#include "axlic/core/license_command.hpp"
#include "axlic/core/license_state.hpp"

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>

namespace {

using axlic::core::ILicenseStateStore;
using axlic::core::IMutationLock;
using axlic::core::InstalledCredential;
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
    return StateStoreOutcome::ok;
  }

  LicenseState state;
  bool exists{false};
  StateStoreOutcome fail_read{StateStoreOutcome::ok};
};

class FakeLock final : public IMutationLock {
 public:
  LockOutcome acquire() override { return busy ? LockOutcome::busy : LockOutcome::acquired; }
  void release() noexcept override {}

  bool busy{false};
};

class FakeMachineStore final : public axlic::core::IMachineStateStore {
 public:
  axlic::core::StateRead read() override {
    if (fail_read != StateStoreOutcome::ok) {
      return {fail_read, std::nullopt};
    }
    axlic::core::StateRead result{exists ? StateStoreOutcome::ok : StateStoreOutcome::absent, std::nullopt};
    if (exists) {
      result.identity = identity;
    }
    return result;
  }
  StateStoreOutcome prepare_for_mutation() override { return StateStoreOutcome::ok; }
  StateStoreOutcome write(const axlic::core::IdentityRecord&) override { return StateStoreOutcome::ok; }

  axlic::core::IdentityRecord identity;
  bool exists{false};
  StateStoreOutcome fail_read{StateStoreOutcome::ok};
};

axlic::core::DeviceReference reference(const char digit) {
  const auto parsed =
      axlic::core::DeviceReference::parse(std::string("axldev_") + std::string(32, digit));
  if (!parsed) {
    throw std::runtime_error("reference fixture");
  }
  return *parsed;
}

InstalledCredential installed() {
  return InstalledCredential{
      .credential_id = "cred-1",
      .license_grant_id = "grant-1",
      .binding_id = "binding-1",
      .device_id = reference('7').to_string(),
      .authority_revision = 3,
      .credential_generation = 7,
      .issued_at = 100,
      .entitlements = {},
      .policy_version = 2,
  };
}

nlohmann::json parse(const axlic::core::RenderedCommand& rendered) {
  return nlohmann::json::parse(rendered.json);
}

}  // namespace

TEST_CASE("status reports absent when no credential is installed") {
  FakeStateStore store;
  const auto rendered = axlic::core::render_status(store);
  const auto document = parse(rendered);
  CHECK(rendered.exit_code == 3);
  CHECK(document.at("code") == "CREDENTIAL_NOT_FOUND");
  CHECK(document.at("data").at("credential_state") == "absent");
}

TEST_CASE("status reports the installed credential version pair") {
  FakeStateStore store;
  store.exists = true;
  store.state.credential = installed();
  const auto rendered = axlic::core::render_status(store);
  const auto document = parse(rendered);
  CHECK(rendered.exit_code == 0);
  CHECK(document.at("code") == "OK");
  CHECK(document.at("data").at("credential_state") == "valid");
  CHECK(document.at("data").at("authority_revision") == 3U);
  CHECK(document.at("data").at("credential_generation") == 7U);
}

TEST_CASE("entitlement reports granted from the installed credential") {
  FakeStateStore store;
  store.exists = true;
  store.state.credential = installed();
  store.state.credential->entitlements.push_back(axlic::wire::Entitlement{
      .entitlement_id = "nearhub.runtime",
      .product_id = "nearhub",
  });
  const auto granted = axlic::core::render_entitlement(store, "nearhub.runtime", 150);
  CHECK(granted.exit_code == 0);
  CHECK(parse(granted).at("data").at("granted") == true);
  const auto denied = axlic::core::render_entitlement(store, "missing.right", 150);
  CHECK(denied.exit_code == 0);
  CHECK(parse(denied).at("data").at("granted") == false);
}

TEST_CASE("device-register reports the dual-state self-report value") {
  FakeStateStore license_store;
  FakeMachineStore machine_store;
  machine_store.exists = true;
  machine_store.identity = axlic::core::IdentityRecord{
      .scheme_id = "axl-win-cng-software-p256-v1",
      .identity_epoch = 1,
      .identity_value = std::string(128, 'c'),
      .provider_kind = axlic::core::ProviderKind::software,
      .provider_name = "Microsoft Software Key Storage Provider",
      .key_name = std::string("Auditoryworks.AxLicense.Identity.v1.") + std::string(32, 'c'),
  };
  const auto unregistered = axlic::core::render_device_register(machine_store, license_store);
  CHECK(unregistered.exit_code == 0);
  const auto document = parse(unregistered);
  CHECK(document.at("data").at("registered") == false);
  CHECK(document.at("data").at("self_report_value") == std::string(128, 'c'));

  license_store.exists = true;
  license_store.state.registration = RegistrationAssociation{.device_id = reference('7')};
  const auto registered = axlic::core::render_device_register(machine_store, license_store);
  const auto registered_document = parse(registered);
  CHECK(registered_document.at("data").at("registered") == true);
  CHECK(registered_document.at("data").at("self_report_value") == reference('7').to_string());
  CHECK(registered_document.at("data").at("device_id") == reference('7').to_string());
}

TEST_CASE("device-register adopt validates the reference format") {
  FakeStateStore store;
  FakeLock lock;
  const auto invalid = axlic::core::render_device_register_adopt(store, lock, "not-a-reference");
  CHECK(invalid.exit_code == 2);
  CHECK(parse(invalid).at("code") == "COMMAND_INVALID");

  const auto valid = axlic::core::render_device_register_adopt(store, lock, reference('7').to_string());
  CHECK(valid.exit_code == 0);
  CHECK(parse(valid).at("data").at("device_id") == reference('7').to_string());
  REQUIRE(store.state.registration.has_value());
  CHECK(store.state.registration->device_id == reference('7'));
}

TEST_CASE("held-version reports the camelCase triple or no credential") {
  FakeStateStore store;
  const auto empty = axlic::core::render_held_version(store);
  CHECK(empty.exit_code == 0);
  CHECK(parse(empty).at("data").at("hasCredential") == false);

  store.exists = true;
  store.state.credential = installed();
  const auto held = axlic::core::render_held_version(store);
  const auto document = parse(held);
  CHECK(held.exit_code == 0);
  CHECK(document.at("data").at("hasCredential") == true);
  const auto& version = document.at("data").at("version");
  CHECK(version.at("authorityRevision") == 3U);
  CHECK(version.at("credentialGeneration") == 7U);
  CHECK(version.at("policyVersion") == 2U);
}

TEST_CASE("corrupt license state maps to the local-state failure family") {
  FakeStateStore store;
  store.fail_read = StateStoreOutcome::corrupt;
  const auto rendered = axlic::core::render_status(store);
  CHECK(rendered.exit_code == 3);
  CHECK(parse(rendered).at("code") == "LOCAL_STATE_CORRUPT");
}
