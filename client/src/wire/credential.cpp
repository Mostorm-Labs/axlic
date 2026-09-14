#include "axlic/wire/credential.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace axlic::wire {
namespace {

constexpr std::size_t kMaximumArtifactSize = 64U * 1024U;
constexpr std::size_t kMaximumItems = 512U;
constexpr std::size_t kMaximumTextSize = 1024U;
constexpr std::size_t kMaximumEntitlements = 128U;
constexpr std::string_view kCredentialDomain = "axlicense.credential";

class ParseError final : public std::runtime_error {
 public:
  ParseError() : std::runtime_error("invalid credential") {}
};

class CborWriter {
 public:
  void unsigned_integer(const std::uint64_t value) { header(0, value); }

  void signed_integer(const std::int64_t value) {
    if (value >= 0) {
      header(0, static_cast<std::uint64_t>(value));
    } else {
      header(1, static_cast<std::uint64_t>(-(value + 1)));
    }
  }

  void text(const std::string_view value) {
    header(3, value.size());
    bytes_.insert(bytes_.end(), value.begin(), value.end());
  }

  void byte_string(const std::span<const std::uint8_t> value) {
    header(2, value.size());
    bytes_.insert(bytes_.end(), value.begin(), value.end());
  }

  void array(const std::size_t size) { header(4, size); }
  void map(const std::size_t size) { header(5, size); }

  [[nodiscard]] std::vector<std::uint8_t> take() { return std::move(bytes_); }

 private:
  void header(const std::uint8_t major, const std::uint64_t value) {
    const auto prefix = static_cast<std::uint8_t>(major << 5U);
    if (value < 24U) {
      bytes_.push_back(static_cast<std::uint8_t>(prefix | value));
    } else if (value <= 0xffU) {
      bytes_.push_back(static_cast<std::uint8_t>(prefix | 24U));
      bytes_.push_back(static_cast<std::uint8_t>(value));
    } else if (value <= 0xffffU) {
      bytes_.push_back(static_cast<std::uint8_t>(prefix | 25U));
      append_big_endian(value, 2);
    } else if (value <= 0xffffffffU) {
      bytes_.push_back(static_cast<std::uint8_t>(prefix | 26U));
      append_big_endian(value, 4);
    } else {
      bytes_.push_back(static_cast<std::uint8_t>(prefix | 27U));
      append_big_endian(value, 8);
    }
  }

  void append_big_endian(const std::uint64_t value, const unsigned width) {
    for (unsigned index = width; index > 0; --index) {
      const auto shift = static_cast<unsigned>((index - 1U) * 8U);
      bytes_.push_back(static_cast<std::uint8_t>((value >> shift) & 0xffU));
    }
  }

  std::vector<std::uint8_t> bytes_;
};

class CborReader {
 public:
  explicit CborReader(const std::span<const std::uint8_t> bytes) : bytes_(bytes) {}

  [[nodiscard]] std::uint64_t unsigned_integer() {
    const auto header = read_header();
    if (header.major != 0U) {
      throw ParseError();
    }
    return header.value;
  }

  [[nodiscard]] std::int64_t signed_integer() {
    const auto header = read_header();
    if (header.major == 0U && header.value <= static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
      return static_cast<std::int64_t>(header.value);
    }
    if (header.major == 1U && header.value <= static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
      return -1 - static_cast<std::int64_t>(header.value);
    }
    throw ParseError();
  }

  [[nodiscard]] std::string text() {
    const auto header = read_header();
    if (header.major != 3U || header.value > kMaximumTextSize || header.value > remaining()) {
      throw ParseError();
    }
    const auto size = static_cast<std::size_t>(header.value);
    const auto value = bytes_.subspan(offset_, size);
    if (!valid_utf8(value)) {
      throw ParseError();
    }
    offset_ += size;
    return {reinterpret_cast<const char*>(value.data()), value.size()};
  }

