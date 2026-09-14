#include "qcbor_codec.hpp"

#include <qcbor/qcbor_encode.h>
#include <qcbor/qcbor_spiffy_decode.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace axlic::wire::detail {
namespace {

constexpr std::size_t kMaximumArtifactSize = 64U * 1024U;
constexpr std::size_t kMaximumTextSize = 1024U;
constexpr std::size_t kMaximumItems = 512U;
constexpr std::uint8_t kMaximumNestingDepth = 8U;
constexpr std::size_t kMaximumEntitlements = 128U;
constexpr std::string_view kCredentialDomain = "axlicense.credential";

UsefulBufC as_useful(const std::span<const std::uint8_t> bytes) {
  return UsefulBufC{bytes.data(), bytes.size()};
}

UsefulBufC as_useful(const std::string_view text) {
  return UsefulBufC{text.data(), text.size()};
}

void validate_structural_bounds(const std::span<const std::uint8_t> bytes) {
  QCBORDecodeContext context{};
  QCBORDecode_Init(&context, as_useful(bytes), QCBOR_DECODE_MODE_NORMAL);
  std::size_t item_count = 0;
  for (;;) {
    QCBORItem item{};
    const auto error = QCBORDecode_GetNext(&context, &item);
    if (error == QCBOR_ERR_NO_MORE_ITEMS) {
      break;
    }
    if (error != QCBOR_SUCCESS || ++item_count > kMaximumItems || item.uNestingLevel > kMaximumNestingDepth) {
      throw CodecError();
    }
  }
}

void require_decode_success(QCBORDecodeContext& context) {
  if (QCBORDecode_GetAndResetError(&context) != QCBOR_SUCCESS) {
    throw CodecError();
  }
}

bool valid_utf8(const std::span<const std::uint8_t> value) {
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

std::string copy_text(const UsefulBufC value) {
  if (value.len > kMaximumTextSize) {
    throw CodecError();
  }
  if (value.len == 0U) {
    return {};
  }
  const auto* bytes = static_cast<const std::uint8_t*>(value.ptr);
  if (!valid_utf8(std::span<const std::uint8_t>{bytes, value.len})) {
    throw CodecError();
  }
  const auto* first = reinterpret_cast<const char*>(bytes);
  return std::string(first, first + value.len);
}

std::vector<std::uint8_t> copy_bytes(const UsefulBufC value) {
  if (value.len > kMaximumArtifactSize) {
    throw CodecError();
  }
  if (value.len == 0U) {
    return {};
  }
  const auto* first = static_cast<const std::uint8_t*>(value.ptr);
  return std::vector<std::uint8_t>(first, first + value.len);
}

template <typename EncodeFn>
std::vector<std::uint8_t> encode_with_qcbor(EncodeFn&& encode_fn) {
  std::vector<std::uint8_t> storage(kMaximumArtifactSize);
  QCBOREncodeContext context{};
  QCBOREncode_Init(&context, UsefulBuf{storage.data(), storage.size()});
  encode_fn(context);

  UsefulBufC encoded{};
  if (QCBOREncode_Finish(&context, &encoded) != QCBOR_SUCCESS || encoded.len > storage.size()) {
    throw CodecError();
  }
  const auto* first = static_cast<const std::uint8_t*>(encoded.ptr);
  return std::vector<std::uint8_t>(first, first + encoded.len);
}

std::uint64_t required_uint(QCBORDecodeContext& context, const std::int64_t label) {
  std::uint64_t value = 0;
  QCBORDecode_GetUInt64InMapN(&context, label, &value);
  require_decode_success(context);
  return value;
}

std::int64_t required_int(QCBORDecodeContext& context, const std::int64_t label) {
  std::int64_t value = 0;
  QCBORDecode_GetInt64InMapN(&context, label, &value);
  require_decode_success(context);
  return value;
}

std::string required_text(QCBORDecodeContext& context, const std::int64_t label) {
  UsefulBufC value{};
  QCBORDecode_GetTextStringInMapN(&context, label, &value);
  require_decode_success(context);
  return copy_text(value);
}

std::optional<std::string> optional_text(QCBORDecodeContext& context, const std::int64_t label) {
  UsefulBufC value{};
  QCBORDecode_GetTextStringInMapN(&context, label, &value);
  const auto error = QCBORDecode_GetAndResetError(&context);
  if (error == QCBOR_ERR_LABEL_NOT_FOUND) {
    return std::nullopt;
  }
  if (error != QCBOR_SUCCESS) {
    throw CodecError();
  }
  return copy_text(value);
}

std::optional<std::int64_t> optional_int(QCBORDecodeContext& context, const std::int64_t label) {
  std::int64_t value = 0;
  QCBORDecode_GetInt64InMapN(&context, label, &value);
  const auto error = QCBORDecode_GetAndResetError(&context);
  if (error == QCBOR_ERR_LABEL_NOT_FOUND) {
    return std::nullopt;
  }
  if (error != QCBOR_SUCCESS) {
    throw CodecError();
  }
  return value;
}

std::size_t required_container_count(
    QCBORDecodeContext& context,
    const std::int64_t label,
    const std::uint8_t expected_type) {
  QCBORItem item{};
  QCBORDecode_GetItemInMapN(&context, label, expected_type, &item);
  require_decode_success(context);
  if (item.val.uCount == QCBOR_COUNT_INDICATES_INDEFINITE_LENGTH) {
    throw CodecError();
  }
  return item.val.uCount;
}

void encode_validity(QCBOREncodeContext& context, const Validity& validity) {
  QCBOREncode_OpenMapInMapN(&context, 5);
  QCBOREncode_AddUInt64ToMapN(&context, 1, static_cast<std::uint8_t>(validity.kind));
  if (validity.not_before) {
    QCBOREncode_AddInt64ToMapN(&context, 2, *validity.not_before);
  }
  if (validity.not_after) {
    QCBOREncode_AddInt64ToMapN(&context, 3, *validity.not_after);
  }
  QCBOREncode_CloseMap(&context);
}

void encode_entitlement(QCBOREncodeContext& context, const Entitlement& entitlement) {
  QCBOREncode_OpenMap(&context);
  QCBOREncode_AddTextToMapN(&context, 1, as_useful(entitlement.entitlement_id));
  QCBOREncode_AddTextToMapN(&context, 2, as_useful(entitlement.product_id));
  QCBOREncode_AddUInt64ToMapN(&context, 3, static_cast<std::uint8_t>(entitlement.right_kind));
  QCBOREncode_AddUInt64ToMapN(&context, 4, static_cast<std::uint8_t>(entitlement.grant_semantics));
  encode_validity(context, entitlement.validity);
  if (entitlement.max_u64) {
    QCBOREncode_OpenMapInMapN(&context, 6);
    QCBOREncode_AddUInt64ToMapN(&context, 1, 1);
    QCBOREncode_AddUInt64ToMapN(&context, 2, *entitlement.max_u64);
    QCBOREncode_CloseMap(&context);
  }
  QCBOREncode_CloseMap(&context);
}

Validity decode_validity(QCBORDecodeContext& context) {
  const auto field_count = required_container_count(context, 5, QCBOR_TYPE_MAP);
  if (field_count < 1U || field_count > 3U) {
    throw CodecError();
  }

  QCBORDecode_EnterMapFromMapN(&context, 5);
  require_decode_success(context);
  const auto kind = required_uint(context, 1);
  if (kind < 1U || kind > 2U) {
    throw CodecError();
  }
  Validity result;
  result.kind = static_cast<ValidityKind>(kind);
  result.not_before = optional_int(context, 2);
  result.not_after = optional_int(context, 3);
  QCBORDecode_ExitMap(&context);
  require_decode_success(context);
  return result;
}

Entitlement decode_entitlement(QCBORDecodeContext& context) {
  QCBORItem map_item{};
  QCBORDecode_EnterMap(&context, &map_item);
  require_decode_success(context);
  if (map_item.val.uCount != 5U && map_item.val.uCount != 6U) {
    throw CodecError();
  }

  Entitlement result;
  result.entitlement_id = required_text(context, 1);
  result.product_id = required_text(context, 2);

  const auto right_kind = required_uint(context, 3);
  if (right_kind < 1U || right_kind > 3U) {
    throw CodecError();
  }
  result.right_kind = static_cast<RightKind>(right_kind);

  const auto grant_semantics = required_uint(context, 4);
  if (grant_semantics < 1U || grant_semantics > 2U) {
    throw CodecError();
  }
  result.grant_semantics = static_cast<GrantSemantics>(grant_semantics);
  result.validity = decode_validity(context);

  if (map_item.val.uCount == 6U) {
    const auto constraint_count = required_container_count(context, 6, QCBOR_TYPE_MAP);
    if (constraint_count != 2U) {
      throw CodecError();
    }
    QCBORDecode_EnterMapFromMapN(&context, 6);
    require_decode_success(context);
    if (required_uint(context, 1) != 1U) {
      throw CodecError();
    }
    result.max_u64 = required_uint(context, 2);
    QCBORDecode_ExitMap(&context);
    require_decode_success(context);
  }

  QCBORDecode_ExitMap(&context);
  require_decode_success(context);
  return result;
}

}  // namespace

std::vector<std::uint8_t> encode_credential_payload_cbor(const Credential& credential) {
  return encode_with_qcbor([&](QCBOREncodeContext& context) {
    QCBOREncode_OpenMap(&context);
    QCBOREncode_OpenArrayInMapN(&context, 1);
    QCBOREncode_AddUInt64(&context, credential.schema_major);
    QCBOREncode_AddUInt64(&context, credential.schema_minor);
    QCBOREncode_CloseArray(&context);
    QCBOREncode_AddTextToMapN(&context, 2, as_useful(credential.credential_id));
    QCBOREncode_AddTextToMapN(&context, 3, as_useful(credential.license_grant_id));
    QCBOREncode_AddUInt64ToMapN(&context, 4, credential.authority_revision);
    QCBOREncode_AddTextToMapN(&context, 5, as_useful(credential.binding_id));
    QCBOREncode_AddTextToMapN(&context, 6, as_useful(credential.device_id));
    QCBOREncode_AddUInt64ToMapN(&context, 7, credential.credential_generation);
    QCBOREncode_AddInt64ToMapN(&context, 8, credential.issued_at);
    if (credential.supersedes_credential_id) {
      QCBOREncode_AddTextToMapN(&context, 9, as_useful(*credential.supersedes_credential_id));
    }

    std::vector<const Entitlement*> ordered;
    ordered.reserve(credential.entitlements.size());
    for (const auto& entitlement : credential.entitlements) {
      ordered.push_back(&entitlement);
    }
    std::sort(ordered.begin(), ordered.end(), [](const auto* left, const auto* right) {
      return left->entitlement_id < right->entitlement_id;
    });

    QCBOREncode_OpenArrayInMapN(&context, 10);
    for (const auto* entitlement : ordered) {
      encode_entitlement(context, *entitlement);
    }
    QCBOREncode_CloseArray(&context);
    QCBOREncode_CloseMap(&context);
  });
}

Credential decode_credential_payload_cbor(const std::span<const std::uint8_t> bytes) {
  if (bytes.empty() || bytes.size() > kMaximumArtifactSize) {
    throw CodecError();
  }

  validate_structural_bounds(bytes);

  QCBORDecodeContext context{};
  QCBORDecode_Init(&context, as_useful(bytes), QCBOR_DECODE_MODE_NORMAL);
  QCBORItem root{};
  QCBORDecode_EnterMap(&context, &root);
  require_decode_success(context);
  if (root.val.uCount != 9U && root.val.uCount != 10U) {
    throw CodecError();
  }

  const auto version_count = required_container_count(context, 1, QCBOR_TYPE_ARRAY);
  if (version_count != 2U) {
    throw CodecError();
  }
  QCBORDecode_EnterArrayFromMapN(&context, 1);
  require_decode_success(context);
  std::uint64_t schema_major = 0;
  std::uint64_t schema_minor = 0;
  QCBORDecode_GetUInt64(&context, &schema_major);
  QCBORDecode_GetUInt64(&context, &schema_minor);
  require_decode_success(context);
  QCBORDecode_ExitArray(&context);
  require_decode_success(context);
  if (schema_major > std::numeric_limits<std::uint16_t>::max() ||
      schema_minor > std::numeric_limits<std::uint16_t>::max()) {
    throw CodecError();
  }

  Credential result;
  result.schema_major = static_cast<std::uint16_t>(schema_major);
  result.schema_minor = static_cast<std::uint16_t>(schema_minor);
  result.credential_id = required_text(context, 2);
  result.license_grant_id = required_text(context, 3);
  result.authority_revision = required_uint(context, 4);
  result.binding_id = required_text(context, 5);
  result.device_id = required_text(context, 6);
  result.credential_generation = required_uint(context, 7);
  result.issued_at = required_int(context, 8);
  result.supersedes_credential_id = optional_text(context, 9);

  const auto entitlement_count = required_container_count(context, 10, QCBOR_TYPE_ARRAY);
  if (entitlement_count > kMaximumEntitlements) {
    throw CodecError();
  }
  QCBORDecode_EnterArrayFromMapN(&context, 10);
  require_decode_success(context);
  result.entitlements.reserve(entitlement_count);
  for (std::size_t index = 0; index < entitlement_count; ++index) {
    result.entitlements.push_back(decode_entitlement(context));
  }
  QCBORDecode_ExitArray(&context);
  require_decode_success(context);

  QCBORDecode_ExitMap(&context);
  require_decode_success(context);
  if (QCBORDecode_Finish(&context) != QCBOR_SUCCESS) {
    throw CodecError();
  }
  return result;
}

std::vector<std::uint8_t> encode_envelope_cbor(const SignedEnvelope& envelope) {
  return encode_with_qcbor([&](QCBOREncodeContext& context) {
    QCBOREncode_OpenMap(&context);
    QCBOREncode_AddUInt64ToMapN(&context, 1, envelope.artifact_kind);
    QCBOREncode_AddUInt64ToMapN(&context, 2, envelope.envelope_version);
    QCBOREncode_AddTextToMapN(&context, 3, as_useful(envelope.algorithm_id));
    QCBOREncode_AddTextToMapN(&context, 4, as_useful(envelope.key_id));
    QCBOREncode_AddBytesToMapN(&context, 5, as_useful(std::span<const std::uint8_t>{envelope.payload}));
    QCBOREncode_AddBytesToMapN(&context, 6, as_useful(std::span<const std::uint8_t>{envelope.signature}));
    QCBOREncode_CloseMap(&context);
  });
}

SignedEnvelope decode_envelope_cbor(const std::span<const std::uint8_t> bytes) {
  if (bytes.empty() || bytes.size() > kMaximumArtifactSize) {
    throw CodecError();
  }

  validate_structural_bounds(bytes);

  QCBORDecodeContext context{};
  QCBORDecode_Init(&context, as_useful(bytes), QCBOR_DECODE_MODE_NORMAL);
  QCBORItem root{};
  QCBORDecode_EnterMap(&context, &root);
  require_decode_success(context);
  if (root.val.uCount != 6U) {
    throw CodecError();
  }

  SignedEnvelope result;
  result.artifact_kind = required_uint(context, 1);
  result.envelope_version = required_uint(context, 2);
  result.algorithm_id = required_text(context, 3);
  result.key_id = required_text(context, 4);

  UsefulBufC payload{};
  QCBORDecode_GetByteStringInMapN(&context, 5, &payload);
  require_decode_success(context);
  result.payload = copy_bytes(payload);

  UsefulBufC signature{};
  QCBORDecode_GetByteStringInMapN(&context, 6, &signature);
  require_decode_success(context);
  result.signature = copy_bytes(signature);

  QCBORDecode_ExitMap(&context);
  require_decode_success(context);
  if (QCBORDecode_Finish(&context) != QCBOR_SUCCESS) {
    throw CodecError();
  }
  return result;
}

std::vector<std::uint8_t> encode_credential_signing_input_cbor(
    const std::string_view algorithm_id,
    const std::string_view key_id,
    const std::span<const std::uint8_t> payload_bytes) {
  return encode_with_qcbor([&](QCBOREncodeContext& context) {
    QCBOREncode_OpenArray(&context);
    QCBOREncode_AddText(&context, as_useful(kCredentialDomain));
    QCBOREncode_AddUInt64(&context, 1);
    QCBOREncode_AddText(&context, as_useful(algorithm_id));
    QCBOREncode_AddText(&context, as_useful(key_id));
    QCBOREncode_AddBytes(&context, as_useful(payload_bytes));
    QCBOREncode_CloseArray(&context);
  });
}

}  // namespace axlic::wire::detail
