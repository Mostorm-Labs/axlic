#include "axlic/wire/credential.hpp"

#include "qcbor_codec.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace axlic::wire {
namespace {

constexpr std::size_t kMaximumArtifactSize = 64U * 1024U;

bool lowercase_ascii_identifier(const std::string_view value) {
  if (value.empty() || value.size() > 128U) {
    return false;
  }
  return std::all_of(value.begin(), value.end(), [](const unsigned char character) {
    return (character >= 'a' && character <= 'z') || (character >= '0' && character <= '9') ||
           character == '.' || character == '_' || character == '-';
  });
}

bool valid_semantics(const Credential& credential) {
  if (credential.credential_id.empty() || credential.license_grant_id.empty() || credential.binding_id.empty() ||
      credential.device_id.empty() || credential.authority_revision == 0U || credential.credential_generation == 0U) {
    return false;
  }
  std::string previous_id;
  for (const auto& entitlement : credential.entitlements) {
    if (!lowercase_ascii_identifier(entitlement.product_id) ||
        !lowercase_ascii_identifier(entitlement.entitlement_id) ||
        !entitlement.entitlement_id.starts_with(entitlement.product_id + ".") ||
        (!previous_id.empty() && entitlement.entitlement_id <= previous_id)) {
      return false;
    }
    previous_id = entitlement.entitlement_id;
    if (entitlement.validity.kind == ValidityKind::perpetual) {
      if (entitlement.validity.not_after) {
        return false;
      }
    } else if (entitlement.validity.kind == ValidityKind::bounded) {
      if (!entitlement.validity.not_after ||
          (entitlement.validity.not_before && *entitlement.validity.not_before >= *entitlement.validity.not_after)) {
        return false;
      }
    } else {
      return false;
    }
    if (entitlement.grant_semantics == GrantSemantics::presence) {
      if (entitlement.max_u64) {
        return false;
      }
    } else if (entitlement.grant_semantics == GrantSemantics::bounded_u64) {
      if (!entitlement.max_u64 || *entitlement.max_u64 == 0U) {
        return false;
      }
    } else {
      return false;
    }
  }
  return true;
}

int compare_big_endian(const std::span<const std::uint8_t> left, const std::span<const std::uint8_t> right) {
  return std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end())
             ? -1
             : (std::lexicographical_compare(right.begin(), right.end(), left.begin(), left.end()) ? 1 : 0);
}

bool valid_low_s_signature(const std::span<const std::uint8_t> signature) {
  constexpr std::array<std::uint8_t, 32> order = {
      0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xbc, 0xe6, 0xfa, 0xad, 0xa7, 0x17, 0x9e, 0x84, 0xf3, 0xb9, 0xca, 0xc2, 0xfc, 0x63, 0x25, 0x51};
  constexpr std::array<std::uint8_t, 32> half_order = {
      0x7f, 0xff, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xde, 0x73, 0x7d, 0x56, 0xd3, 0x8b, 0xcf, 0x42, 0x79, 0xdc, 0xe5, 0x61, 0x7e, 0x31, 0x92, 0xa8};
  if (signature.size() != 64U) {
    return false;
  }
  const auto r = signature.first<32>();
  const auto s = signature.last<32>();
  const bool r_zero = std::all_of(r.begin(), r.end(), [](const auto value) { return value == 0U; });
  const bool s_zero = std::all_of(s.begin(), s.end(), [](const auto value) { return value == 0U; });
  return !r_zero && !s_zero && compare_big_endian(r, order) < 0 && compare_big_endian(s, order) < 0 &&
         compare_big_endian(s, half_order) <= 0;
}

