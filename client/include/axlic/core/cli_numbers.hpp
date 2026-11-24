#pragma once

#include <cstdint>
#include <fstream>
#include <iterator>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace axlic::core {

// Maximum artifact size accepted on any CLI surface before wire-level
// verification (mirrors the wire::verify_credential limit).
constexpr std::size_t kMaximumArtifactSize = 64U * 1024U;

// Strict decimal u64 parsing shared by the production CLI and the contract
// scenario executable so both surfaces reject the same inputs: digits only,
// overflow-checked (a 20-digit UINT64_MAX stays legal), and zero optionally
// rejected (an epoch "now" of zero is legal, a policy version is not).
[[nodiscard]] inline std::optional<std::uint64_t> parse_u64_strict(
    const std::string_view text, const bool allow_zero) {
  if (text.empty() || text.size() > 20U) {
    return std::nullopt;
  }
  std::uint64_t value{};
  for (const char character : text) {
    if (character < '0' || character > '9') {
      return std::nullopt;
    }
    const auto digit = static_cast<std::uint64_t>(character - '0');
    if (value > (std::numeric_limits<std::uint64_t>::max() - digit) / 10U) {
      return std::nullopt;
    }
    value = value * 10U + digit;
  }
  if (value == 0U && !allow_zero) {
    return std::nullopt;
  }
  return value;
}

// Reads an install artifact (non-empty, size-bounded) as owned bytes. Shared
// by the production CLI and the scenario executable so unreadable/empty/
// oversize artifacts classify identically on both surfaces.
[[nodiscard]] inline std::optional<std::vector<std::uint8_t>> read_artifact_file(const std::string& path) {
  std::ifstream stream(path, std::ios::binary);
  if (!stream) {
    return std::nullopt;
  }
  std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(stream),
                                  std::istreambuf_iterator<char>()};
  if (bytes.empty() || bytes.size() > kMaximumArtifactSize) {
    return std::nullopt;
  }
  return bytes;
}

}  // namespace axlic::core
