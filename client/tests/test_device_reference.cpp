#include "axlic/core/device_reference.hpp"

#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <string>

namespace {

const axlic::core::DeviceReference sample_from_hex(const std::string& hex_digits) {
  const auto parsed = axlic::core::DeviceReference::parse("axldev_" + hex_digits);
  REQUIRE(parsed.has_value());
  return *parsed;
}

}  // namespace

TEST_CASE("device reference accepts canonical text and round-trips") {
  const std::string canonical = "axldev_0123456789abcdef0123456789abcdef";
  const auto parsed = axlic::core::DeviceReference::parse(canonical);
  REQUIRE(parsed.has_value());
  CHECK(parsed->to_string() == canonical);
}

TEST_CASE("device reference rejects uppercase hex") {
  CHECK_FALSE(axlic::core::DeviceReference::parse("axldev_0123456789ABCDEF0123456789ABCDEF").has_value());
}

TEST_CASE("device reference rejects wrong length") {
  CHECK_FALSE(axlic::core::DeviceReference::parse("axldev_0123456789abcdef").has_value());
  CHECK_FALSE(axlic::core::DeviceReference::parse("axldev_").has_value());
  CHECK_FALSE(
      axlic::core::DeviceReference::parse("axldev_0123456789abcdef0123456789abcdeff").has_value());
}

TEST_CASE("device reference rejects wrong prefix and non-hex") {
  CHECK_FALSE(axlic::core::DeviceReference::parse("AXLDEV_0123456789abcdef0123456789abcdef").has_value());
  CHECK_FALSE(axlic::core::DeviceReference::parse("axldevz0123456789abcdef0123456789abcde").has_value());
  CHECK_FALSE(axlic::core::DeviceReference::parse("").has_value());
}

TEST_CASE("device reference compares bytes not formatting") {
  const auto left = sample_from_hex("000000000000000000000000000000ff");
  const auto right = sample_from_hex("000000000000000000000000000000ff");
  const auto other = sample_from_hex("000000000000000000000000000000fe");
  CHECK(left == right);
  CHECK_FALSE(left == other);
}

TEST_CASE("device reference hex digit order maps to byte order") {
  const auto parsed = sample_from_hex("0102030405060708090a0b0c0d0e0f10");
  CHECK(parsed.bytes[0] == 0x01U);
  CHECK(parsed.bytes[15] == 0x10U);
}
