#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace axlic::wire {

enum class RightKind : std::uint8_t {
  runtime = 1,
  maintenance_update = 2,
  cloud_service = 3,
};

enum class GrantSemantics : std::uint8_t {
  presence = 1,
  bounded_u64 = 2,
};

enum class ValidityKind : std::uint8_t {
  perpetual = 1,
  bounded = 2,
};

struct Validity {
  ValidityKind kind{ValidityKind::perpetual};
  std::optional<std::int64_t> not_before;
  std::optional<std::int64_t> not_after;
};

struct Entitlement {
  std::string entitlement_id;
  std::string product_id;
  RightKind right_kind{RightKind::runtime};
  GrantSemantics grant_semantics{GrantSemantics::presence};
  Validity validity;
  std::optional<std::uint64_t> max_u64;
};

struct Credential {
  std::uint16_t schema_major{1};
  std::uint16_t schema_minor{0};
  std::string credential_id;
  std::string license_grant_id;
  std::uint64_t authority_revision{};
  std::string binding_id;
  std::string device_id;
  std::uint64_t credential_generation{};
  std::int64_t issued_at{};
  std::optional<std::string> supersedes_credential_id;
  std::vector<Entitlement> entitlements;
};

inline constexpr std::string_view kAlgorithmId = "axl-ecdsa-p256-sha256-v1";

enum class VerificationStatus {
  valid,
  invalid,
  unsupported,
};

struct TrustedPublicKey {
  std::string key_id;
  std::array<std::uint8_t, 64> public_key_xy{};
};

struct VerificationResult {
  VerificationStatus status{VerificationStatus::invalid};
  std::optional<Credential> credential;
};

std::vector<std::uint8_t> encode_credential_payload(const Credential& credential);
std::vector<std::uint8_t> make_credential_signing_input(
    std::string_view algorithm_id,
    std::string_view key_id,
    std::span<const std::uint8_t> payload_bytes);
VerificationResult verify_credential(
    std::span<const std::uint8_t> artifact,
    std::span<const TrustedPublicKey> trusted_keys);

}  // namespace axlic::wire