bool cng_verify(
    const std::span<const std::uint8_t> message,
    const std::span<const std::uint8_t> signature,
    const std::array<std::uint8_t, 64>& public_key_xy) {
  BCRYPT_ALG_HANDLE hash_algorithm = nullptr;
  BCRYPT_ALG_HANDLE signature_algorithm = nullptr;
  BCRYPT_KEY_HANDLE key = nullptr;
  std::array<std::uint8_t, 32> digest{};
  std::vector<std::uint8_t> blob(sizeof(BCRYPT_ECCKEY_BLOB) + public_key_xy.size());
  const BCRYPT_ECCKEY_BLOB header{BCRYPT_ECDSA_PUBLIC_P256_MAGIC, 32U};
  std::memcpy(blob.data(), &header, sizeof(header));
  std::memcpy(blob.data() + sizeof(header), public_key_xy.data(), public_key_xy.size());

  auto cleanup = [&]() {
    if (key != nullptr) {
      BCryptDestroyKey(key);
    }
    if (signature_algorithm != nullptr) {
      BCryptCloseAlgorithmProvider(signature_algorithm, 0);
    }
    if (hash_algorithm != nullptr) {
      BCryptCloseAlgorithmProvider(hash_algorithm, 0);
    }
  };

  if (BCryptOpenAlgorithmProvider(&hash_algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0 ||
      BCryptHash(hash_algorithm, nullptr, 0, const_cast<PUCHAR>(message.data()), static_cast<ULONG>(message.size()),
                 digest.data(), static_cast<ULONG>(digest.size())) < 0 ||
      BCryptOpenAlgorithmProvider(&signature_algorithm, BCRYPT_ECDSA_P256_ALGORITHM, nullptr, 0) < 0 ||
      BCryptImportKeyPair(signature_algorithm, nullptr, BCRYPT_ECCPUBLIC_BLOB, &key, blob.data(),
                          static_cast<ULONG>(blob.size()), 0) < 0) {
    cleanup();
    return false;
  }
  const auto status = BCryptVerifySignature(key, nullptr, digest.data(), static_cast<ULONG>(digest.size()),
                                            const_cast<PUCHAR>(signature.data()),
                                            static_cast<ULONG>(signature.size()), 0);
  cleanup();
  return status >= 0;
}

}  // namespace

std::vector<std::uint8_t> encode_credential_payload(const Credential& credential) {
  return detail::encode_credential_payload_cbor(credential);
}

std::vector<std::uint8_t> make_credential_signing_input(
    const std::string_view algorithm_id,
    const std::string_view key_id,
    const std::span<const std::uint8_t> payload_bytes) {
  return detail::encode_credential_signing_input_cbor(algorithm_id, key_id, payload_bytes);
}

VerificationResult verify_credential(
    const std::span<const std::uint8_t> artifact,
    const std::span<const TrustedPublicKey> trusted_keys) {
  if (artifact.empty() || artifact.size() > kMaximumArtifactSize) {
    return {};
  }
  try {
    const auto envelope = detail::decode_envelope_cbor(artifact);
    if (detail::encode_envelope_cbor(envelope) != std::vector<std::uint8_t>(artifact.begin(), artifact.end())) {
      return {};
    }
    if (envelope.artifact_kind != 1U || envelope.envelope_version != 1U || envelope.algorithm_id != kAlgorithmId) {
      return {.status = VerificationStatus::unsupported, .credential = std::nullopt};
    }
    const auto key = std::find_if(trusted_keys.begin(), trusted_keys.end(), [&](const TrustedPublicKey& candidate) {
      return candidate.key_id == envelope.key_id;
    });
    if (key == trusted_keys.end()) {
      return {.status = VerificationStatus::unsupported, .credential = std::nullopt};
    }

    const auto credential = detail::decode_credential_payload_cbor(envelope.payload);
    if (detail::encode_credential_payload_cbor(credential) != envelope.payload ||
        !valid_low_s_signature(envelope.signature)) {
      return {};
    }

    const auto signing_bytes = detail::encode_credential_signing_input_cbor(
        envelope.algorithm_id, envelope.key_id, envelope.payload);
    if (!cng_verify(signing_bytes, envelope.signature, key->public_key_xy)) {
      return {};
    }
    if (credential.schema_major != 1U) {
      return {.status = VerificationStatus::unsupported, .credential = std::nullopt};
    }
    if (!valid_semantics(credential)) {
      return {};
    }
    return {.status = VerificationStatus::valid, .credential = credential};
  } catch (const detail::CodecError&) {
    return {};
  } catch (...) {
    return {};
  }
}

}  // namespace axlic::wire
