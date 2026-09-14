#include "axlic/core/runtime_license.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {

axlic::wire::Credential credential() {
  axlic::wire::Credential value;
  value.credential_id = "cred-a0";
  value.license_grant_id = "grant-a0";
  value.authority_revision = 8;
  value.binding_id = "binding-a0";
  value.device_id = "device-a0";
  value.credential_generation = 4;
  value.issued_at = 1767225600;
  value.entitlements = {
      {.entitlement_id = "nearhub.runtime",
       .product_id = "nearhub",
       .right_kind = axlic::wire::RightKind::runtime,
       .grant_semantics = axlic::wire::GrantSemantics::presence,
       .validity = {.kind = axlic::wire::ValidityKind::perpetual}},
      {.entitlement_id = "nearhub.concurrent_sources",
       .product_id = "nearhub",
       .right_kind = axlic::wire::RightKind::runtime,
       .grant_semantics = axlic::wire::GrantSemantics::bounded_u64,
       .validity = {.kind = axlic::wire::ValidityKind::bounded,
                    .not_before = 1767225600,
                    .not_after = 4102444800},
       .max_u64 = 4},
  };
  return value;
}

}  // namespace

TEST_CASE("absent credential produces an absent status and denies entitlements") {
  const auto runtime = axlic::core::RuntimeLicense::absent();
  REQUIRE(runtime.status().credential_state == axlic::core::CredentialState::absent);
  REQUIRE_FALSE(runtime.entitlement("nearhub.runtime", 2000000000).granted);
}

TEST_CASE("valid credential exposes revision and exact entitlement values") {
  const auto runtime = axlic::core::RuntimeLicense::from_verification(
      {.status = axlic::wire::VerificationStatus::valid, .credential = credential()});
  const auto status = runtime.status();
  REQUIRE(status.credential_state == axlic::core::CredentialState::valid);
  REQUIRE(status.authority_revision == 8);
  REQUIRE(status.credential_generation == 4);
  REQUIRE(runtime.entitlement("nearhub.runtime", 2000000000).granted);
  const auto bounded = runtime.entitlement("nearhub.concurrent_sources", 2000000000);
  REQUIRE(bounded.granted);
  REQUIRE(bounded.value_u64 == 4);
  REQUIRE_FALSE(runtime.entitlement("nearhub.unknown", 2000000000).granted);
}

TEST_CASE("bounded validity uses the frozen half-open interval") {
  const auto runtime = axlic::core::RuntimeLicense::from_verification(
      {.status = axlic::wire::VerificationStatus::valid, .credential = credential()});
  REQUIRE_FALSE(runtime.entitlement("nearhub.concurrent_sources", 1767225599).granted);
  REQUIRE(runtime.entitlement("nearhub.concurrent_sources", 1767225600).granted);
  REQUIRE_FALSE(runtime.entitlement("nearhub.concurrent_sources", 4102444800).granted);
}

TEST_CASE("failed verification remains fail closed") {
  const auto invalid = axlic::core::RuntimeLicense::from_verification(
      {.status = axlic::wire::VerificationStatus::invalid});
  const auto unsupported = axlic::core::RuntimeLicense::from_verification(
      {.status = axlic::wire::VerificationStatus::unsupported});
  REQUIRE(invalid.status().credential_state == axlic::core::CredentialState::invalid);
  REQUIRE(unsupported.status().credential_state == axlic::core::CredentialState::unsupported);
  REQUIRE_FALSE(invalid.entitlement("nearhub.runtime", 2000000000).granted);
  REQUIRE_FALSE(unsupported.entitlement("nearhub.runtime", 2000000000).granted);
}
