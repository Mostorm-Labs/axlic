#include "axlic/core/device_reference.hpp"

#include <algorithm>
#include <array>

namespace axlic::core {
namespace {

constexpr std::string_view kPrefix = "axldev_";

std::uint8_t nibble(const char character) {
  return character <= '9' ? static_cast<std::uint8_t>(character - '0')
                          : static_cast<std::uint8_t>(character - 'a' + 10);
}

bool is_lower_hex(const std::string_view value) {
  return std::ranges::all_of(value, [](const char character) {
    return (character >= '0' && character <= '9') || (character >= 'a' && character <= 'f');
  });
}

}  // namespace

std::optional<DeviceReference> DeviceReference::parse(const std::string_view text) {
  if (text.size() != kTextLength || !text.starts_with(kPrefix)) {
    return std::nullopt;
  }
  const auto digits = text.substr(kPrefixLength);
  if (!is_lower_hex(digits)) {
    return std::nullopt;
  }
  DeviceReference reference;
  for (std::size_t index = 0; index < kHexLength; index += 2U) {
    reference.bytes[index / 2U] =
        static_cast<std::uint8_t>((nibble(digits[index]) << 4U) | nibble(digits[index + 1U]));
  }
  return reference;
}

std::string DeviceReference::to_string() const {
  constexpr std::array<char, 16> digits{'0', '1', '2', '3', '4', '5', '6', '7', '8',
                                        '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  std::string result(kPrefix);
  result.reserve(kTextLength);
  for (const auto byte : bytes) {
    result.push_back(digits[byte >> 4U]);
    result.push_back(digits[byte & 0x0fU]);
  }
  return result;
}

}  // namespace axlic::core
