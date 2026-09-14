#include "axlic/wire/credential.hpp"

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <span>
#include <string>
#include <vector>

namespace {

std::string to_hex(const std::vector<std::uint8_t>& bytes) {
  constexpr char digits[] = "0123456789abcdef";
  std::string result;
  result.reserve(bytes.size() * 2U);
  for (const auto byte : bytes) {
    result.push_back(digits[(byte >> 4U) & 0x0fU]);
    result.push_back(digits[byte & 0x0fU]);
  }
  return result;
}

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
  std::ifstream stream(std::string(AXLIC_SOURCE_DIR) + "/reference/vectors/a0/corpus.json");
  REQUIRE(stream.good());
  return nlohmann::json::parse(stream);
}

std::array<std::uint8_t, 64> public_key(const std::string& hex) {
  const auto bytes = from_hex(hex);
  REQUIRE(bytes.size() == 64U);
  std::array<std::uint8_t, 64> result{};
  std::copy(bytes.begin(), bytes.end(), result.begin());
  return result;
}

axlic::wire::Credential perpetual_credential() {
  axlic::wire::Credential credential;
  credential.credential_id = "cred-a0";
  credential.license_grant_id = "grant-a0";
  credential.authority_revision = 7;
  credential.binding_id = "binding-a0";
  credential.device_id = "device-a0";
  credential.credential_generation = 3;
  credential.issued_at = 1767225600;
  credential.entitlements.push_back({
      .entitlement_id = "nearhub.runtime",
      .product_id = "nearhub",
      .right_kind = axlic::wire::RightKind::runtime,
      .grant_semantics = axlic::wire::GrantSemantics::presence,
      .validity = {.kind = axlic::wire::ValidityKind::perpetual},
  });
  return credential;
}

}  // namespace

TEST_CASE("credential payload uses the frozen P17 integer field map") {
  const auto actual = axlic::wire::encode_credential_payload(perpetual_credential());
  const std::string expected =
      "a9018201000267637265642d613003686772616e742d61300407056a62696e64696e"
      "672d613006696465766963652d61300703081a6955b9000a81a5016f6e6561726875"
      "622e72756e74696d6502676e6561726875620301040105a10101";
  REQUIRE(to_hex(actual) == expected);
}

TEST_CASE("credential payload ordering is independent of entitlement input order") {
  axlic::wire::Credential credential;
  credential.credential_id = "cred-a0-bounded";
  credential.license_grant_id = "grant-a0";
  credential.authority_revision = 8;
  credential.binding_id = "binding-a0";
  credential.device_id = "device-a0";
  credential.credential_generation = 4;
  credential.issued_at = 1767225600;
  credential.supersedes_credential_id = "cred-a0-perpetual";
  credential.entitlements = {
      {.entitlement_id = "nearhub.runtime",
       .product_id = "nearhub",
       .right_kind = axlic::wire::RightKind::runtime,
       .grant_semantics = axlic::wire::GrantSemantics::presence,
       .validity = {.kind = axlic::wire::ValidityKind::perpetual, .not_before = 1767225600}},
      {.entitlement_id = "nearhub.concurrent_sources",
       .product_id = "nearhub",
       .right_kind = axlic::wire::RightKind::runtime,
       .grant_semantics = axlic::wire::GrantSemantics::bounded_u64,
       .validity = {.kind = axlic::wire::ValidityKind::bounded,
                    .not_before = 1767225600,
                    .not_after = 4102444800},
       .max_u64 = 4},
  };
  const auto expected = corpus()["positives"][1]["payload_hex"].get<std::string>();
  REQUIRE(to_hex(axlic::wire::encode_credential_payload(credential)) == expected);
  std::reverse(credential.entitlements.begin(), credential.entitlements.end());
  REQUIRE(to_hex(axlic::wire::encode_credential_payload(credential)) == expected);
}

TEST_CASE("credential signing input matches the frozen P17 domain separation") {
  const auto vectors = corpus();
  const auto vector = vectors["positives"][0];
  const auto payload = from_hex(vector["payload_hex"].get<std::string>());
  const auto actual = axlic::wire::make_credential_signing_input(
      axlic::wire::kAlgorithmId, vectors["key_id"].get<std::string>(), payload);
  REQUIRE(to_hex(actual) == vector["signing_input_hex"].get<std::string>());
}

TEST_CASE("production verifier accepts every frozen positive vector") {
  const auto vectors = corpus();
  const axlic::wire::TrustedPublicKey key{
      .key_id = vectors["key_id"].get<std::string>(),
      .public_key_xy = public_key(vectors["public_key_xy_hex"].get<std::string>()),
  };
  for (const auto& vector : vectors["positives"]) {
    DYNAMIC_SECTION(vector["id"].get<std::string>()) {
      const auto artifact = from_hex(vector["artifact_hex"].get<std::string>());
      const auto result = axlic::wire::verify_credential(artifact, std::span{&key, 1U});
      REQUIRE(result.status == axlic::wire::VerificationStatus::valid);
      REQUIRE(result.credential.has_value());
    }
  }
}

TEST_CASE("production verifier fails closed for every frozen negative vector") {
  const auto vectors = corpus();
  for (const auto& vector : vectors["negatives"]) {
    DYNAMIC_SECTION(vector["id"].get<std::string>()) {
      const auto key_hex = vector.contains("trusted_public_key_xy_hex")
                               ? vector["trusted_public_key_xy_hex"].get<std::string>()
                               : vectors["public_key_xy_hex"].get<std::string>();
      const axlic::wire::TrustedPublicKey key{
          .key_id = vectors["key_id"].get<std::string>(),
          .public_key_xy = public_key(key_hex),
      };
      const auto artifact = from_hex(vector["artifact_hex"].get<std::string>());
      const auto result = axlic::wire::verify_credential(artifact, std::span{&key, 1U});
      const auto expected = vector["expected_status"].get<std::string>();
      const auto expected_status = expected == "unsupported"
                                       ? axlic::wire::VerificationStatus::unsupported
                                       : axlic::wire::VerificationStatus::invalid;
      REQUIRE(result.status == expected_status);
      REQUIRE_FALSE(result.credential.has_value());
    }
  }
}