  [[nodiscard]] std::vector<std::uint8_t> byte_string() {
    const auto header = read_header();
    if (header.major != 2U || header.value > kMaximumArtifactSize || header.value > remaining()) {
      throw ParseError();
    }
    const auto size = static_cast<std::size_t>(header.value);
    std::vector<std::uint8_t> result(bytes_.begin() + static_cast<std::ptrdiff_t>(offset_),
                                     bytes_.begin() + static_cast<std::ptrdiff_t>(offset_ + size));
    offset_ += size;
    return result;
  }

  [[nodiscard]] std::size_t array_size() { return container_size(4U); }
  [[nodiscard]] std::size_t map_size() { return container_size(5U); }
  [[nodiscard]] bool finished() const noexcept { return offset_ == bytes_.size(); }

 private:
  struct Header {
    std::uint8_t major;
    std::uint64_t value;
  };

  [[nodiscard]] std::size_t remaining() const noexcept { return bytes_.size() - offset_; }

  [[nodiscard]] Header read_header() {
    if (remaining() == 0U || ++items_ > kMaximumItems) {
      throw ParseError();
    }
    const auto initial = bytes_[offset_++];
    const auto major = static_cast<std::uint8_t>(initial >> 5U);
    const auto additional = static_cast<std::uint8_t>(initial & 0x1fU);
    if (additional < 24U) {
      return {major, additional};
    }
    unsigned width = 0;
    std::uint64_t minimum = 0;
    switch (additional) {
      case 24:
        width = 1;
        minimum = 24;
        break;
      case 25:
        width = 2;
        minimum = 0x100U;
        break;
      case 26:
        width = 4;
        minimum = 0x10000U;
        break;
      case 27:
        width = 8;
        minimum = 0x100000000ULL;
        break;
      default:
        throw ParseError();
    }
    if (remaining() < width) {
      throw ParseError();
    }
    std::uint64_t value = 0;
    for (unsigned index = 0; index < width; ++index) {
      value = (value << 8U) | bytes_[offset_++];
    }
    if (value < minimum) {
      throw ParseError();
    }
    return {major, value};
  }

  [[nodiscard]] std::size_t container_size(const std::uint8_t expected_major) {
    const auto header = read_header();
    if (header.major != expected_major || header.value > kMaximumItems) {
      throw ParseError();
    }
    return static_cast<std::size_t>(header.value);
  }

  static bool valid_utf8(const std::span<const std::uint8_t> value) {
    std::size_t index = 0;
    while (index < value.size()) {
      const auto first = value[index++];
      if (first <= 0x7fU) {
        continue;
      }
      unsigned continuation_count = 0;
      std::uint32_t code_point = 0;
      if ((first & 0xe0U) == 0xc0U) {
        continuation_count = 1;
        code_point = first & 0x1fU;
      } else if ((first & 0xf0U) == 0xe0U) {
        continuation_count = 2;
        code_point = first & 0x0fU;
      } else if ((first & 0xf8U) == 0xf0U) {
        continuation_count = 3;
        code_point = first & 0x07U;
      } else {
        return false;
      }
      if (index + continuation_count > value.size()) {
        return false;
      }
      for (unsigned count = 0; count < continuation_count; ++count) {
        const auto next = value[index++];
        if ((next & 0xc0U) != 0x80U) {
          return false;
        }
        code_point = (code_point << 6U) | (next & 0x3fU);
      }
      const bool overlong = (continuation_count == 1U && code_point < 0x80U) ||
                            (continuation_count == 2U && code_point < 0x800U) ||
                            (continuation_count == 3U && code_point < 0x10000U);
      if (overlong || code_point > 0x10ffffU || (code_point >= 0xd800U && code_point <= 0xdfffU)) {
        return false;
      }
    }
    return true;
  }

