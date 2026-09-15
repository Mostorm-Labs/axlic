#include "axlic/windows/machine_state.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace {

using axlic::core::IdentityRecord;
using axlic::core::ProviderKind;
using axlic::core::StateStoreOutcome;
using axlic::windows::IDataProtector;
using axlic::windows::MachineStateCodec;
using axlic::windows::ProtectionOutcome;
using axlic::windows::ProtectionResult;

IdentityRecord software_record() {
  return {
      .scheme_id = "axl-win-cng-software-p256-v1",
      .identity_epoch = 1,
      .identity_value = std::string(128, '1'),
      .provider_kind = ProviderKind::software,
      .provider_name = "Microsoft Software Key Storage Provider",
      .key_name = "Auditoryworks.AxLicense.Identity.v1.00112233445566778899aabbccddeeff",
  };
}

std::string hex(const std::vector<std::uint8_t>& bytes) {
  constexpr char digits[] = "0123456789abcdef";
  std::string result;
  result.reserve(bytes.size() * 2U);
  for (const auto byte : bytes) {
    result.push_back(digits[byte >> 4U]);
    result.push_back(digits[byte & 0x0fU]);
  }
  return result;
}

class ReversingProtector final : public IDataProtector {
 public:
  ProtectionResult protect(const std::vector<std::uint8_t>& plaintext) override {
    auto value = plaintext;
    std::reverse(value.begin(), value.end());
    value.insert(value.begin(), 0xa5U);
    return {ProtectionOutcome::ok, std::move(value)};
  }
  ProtectionResult unprotect(const std::vector<std::uint8_t>& protected_bytes) override {
    if (protected_bytes.empty() || protected_bytes.front() != 0xa5U) {
      return {ProtectionOutcome::corrupt, {}};
    }
    std::vector<std::uint8_t> value(protected_bytes.begin() + 1, protected_bytes.end());
    std::reverse(value.begin(), value.end());
    return {ProtectionOutcome::ok, std::move(value)};
  }
};

}  // namespace

TEST_CASE("protected machine state round-trips the frozen A1 identity schema") {
  ReversingProtector protector;
  MachineStateCodec codec(protector);
  const auto expected = software_record();

  const auto encoded = codec.encode(expected);
  REQUIRE(encoded.outcome == StateStoreOutcome::ok);
  REQUIRE(encoded.carrier.find(expected.key_name) == std::string::npos);
  const auto decoded = codec.decode(encoded.carrier);

  REQUIRE(decoded.outcome == StateStoreOutcome::ok);
  REQUIRE(decoded.identity == expected);
}

TEST_CASE("tampered protected bytes fail closed as corrupt") {
  ReversingProtector protector;
  MachineStateCodec codec(protector);
  auto encoded = codec.encode(software_record()).carrier;
  const auto position = encoded.find("a5");
  REQUIRE(position != std::string::npos);
  encoded.replace(position, 2, "a4");

  REQUIRE(codec.decode(encoded).outcome == StateStoreOutcome::corrupt);
}

TEST_CASE("malformed odd and non-hex state envelopes fail closed") {
  ReversingProtector protector;
  MachineStateCodec codec(protector);

  REQUIRE(codec.decode("not-json").outcome == StateStoreOutcome::corrupt);
  REQUIRE(codec.decode(R"({"format":"axlicense-machine-state","envelope_version":1,"protected_blob_hex":"a"})")
              .outcome == StateStoreOutcome::corrupt);
  REQUIRE(codec.decode(R"({"format":"axlicense-machine-state","envelope_version":1,"protected_blob_hex":"zz"})")
              .outcome == StateStoreOutcome::corrupt);
}

TEST_CASE("oversized state is rejected before invoking protection") {
  class CountingProtector final : public IDataProtector {
   public:
    ProtectionResult protect(const std::vector<std::uint8_t>&) override { return {}; }
    ProtectionResult unprotect(const std::vector<std::uint8_t>&) override {
      ++calls;
      return {};
    }
    int calls{};
  } protector;
  MachineStateCodec codec(protector);

  const auto result = codec.decode(std::string(MachineStateCodec::maximum_carrier_size + 1U, 'x'));

  REQUIRE(result.outcome == StateStoreOutcome::corrupt);
  REQUIRE(protector.calls == 0);
}

TEST_CASE("future envelope and plaintext schema versions are unsupported") {
  ReversingProtector protector;
  MachineStateCodec codec(protector);
  const auto future_envelope =
      R"({"format":"axlicense-machine-state","envelope_version":2,"protected_blob_hex":"a500"})";
  REQUIRE(codec.decode(future_envelope).outcome == StateStoreOutcome::unsupported);

  const auto future_plaintext = std::string(
      R"({"schema_version":{"major":2,"minor":0},"state_generation":1,"identity":{"scheme_id":"axl-win-cng-software-p256-v1","identity_epoch":1,"identity_value":")") +
      std::string(128, '1') +
      R"(","provider_kind":"software","provider_name":"Microsoft Software Key Storage Provider","key_name":"Auditoryworks.AxLicense.Identity.v1.00112233445566778899aabbccddeeff"}})";
  std::vector<std::uint8_t> bytes(future_plaintext.begin(), future_plaintext.end());
  const auto protected_future = protector.protect(bytes).bytes;
  const auto carrier = std::string(
                           R"({"format":"axlicense-machine-state","envelope_version":1,"protected_blob_hex":")") +
                       hex(protected_future) + "\"}";
  REQUIRE(codec.decode(carrier).outcome == StateStoreOutcome::unsupported);
}

TEST_CASE("DPAPI machine-scope protection round-trips bytes and rejects modification") {
  axlic::windows::DpapiProtector protector;
  const std::vector<std::uint8_t> plaintext{'a', '1', '-', 'd', 'p', 'a', 'p', 'i'};
  const auto protected_value = protector.protect(plaintext);
  REQUIRE(protected_value.outcome == ProtectionOutcome::ok);
  REQUIRE(protected_value.bytes != plaintext);
  REQUIRE(protector.unprotect(protected_value.bytes).bytes == plaintext);

  auto tampered = protected_value.bytes;
  tampered[tampered.size() / 2U] ^= 0x01U;
  REQUIRE(protector.unprotect(tampered).outcome == ProtectionOutcome::corrupt);
}
