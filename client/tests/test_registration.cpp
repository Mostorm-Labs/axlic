#include "axlic/core/license_state.hpp"
#include "axlic/core/registration.hpp"

#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <stdexcept>
#include <string>

namespace {

using axlic::core::DeviceReference;
using axlic::core::ILicenseStateStore;
using axlic::core::IMutationLock;
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
  StateStoreOutcome prepare_for_mutation() override { return fail_prepare; }
  StateStoreOutcome write(const LicenseState& value) override {
    writes += 1U;
    state = value;
    exists = true;
    return StateStoreOutcome::ok;
  }

  LicenseState state;
  bool exists{false};
  StateStoreOutcome fail_read{StateStoreOutcome::ok};
  StateStoreOutcome fail_prepare{StateStoreOutcome::ok};
  unsigned writes{0U};
};

class FakeLock final : public IMutationLock {
 public:
  LockOutcome acquire() override { return busy ? LockOutcome::busy : LockOutcome::acquired; }
  void release() noexcept override { releases += 1U; }

  bool busy{false};
  unsigned releases{0U};
};

DeviceReference reference(const char digit) {
  const auto parsed = DeviceReference::parse(std::string("axldev_") + std::string(32, digit));
  if (!parsed) {
    throw std::runtime_error("reference fixture");
  }
  return *parsed;
}

}  // namespace

TEST_CASE("self-report uses identity_value when unregistered") {
  const auto value = axlic::core::resolve_self_report(std::nullopt, std::string(128, 'a'));
  CHECK_FALSE(value.registered);
  CHECK(value.value == std::string(128, 'a'));
}

TEST_CASE("self-report uses the platform reference when registered") {
  const axlic::core::RegistrationAssociation association{.device_id = reference('7')};
  const auto value = axlic::core::resolve_self_report(association, std::string(128, 'a'));
  CHECK(value.registered);
  CHECK(value.value == reference('7').to_string());
}

TEST_CASE("adopt persists the reference and preserves the credential") {
  FakeStateStore store;
  store.exists = true;
  store.state.credential = axlic::core::InstalledCredential{
      .credential_id = "cred",
      .license_grant_id = "grant",
      .binding_id = "binding",
      .device_id = reference('7').to_string(),
      .authority_revision = 3,
      .credential_generation = 7,
      .issued_at = 1,
      .entitlements = {},
      .policy_version = 1,
  };
  FakeLock lock;
  const auto result = axlic::core::adopt_registration(store, lock, reference('7'));
  CHECK(result.code == axlic::core::RegistrationCode::ok);
  CHECK_FALSE(result.retryable);
  REQUIRE(store.state.registration.has_value());
  CHECK(store.state.registration->device_id == reference('7'));
  CHECK(store.state.credential.has_value());
  CHECK(lock.releases == 1U);
}

TEST_CASE("adopt on an absent state creates the registration alone") {
  FakeStateStore store;
  FakeLock lock;
  const auto result = axlic::core::adopt_registration(store, lock, reference('1'));
  CHECK(result.code == axlic::core::RegistrationCode::ok);
  REQUIRE(store.state.registration.has_value());
  CHECK_FALSE(store.state.credential.has_value());
}

TEST_CASE("reset drops the reference and preserves the credential") {
  FakeStateStore store;
  store.exists = true;
  store.state.registration = RegistrationAssociation{.device_id = reference('7')};
  store.state.credential = axlic::core::InstalledCredential{
      .credential_id = "cred",
      .license_grant_id = "grant",
      .binding_id = "binding",
      .device_id = reference('7').to_string(),
      .authority_revision = 3,
      .credential_generation = 7,
      .issued_at = 1,
      .entitlements = {},
      .policy_version = 1,
  };
  FakeLock lock;
  const auto result = axlic::core::reset_registration(store, lock);
  CHECK(result.code == axlic::core::RegistrationCode::ok);
  CHECK_FALSE(store.state.registration.has_value());
  CHECK(store.state.credential.has_value());
  // After reset the self-report falls back to identity_value.
  const auto value = axlic::core::resolve_self_report(store.state.registration, std::string(128, 'b'));
  CHECK_FALSE(value.registered);
  CHECK(value.value == std::string(128, 'b'));
}

TEST_CASE("busy lock maps to a retryable code without writing") {
  FakeStateStore store;
  FakeLock lock;
  lock.busy = true;
  const auto result = axlic::core::adopt_registration(store, lock, reference('7'));
  CHECK(result.code == axlic::core::RegistrationCode::local_state_busy);
  CHECK(result.retryable);
  CHECK(store.writes == 0U);
  CHECK(lock.releases == 0U);
}

TEST_CASE("corrupt state is reported without writing") {
  FakeStateStore store;
  store.fail_read = StateStoreOutcome::corrupt;
  FakeLock lock;
  const auto result = axlic::core::reset_registration(store, lock);
  CHECK(result.code == axlic::core::RegistrationCode::local_state_corrupt);
  CHECK_FALSE(result.retryable);
  CHECK(store.writes == 0U);
}
