#pragma once

#include "axlic/wire/credential.hpp"

#include <filesystem>
#include <optional>
#include <string_view>
#include <vector>

namespace axlic::core {

// Embedded production trust floor, generated at configure time from
// client/keyring/trust-floor.json. The checked-in artifact starts empty:
// an empty merged keyring is legal and fail-closed (nothing verifies).
[[nodiscard]] std::string_view embedded_trust_floor_json() noexcept;

struct KeyringRead {
  // The state-root overlay existed but could not be parsed; it is ignored and
  // only the floor contributes trust (trust-loss direction, never trust gain).
  bool overlay_ignored{false};
  std::vector<wire::TrustedPublicKey> keys;
};

// Merged trust set = embedded floor plus the state-root overlay file
// (trust-keyring.v1.json). An overlay entry with the same key_id overrides
// the floor entry (key rotation replacement). A missing overlay leaves the
// floor; a malformed overlay is ignored. Entries must be
// {key_id, public_key_xy_hex = exactly 128 lowercase hex chars}.
[[nodiscard]] KeyringRead load_trust_keyring(const std::filesystem::path& state_root);

// Injection point for tests and tooling: same merge semantics over an
// explicit floor document instead of the embedded one.
[[nodiscard]] KeyringRead load_trust_keyring_from(
    std::string_view floor_document,
    const std::filesystem::path& state_root);

// Parses one floor/overlay document. Returns nothing when the document is
// malformed or carries an unexpected format/version.
[[nodiscard]] std::optional<std::vector<wire::TrustedPublicKey>> parse_trust_keyring_document(
    std::string_view document,
    std::string_view expected_format);

}  // namespace axlic::core
