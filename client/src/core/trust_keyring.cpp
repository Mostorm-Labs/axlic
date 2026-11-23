#include "axlic/core/trust_keyring.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <limits>
#include <string>
#include <unordered_map>
#include <utility>

namespace axlic::core {
namespace {

constexpr std::string_view kFloorFormat = "axlic-trust-floor";
constexpr std::string_view kOverlayFormat = "axlic-trust-keyring-append";
constexpr std::size_t kMaximumDocumentSize = 1024U * 1024U;
constexpr std::size_t kMaximumKeyIdSize = 1024U;
constexpr std::size_t kPublicKeyHexLength = 128U;

bool is_lower_hex(const std::string_view value) {
  return std::ranges::all_of(value, [](const char character) {
    return (character >= '0' && character <= '9') || (character >= 'a' && character <= 'f');
  });
}

std::array<std::uint8_t, 64> decode_public_key(const std::string_view hex_digits) {
  const auto nibble = [](const char character) -> std::uint8_t {
    return character <= '9' ? static_cast<std::uint8_t>(character - '0')
                            : static_cast<std::uint8_t>(character - 'a' + 10);
  };
  std::array<std::uint8_t, 64> bytes{};
  for (std::size_t index = 0; index < kPublicKeyHexLength; index += 2U) {
    bytes[index / 2U] =
        static_cast<std::uint8_t>((nibble(hex_digits[index]) << 4U) | nibble(hex_digits[index + 1U]));
  }
  return bytes;
}

bool read_overlay_document(const std::filesystem::path& overlay_path, std::string& document) {
  std::error_code error;
  const auto status = std::filesystem::status(overlay_path, error);
  if (error || !std::filesystem::exists(status)) {
    return false;
  }
  const auto size = std::filesystem::file_size(overlay_path, error);
  if (error || size > kMaximumDocumentSize) {
    document.clear();
    return true;
  }
  std::ifstream stream(overlay_path, std::ios::binary);
  if (!stream) {
    document.clear();
    return true;
  }
  document.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
  if (document.size() > kMaximumDocumentSize) {
    document.clear();
  }
  return true;
}

}  // namespace

std::optional<std::vector<wire::TrustedPublicKey>> parse_trust_keyring_document(
    const std::string_view document,
    const std::string_view expected_format) {
  if (document.empty() || document.size() > kMaximumDocumentSize) {
    return std::nullopt;
  }
  try {
    const auto parsed = nlohmann::json::parse(document);
    if (!parsed.is_object() || parsed.value("format", "") != expected_format) {
      return std::nullopt;
    }
    if (!parsed.contains("version") || !parsed["version"].is_number_unsigned() ||
        parsed["version"].get<std::uint64_t>() != 1U) {
      return std::nullopt;
    }
    const auto& keys = parsed.at("keys");
    if (!keys.is_array()) {
      return std::nullopt;
    }
    std::vector<wire::TrustedPublicKey> result;
    result.reserve(keys.size());
    for (const auto& entry : keys) {
      if (!entry.is_object() || !entry.contains("key_id") || !entry["key_id"].is_string() ||
          !entry.contains("public_key_xy_hex") || !entry["public_key_xy_hex"].is_string()) {
        return std::nullopt;
      }
      const auto key_id = entry["key_id"].get<std::string>();
      const auto public_key_hex = entry["public_key_xy_hex"].get<std::string>();
      if (key_id.empty() || key_id.size() > kMaximumKeyIdSize ||
          public_key_hex.size() != kPublicKeyHexLength || !is_lower_hex(public_key_hex)) {
        return std::nullopt;
      }
      result.push_back(wire::TrustedPublicKey{
          .key_id = key_id,
          .public_key_xy = decode_public_key(public_key_hex),
      });
    }
    return result;
  } catch (const nlohmann::json::exception&) {
    return std::nullopt;
  }
}

KeyringRead load_trust_keyring_from(
    const std::string_view floor_document,
    const std::filesystem::path& state_root) {
  KeyringRead read;
  std::unordered_map<std::string, wire::TrustedPublicKey> merged;
  if (const auto floor = parse_trust_keyring_document(floor_document, kFloorFormat)) {
    for (auto& entry : *floor) {
      merged.insert_or_assign(entry.key_id, std::move(entry));
    }
  }
  std::string overlay_document;
  if (read_overlay_document(state_root / "trust-keyring.v1.json", overlay_document)) {
    if (const auto overlay = parse_trust_keyring_document(overlay_document, kOverlayFormat)) {
      for (auto& entry : *overlay) {
        merged.insert_or_assign(entry.key_id, std::move(entry));
      }
    } else {
      read.overlay_ignored = true;
    }
  }
  read.keys.reserve(merged.size());
  for (auto& [_, entry] : merged) {
    read.keys.push_back(std::move(entry));
  }
  std::ranges::sort(read.keys, [](const auto& left, const auto& right) {
    return left.key_id < right.key_id;
  });
  return read;
}

}  // namespace axlic::core
