#pragma once

#include "axlic/wire/credential.hpp"

#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace axlic::wire::detail {

class CodecError final : public std::runtime_error {
 public:
  CodecError() : std::runtime_error("invalid credential encoding") {}
};

struct SignedEnvelope {
  std::uint64_t artifact_kind{};
  std::uint64_t envelope_version{};
  std::string algorithm_id;
  std::string key_id;
  std::vector<std::uint8_t> payload;
  std::vector<std::uint8_t> signature;
};

std::vector<std::uint8_t> encode_credential_payload_cbor(const Credential& credential);
Credential decode_credential_payload_cbor(std::span<const std::uint8_t> bytes);

std::vector<std::uint8_t> encode_envelope_cbor(const SignedEnvelope& envelope);
SignedEnvelope decode_envelope_cbor(std::span<const std::uint8_t> bytes);

std::vector<std::uint8_t> encode_credential_signing_input_cbor(
    std::string_view algorithm_id,
    std::string_view key_id,
    std::span<const std::uint8_t> payload_bytes);

}  // namespace axlic::wire::detail
