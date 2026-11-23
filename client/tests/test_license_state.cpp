#include "axlic/core/license_state.hpp"

#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <string>

namespace {

std::optional<axlic::core::DeviceReference> reference(const char digit) {
  return axlic::core::DeviceReference::parse(std::string("axldev_") + std::string(32, digit));
}

axlic::core::InstalledCredential credential(const std::uint64_t revision, const std::uint64_t generation) {
  return {
      .credential_id = "cred-1",
      .license_grant_id = "grant-1",
      .binding_id = "binding-1",
      .authority_revision = revision,
      .credential_generation = generation,
      .issued_at = 100,
      .entitlements = {},
      .policy_version = 1,
  };
}

}  // namespace

TEST_CASE("license state defaults to unregistered with no credential") {
  const axlic::core::LicenseState state;
  CHECK_FALSE(state.registration.has_value());
  CHECK_FALSE(state.credential.has_value());
}

TEST_CASE("registration and credential are independent halves") {
  axlic::core::LicenseState state;
  state.registration = axlic::core::RegistrationAssociation{.device_id = *reference('1')};
  CHECK(state.registration.has_value());
  CHECK_FALSE(state.credential.has_value());

  state.credential = credential(3, 7);
  CHECK(state.credential->authority_revision == 3U);
  CHECK(state.credential->credential_generation == 7U);
}

TEST_CASE("installed credential equality compares payload fields and policy version") {
  auto left = credential(3, 7);
  auto right = credential(3, 7);
  CHECK(left == right);
  right.policy_version = 2;
  CHECK_FALSE(left == right);
  right = credential(3, 8);
  CHECK_FALSE(left == right);
}

TEST_CASE("empty entitlements is a representable revocation credential") {
  auto revocation = credential(4, 9);
  CHECK(revocation.entitlements.empty());
  axlic::wire::Entitlement entitlement{
      .entitlement_id = "launcher.runtime",
      .product_id = "launcher",
  };
  axlic::core::LicenseState with_entries;
  with_entries.credential = revocation;
  with_entries.credential->entitlements.push_back(entitlement);
  CHECK(with_entries.credential->entitlements.size() == 1U);
  CHECK(revocation.entitlements.empty());
}
