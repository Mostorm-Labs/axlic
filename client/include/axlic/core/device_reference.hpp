#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace axlic::core {

// Platform-assigned device reference: "axldev_" + exactly 32 lowercase hex
// characters (39 total, 16 decoded bytes). Parsing is strict: uppercase hex,
// wrong length or wrong prefix are rejected, and to_string() round-trips the
// canonical text form byte-for-byte.
class DeviceReference {
 public:
  static constexpr std::size_t kPrefixLength = 7U;
  static constexpr std::size_t kHexLength = 32U;
  static constexpr std::size_t kTextLength = kPrefixLength + kHexLength;
  static constexpr std::size_t kByteLength = 16U;

  [[nodiscard]] static std::optional<DeviceReference> parse(std::string_view text);
  [[nodiscard]] std::string to_string() const;

  std::array<std::uint8_t, kByteLength> bytes{};

  bool operator==(const DeviceReference&) const = default;
};

}  // namespace axlic::core
