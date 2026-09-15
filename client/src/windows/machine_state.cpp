#include "axlic/windows/machine_state.hpp"

#include <nlohmann/json.hpp>

#define NOMINMAX
#include <windows.h>
#include <wincrypt.h>

#include <algorithm>
#include <array>
#include <limits>
#include <string>
#include <utility>

namespace axlic::windows {
namespace {

constexpr std::string_view kEntropy = "AxLicense/MachineState/v1";

bool is_lower_hex(const std::string_view value, const std::size_t exact_length = 0U) {
  if ((exact_length != 0U && value.size() != exact_length) || (value.size() % 2U) != 0U) {
    return false;
  }
  return std::ranges::all_of(value, [](const char character) {
    return (character >= '0' && character <= '9') || (character >= 'a' && character <= 'f');
  });
}

std::string encode_hex(const std::vector<std::uint8_t>& bytes) {
  constexpr std::array<char, 16> digits{'0', '1', '2', '3', '4', '5', '6', '7', '8',
                                        '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  std::string result;
  result.reserve(bytes.size() * 2U);
  for (const auto byte : bytes) {
    result.push_back(digits[byte >> 4U]);
    result.push_back(digits[byte & 0x0fU]);
  }
  return result;
}

std::vector<std::uint8_t> decode_hex(const std::string_view value) {
  std::vector<std::uint8_t> result;
  result.reserve(value.size() / 2U);
  const auto nibble = [](const char character) -> std::uint8_t {
    return character <= '9' ? static_cast<std::uint8_t>(character - '0')
                            : static_cast<std::uint8_t>(character - 'a' + 10);
  };
  for (std::size_t index = 0; index < value.size(); index += 2U) {
    result.push_back(static_cast<std::uint8_t>((nibble(value[index]) << 4U) | nibble(value[index + 1U])));
  }
  return result;
}

DATA_BLOB blob(const std::vector<std::uint8_t>& bytes) {
  return DATA_BLOB{
      .cbData = static_cast<DWORD>(bytes.size()),
      .pbData = const_cast<BYTE*>(bytes.data()),
  };
}

DATA_BLOB entropy_blob() {
  return DATA_BLOB{
      .cbData = static_cast<DWORD>(kEntropy.size()),
      .pbData = reinterpret_cast<BYTE*>(const_cast<char*>(kEntropy.data())),
  };
}

ProtectionResult copy_and_free(DATA_BLOB& output) {
  std::vector<std::uint8_t> result(output.pbData, output.pbData + output.cbData);
  LocalFree(output.pbData);
  return {ProtectionOutcome::ok, std::move(result)};
}

bool valid_key_name(const std::string_view value) {
  constexpr std::string_view prefix = "Auditoryworks.AxLicense.Identity.v1.";
  return value.starts_with(prefix) && value.size() == prefix.size() + 32U &&
         is_lower_hex(value.substr(prefix.size()), 32U);
}

bool valid_record(const core::IdentityRecord& record) {
  const bool tpm = record.provider_kind == core::ProviderKind::tpm;
  const std::string_view expected_scheme =
      tpm ? "axl-win-cng-tpm-p256-v1" : "axl-win-cng-software-p256-v1";
  const std::string_view expected_provider =
      tpm ? "Microsoft Platform Crypto Provider" : "Microsoft Software Key Storage Provider";
  return record.scheme_id == expected_scheme && record.provider_name == expected_provider &&
         record.identity_epoch == 1U && is_lower_hex(record.identity_value, 128U) && valid_key_name(record.key_name);
}

}  // namespace

ProtectionResult DpapiProtector::protect(const std::vector<std::uint8_t>& plaintext) {
  if (plaintext.empty() || plaintext.size() > std::numeric_limits<DWORD>::max()) {
    return {ProtectionOutcome::unexpected_failure, {}};
  }
  auto input = blob(plaintext);
  auto entropy = entropy_blob();
  DATA_BLOB output{};
  if (CryptProtectData(
          &input,
          L"AxLicense machine state v1",
          &entropy,
          nullptr,
          nullptr,
          CRYPTPROTECT_LOCAL_MACHINE | CRYPTPROTECT_UI_FORBIDDEN,
          &output) == FALSE) {
    return {ProtectionOutcome::unexpected_failure, {}};
  }
  return copy_and_free(output);
}

ProtectionResult DpapiProtector::unprotect(const std::vector<std::uint8_t>& protected_bytes) {
  if (protected_bytes.empty() || protected_bytes.size() > std::numeric_limits<DWORD>::max()) {
    return {ProtectionOutcome::corrupt, {}};
  }
  auto input = blob(protected_bytes);
  auto entropy = entropy_blob();
  DATA_BLOB output{};
  if (CryptUnprotectData(
          &input, nullptr, &entropy, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output) == FALSE) {
    return {ProtectionOutcome::corrupt, {}};
  }
  return copy_and_free(output);
}

MachineStateCodec::MachineStateCodec(IDataProtector& protector) : protector_(protector) {}

StateCodecResult MachineStateCodec::encode(const core::IdentityRecord& identity) {
  if (!valid_record(identity)) {
    return {core::StateStoreOutcome::unexpected_failure, std::nullopt, {}};
  }
  const auto provider_kind = identity.provider_kind == core::ProviderKind::tpm ? "tpm" : "software";
  const nlohmann::json plaintext = {
      {"schema_version", {{"major", 1}, {"minor", 0}}},
      {"state_generation", 1},
      {"identity",
       {
           {"scheme_id", identity.scheme_id},
           {"identity_epoch", identity.identity_epoch},
           {"identity_value", identity.identity_value},
           {"provider_kind", provider_kind},
           {"provider_name", identity.provider_name},
           {"key_name", identity.key_name},
       }},
  };
  const auto serialized = plaintext.dump();
  const std::vector<std::uint8_t> bytes(serialized.begin(), serialized.end());
  auto protected_value = protector_.protect(bytes);
  if (protected_value.outcome != ProtectionOutcome::ok) {
    return {core::StateStoreOutcome::unexpected_failure, std::nullopt, {}};
  }
  const nlohmann::json envelope = {
      {"format", "axlicense-machine-state"},
      {"envelope_version", 1},
      {"protected_blob_hex", encode_hex(protected_value.bytes)},
  };
  auto carrier = envelope.dump();
  if (carrier.size() > maximum_carrier_size) {
    return {core::StateStoreOutcome::unexpected_failure, std::nullopt, {}};
  }
  return {core::StateStoreOutcome::ok, identity, std::move(carrier)};
}

StateCodecResult MachineStateCodec::decode(const std::string_view carrier) {
  if (carrier.empty() || carrier.size() > maximum_carrier_size) {
    return {core::StateStoreOutcome::corrupt, std::nullopt, {}};
  }
  try {
    const auto envelope = nlohmann::json::parse(carrier);
    if (!envelope.is_object() || envelope.value("format", "") != "axlicense-machine-state") {
      return {core::StateStoreOutcome::corrupt, std::nullopt, {}};
    }
    if (!envelope.contains("envelope_version") || !envelope["envelope_version"].is_number_unsigned()) {
      return {core::StateStoreOutcome::corrupt, std::nullopt, {}};
    }
    if (envelope["envelope_version"].get<std::uint64_t>() != 1U) {
      return {core::StateStoreOutcome::unsupported, std::nullopt, {}};
    }
    const auto protected_hex = envelope.at("protected_blob_hex").get<std::string>();
    if (!is_lower_hex(protected_hex) || protected_hex.empty()) {
      return {core::StateStoreOutcome::corrupt, std::nullopt, {}};
    }
    const auto unprotected = protector_.unprotect(decode_hex(protected_hex));
    if (unprotected.outcome == ProtectionOutcome::corrupt) {
      return {core::StateStoreOutcome::corrupt, std::nullopt, {}};
    }
    if (unprotected.outcome != ProtectionOutcome::ok) {
      return {core::StateStoreOutcome::unexpected_failure, std::nullopt, {}};
    }
    const std::string plaintext(unprotected.bytes.begin(), unprotected.bytes.end());
    const auto document = nlohmann::json::parse(plaintext);
    const auto& version = document.at("schema_version");
    const auto major = version.at("major").get<std::uint64_t>();
    const auto minor = version.at("minor").get<std::uint64_t>();
    if (major != 1U || minor != 0U) {
      return {core::StateStoreOutcome::unsupported, std::nullopt, {}};
    }
    if (document.at("state_generation").get<std::uint64_t>() != 1U) {
      return {core::StateStoreOutcome::corrupt, std::nullopt, {}};
    }
    const auto& source = document.at("identity");
    const auto kind_name = source.at("provider_kind").get<std::string>();
    if (kind_name != "tpm" && kind_name != "software") {
      return {core::StateStoreOutcome::corrupt, std::nullopt, {}};
    }
    core::IdentityRecord identity{
        .scheme_id = source.at("scheme_id").get<std::string>(),
        .identity_epoch = source.at("identity_epoch").get<std::uint32_t>(),
        .identity_value = source.at("identity_value").get<std::string>(),
        .provider_kind = kind_name == "tpm" ? core::ProviderKind::tpm : core::ProviderKind::software,
        .provider_name = source.at("provider_name").get<std::string>(),
        .key_name = source.at("key_name").get<std::string>(),
    };
    if (!valid_record(identity)) {
      return {core::StateStoreOutcome::corrupt, std::nullopt, {}};
    }
    return {core::StateStoreOutcome::ok, std::move(identity), {}};
  } catch (const nlohmann::json::exception&) {
    return {core::StateStoreOutcome::corrupt, std::nullopt, {}};
  }
}

}  // namespace axlic::windows