  std::span<const std::uint8_t> bytes_;
  std::size_t offset_{};
  std::size_t items_{};
};

void encode_validity(CborWriter& writer, const Validity& validity) {
  const auto field_count = 1U + static_cast<unsigned>(validity.not_before.has_value()) +
                           static_cast<unsigned>(validity.not_after.has_value());
  writer.map(field_count);
  writer.unsigned_integer(1);
  writer.unsigned_integer(static_cast<std::uint8_t>(validity.kind));
  if (validity.not_before) {
    writer.unsigned_integer(2);
    writer.signed_integer(*validity.not_before);
  }
  if (validity.not_after) {
    writer.unsigned_integer(3);
    writer.signed_integer(*validity.not_after);
  }
}

void encode_entitlement(CborWriter& writer, const Entitlement& entitlement) {
  writer.map(entitlement.max_u64 ? 6 : 5);
  writer.unsigned_integer(1);
  writer.text(entitlement.entitlement_id);
  writer.unsigned_integer(2);
  writer.text(entitlement.product_id);
  writer.unsigned_integer(3);
  writer.unsigned_integer(static_cast<std::uint8_t>(entitlement.right_kind));
  writer.unsigned_integer(4);
  writer.unsigned_integer(static_cast<std::uint8_t>(entitlement.grant_semantics));
  writer.unsigned_integer(5);
  encode_validity(writer, entitlement.validity);
  if (entitlement.max_u64) {
    writer.unsigned_integer(6);
    writer.map(2);
    writer.unsigned_integer(1);
    writer.unsigned_integer(1);
    writer.unsigned_integer(2);
    writer.unsigned_integer(*entitlement.max_u64);
  }
}

Validity parse_validity(CborReader& reader) {
  const auto field_count = reader.map_size();
  if (field_count < 1U || field_count > 3U || reader.unsigned_integer() != 1U) {
    throw ParseError();
  }
  const auto kind = reader.unsigned_integer();
  if (kind < 1U || kind > 2U) {
    throw ParseError();
  }
  Validity result{.kind = static_cast<ValidityKind>(kind)};
  std::size_t consumed = 1;
  if (consumed < field_count) {
    const auto key = reader.unsigned_integer();
    if (key == 2U) {
      result.not_before = reader.signed_integer();
    } else if (key == 3U) {
      result.not_after = reader.signed_integer();
    } else {
      throw ParseError();
    }
    ++consumed;
  }
  if (consumed < field_count) {
    if (reader.unsigned_integer() != 3U || result.not_after) {
      throw ParseError();
    }
    result.not_after = reader.signed_integer();
    ++consumed;
  }
  if (consumed != field_count) {
    throw ParseError();
  }
  return result;
}

Entitlement parse_entitlement(CborReader& reader) {
  const auto field_count = reader.map_size();
  if ((field_count != 5U && field_count != 6U) || reader.unsigned_integer() != 1U) {
    throw ParseError();
  }
  Entitlement result;
  result.entitlement_id = reader.text();
  if (reader.unsigned_integer() != 2U) {
    throw ParseError();
  }
  result.product_id = reader.text();
  if (reader.unsigned_integer() != 3U) {
    throw ParseError();
  }
  const auto right_kind = reader.unsigned_integer();
  if (right_kind < 1U || right_kind > 3U) {
    throw ParseError();
  }
  result.right_kind = static_cast<RightKind>(right_kind);
  if (reader.unsigned_integer() != 4U) {
    throw ParseError();
  }
  const auto grant_semantics = reader.unsigned_integer();
  if (grant_semantics < 1U || grant_semantics > 2U) {
    throw ParseError();
  }
  result.grant_semantics = static_cast<GrantSemantics>(grant_semantics);
  if (reader.unsigned_integer() != 5U) {
    throw ParseError();
  }
  result.validity = parse_validity(reader);
  if (field_count == 6U) {
    if (reader.unsigned_integer() != 6U || reader.map_size() != 2U || reader.unsigned_integer() != 1U ||
        reader.unsigned_integer() != 1U || reader.unsigned_integer() != 2U) {
      throw ParseError();
    }
    result.max_u64 = reader.unsigned_integer();
  }
  return result;
}

Credential parse_payload(const std::span<const std::uint8_t> bytes) {
  CborReader reader(bytes);
  const auto field_count = reader.map_size();
  if ((field_count != 9U && field_count != 10U) || reader.unsigned_integer() != 1U || reader.array_size() != 2U) {
    throw ParseError();
  }
  Credential result;
  const auto major = reader.unsigned_integer();
  const auto minor = reader.unsigned_integer();
  if (major > std::numeric_limits<std::uint16_t>::max() || minor > std::numeric_limits<std::uint16_t>::max()) {
    throw ParseError();
  }
  result.schema_major = static_cast<std::uint16_t>(major);
  result.schema_minor = static_cast<std::uint16_t>(minor);
  if (reader.unsigned_integer() != 2U) {
    throw ParseError();
  }
  result.credential_id = reader.text();
  if (reader.unsigned_integer() != 3U) {
    throw ParseError();
  }
  result.license_grant_id = reader.text();
  if (reader.unsigned_integer() != 4U) {
    throw ParseError();
  }
  result.authority_revision = reader.unsigned_integer();
  if (reader.unsigned_integer() != 5U) {
    throw ParseError();
  }
  result.binding_id = reader.text();
  if (reader.unsigned_integer() != 6U) {
    throw ParseError();
  }
  result.device_id = reader.text();
  if (reader.unsigned_integer() != 7U) {
    throw ParseError();
  }
  result.credential_generation = reader.unsigned_integer();
  if (reader.unsigned_integer() != 8U) {
    throw ParseError();
  }
  result.issued_at = reader.signed_integer();
  if (field_count == 10U) {
    if (reader.unsigned_integer() != 9U) {
      throw ParseError();
    }
    result.supersedes_credential_id = reader.text();
  }
  if (reader.unsigned_integer() != 10U) {
    throw ParseError();
  }
  const auto entitlement_count = reader.array_size();
  if (entitlement_count > kMaximumEntitlements) {
    throw ParseError();
  }
  result.entitlements.reserve(entitlement_count);
  for (std::size_t index = 0; index < entitlement_count; ++index) {
    result.entitlements.push_back(parse_entitlement(reader));
  }
  if (!reader.finished()) {
    throw ParseError();
  }
  return result;
}

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

struct Envelope {
  std::uint64_t artifact_kind{};
  std::uint64_t envelope_version{};
  std::string algorithm_id;
  std::string key_id;
  std::vector<std::uint8_t> payload;
  std::vector<std::uint8_t> signature;
};

Envelope parse_envelope(const std::span<const std::uint8_t> artifact) {
  CborReader reader(artifact);
  if (reader.map_size() != 6U || reader.unsigned_integer() != 1U) {
    throw ParseError();
  }
  Envelope result;
  result.artifact_kind = reader.unsigned_integer();
  if (reader.unsigned_integer() != 2U) {
    throw ParseError();
  }
  result.envelope_version = reader.unsigned_integer();
  if (reader.unsigned_integer() != 3U) {
    throw ParseError();
  }
  result.algorithm_id = reader.text();
  if (reader.unsigned_integer() != 4U) {
    throw ParseError();
  }
  result.key_id = reader.text();
  if (reader.unsigned_integer() != 5U) {
    throw ParseError();
  }
  result.payload = reader.byte_string();
  if (reader.unsigned_integer() != 6U) {
    throw ParseError();
  }
  result.signature = reader.byte_string();
  if (!reader.finished()) {
    throw ParseError();
  }
  return result;
}

std::vector<std::uint8_t> encode_envelope(const Envelope& envelope) {
  CborWriter writer;
  writer.map(6);
  writer.unsigned_integer(1);
  writer.unsigned_integer(envelope.artifact_kind);
  writer.unsigned_integer(2);
  writer.unsigned_integer(envelope.envelope_version);
  writer.unsigned_integer(3);
  writer.text(envelope.algorithm_id);
  writer.unsigned_integer(4);
  writer.text(envelope.key_id);
  writer.unsigned_integer(5);
  writer.byte_string(envelope.payload);
  writer.unsigned_integer(6);
  writer.byte_string(envelope.signature);
  return writer.take();
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
  CborWriter writer;
  writer.map(credential.supersedes_credential_id ? 10 : 9);
  writer.unsigned_integer(1);
  writer.array(2);
  writer.unsigned_integer(credential.schema_major);
  writer.unsigned_integer(credential.schema_minor);
  writer.unsigned_integer(2);
  writer.text(credential.credential_id);
  writer.unsigned_integer(3);
  writer.text(credential.license_grant_id);
  writer.unsigned_integer(4);
  writer.unsigned_integer(credential.authority_revision);
  writer.unsigned_integer(5);
  writer.text(credential.binding_id);
  writer.unsigned_integer(6);
  writer.text(credential.device_id);
  writer.unsigned_integer(7);
  writer.unsigned_integer(credential.credential_generation);
  writer.unsigned_integer(8);
  writer.signed_integer(credential.issued_at);
  if (credential.supersedes_credential_id) {
    writer.unsigned_integer(9);
    writer.text(*credential.supersedes_credential_id);
  }
  writer.unsigned_integer(10);
  std::vector<const Entitlement*> ordered;
  ordered.reserve(credential.entitlements.size());
  for (const auto& entitlement : credential.entitlements) {
    ordered.push_back(&entitlement);
  }
  std::sort(ordered.begin(), ordered.end(), [](const auto* left, const auto* right) {
    return left->entitlement_id < right->entitlement_id;
  });
  writer.array(ordered.size());
  for (const auto* entitlement : ordered) {
    encode_entitlement(writer, *entitlement);
  }
  return writer.take();
}

std::vector<std::uint8_t> make_credential_signing_input(
    const std::string_view algorithm_id,
    const std::string_view key_id,
    const std::span<const std::uint8_t> payload_bytes) {
  CborWriter writer;
  writer.array(5);
  writer.text(kCredentialDomain);
  writer.unsigned_integer(1);
  writer.text(algorithm_id);
  writer.text(key_id);
  writer.byte_string(payload_bytes);
  return writer.take();
}

VerificationResult verify_credential(
    const std::span<const std::uint8_t> artifact,
    const std::span<const TrustedPublicKey> trusted_keys) {
  if (artifact.empty() || artifact.size() > kMaximumArtifactSize) {
    return {};
  }
  try {
    const auto envelope = parse_envelope(artifact);
    if (encode_envelope(envelope) != std::vector<std::uint8_t>(artifact.begin(), artifact.end())) {
      return {};
    }
    if (envelope.artifact_kind != 1U || envelope.envelope_version != 1U || envelope.algorithm_id != kAlgorithmId) {
      return {.status = VerificationStatus::unsupported};
    }
    const auto key = std::find_if(trusted_keys.begin(), trusted_keys.end(), [&](const TrustedPublicKey& candidate) {
      return candidate.key_id == envelope.key_id;
    });
    if (key == trusted_keys.end()) {
      return {.status = VerificationStatus::unsupported};
    }
    const auto credential = parse_payload(envelope.payload);
    if (encode_credential_payload(credential) != envelope.payload || !valid_low_s_signature(envelope.signature)) {
      return {};
    }
    const auto signing_bytes = make_credential_signing_input(envelope.algorithm_id, envelope.key_id, envelope.payload);
    if (!cng_verify(signing_bytes, envelope.signature, key->public_key_xy)) {
      return {};
    }
    if (credential.schema_major != 1U) {
      return {.status = VerificationStatus::unsupported};
    }
    if (!valid_semantics(credential)) {
      return {};
    }
    return {.status = VerificationStatus::valid, .credential = credential};
  } catch (const ParseError&) {
    return {};
  } catch (...) {
    return {};
  }
}

}  // namespace axlic::wire
