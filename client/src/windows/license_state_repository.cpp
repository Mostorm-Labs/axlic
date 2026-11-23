#include "axlic/windows/license_state_repository.hpp"

#include <nlohmann/json.hpp>

#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace axlic::windows {
namespace {

constexpr std::string_view kEntropy = "AxLicense/LicenseState/v1";
constexpr std::string_view kEnvelopeFormat = "axlicense-license-state";
constexpr std::string_view kStateFileName = "license-state.v1.json";

bool is_lower_hex(const std::string_view value) {
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

class FileHandle {
 public:
  explicit FileHandle(const HANDLE value = INVALID_HANDLE_VALUE) : value_(value) {}
  ~FileHandle() {
    if (value_ != INVALID_HANDLE_VALUE) {
      CloseHandle(value_);
    }
  }
  FileHandle(const FileHandle&) = delete;
  FileHandle& operator=(const FileHandle&) = delete;
  [[nodiscard]] HANDLE get() const noexcept { return value_; }

 private:
  HANDLE value_;
};

bool valid_installed_credential(const core::InstalledCredential& credential) {
  return !credential.credential_id.empty() && !credential.license_grant_id.empty() &&
         core::DeviceReference::parse(credential.device_id).has_value() &&
         credential.authority_revision != 0U && credential.credential_generation != 0U &&
         credential.policy_version != 0U && credential.entitlements.size() <= 256U;
}

nlohmann::json entitlement_to_json(const wire::Entitlement& entitlement) {
  nlohmann::json validity = {{"kind", static_cast<std::uint64_t>(entitlement.validity.kind)}};
  if (entitlement.validity.not_before) {
    validity["not_before"] = *entitlement.validity.not_before;
  }
  if (entitlement.validity.not_after) {
    validity["not_after"] = *entitlement.validity.not_after;
  }
  nlohmann::json entry = {
      {"entitlement_id", entitlement.entitlement_id},
      {"product_id", entitlement.product_id},
      {"right_kind", static_cast<std::uint64_t>(entitlement.right_kind)},
      {"grant_semantics", static_cast<std::uint64_t>(entitlement.grant_semantics)},
      {"validity", std::move(validity)},
  };
  if (entitlement.max_u64) {
    entry["max_u64"] = *entitlement.max_u64;
  }
  return entry;
}

std::optional<wire::Entitlement> entitlement_from_json(const nlohmann::json& source) {
  const auto read_u64 = [](const nlohmann::json& value) -> std::optional<std::uint64_t> {
    if (!value.is_number_unsigned()) {
      return std::nullopt;
    }
    return value.get<std::uint64_t>();
  };
  if (!source.is_object() || !source.contains("entitlement_id") || !source["entitlement_id"].is_string() ||
      !source.contains("product_id") || !source["product_id"].is_string()) {
    return std::nullopt;
  }
  const auto right_kind = read_u64(source.at("right_kind"));
  const auto grant_semantics = read_u64(source.at("grant_semantics"));
  if (!right_kind || *right_kind < 1U || *right_kind > 3U || !grant_semantics || *grant_semantics < 1U ||
      *grant_semantics > 2U) {
    return std::nullopt;
  }
  const auto& validity_source = source.at("validity");
  const auto validity_kind = read_u64(validity_source.at("kind"));
  if (!validity_kind || (*validity_kind != 1U && *validity_kind != 2U)) {
    return std::nullopt;
  }
  wire::Entitlement entitlement{
      .entitlement_id = source.at("entitlement_id").get<std::string>(),
      .product_id = source.at("product_id").get<std::string>(),
      .right_kind = static_cast<wire::RightKind>(*right_kind),
      .grant_semantics = static_cast<wire::GrantSemantics>(*grant_semantics),
      .validity{.kind = static_cast<wire::ValidityKind>(*validity_kind)},
  };
  const auto read_i64 = [](const nlohmann::json& value) -> std::optional<std::int64_t> {
    if (!value.is_number_integer()) {
      return std::nullopt;
    }
    return value.get<std::int64_t>();
  };
  if (validity_source.contains("not_before")) {
    entitlement.validity.not_before = read_i64(validity_source.at("not_before"));
    if (!entitlement.validity.not_before) {
      return std::nullopt;
    }
  }
  if (validity_source.contains("not_after")) {
    entitlement.validity.not_after = read_i64(validity_source.at("not_after"));
    if (!entitlement.validity.not_after) {
      return std::nullopt;
    }
  }
  if (source.contains("max_u64")) {
    const auto max_u64 = read_u64(source.at("max_u64"));
    if (!max_u64) {
      return std::nullopt;
    }
    entitlement.max_u64 = max_u64;
  }
  return entitlement;
}

}  // namespace

LicenseStateCodec::LicenseStateCodec(IDataProtector& protector) : protector_(protector) {}

core::StateStoreOutcome LicenseStateCodec::encode(const core::LicenseState& state, std::string& carrier) const {
  carrier.clear();
  if (state.registration && core::DeviceReference::parse(state.registration->device_id.to_string()) !=
                                state.registration->device_id) {
    return core::StateStoreOutcome::unexpected_failure;
  }
  if (state.credential && !valid_installed_credential(*state.credential)) {
    return core::StateStoreOutcome::unexpected_failure;
  }
  nlohmann::json plaintext = {
      {"schema_version", {{"major", 1}, {"minor", 0}}},
      {"state_generation", 1},
  };
  if (state.registration) {
    plaintext["registration"] = {{"device_id", state.registration->device_id.to_string()}};
  }
  if (state.credential) {
    const auto& credential = *state.credential;
    nlohmann::json entitlements = nlohmann::json::array();
    for (const auto& entitlement : credential.entitlements) {
      entitlements.push_back(entitlement_to_json(entitlement));
    }
    plaintext["credential"] = {
        {"credential_id", credential.credential_id},
        {"license_grant_id", credential.license_grant_id},
        {"binding_id", credential.binding_id},
        {"device_id", credential.device_id},
        {"authority_revision", credential.authority_revision},
        {"credential_generation", credential.credential_generation},
        {"issued_at", credential.issued_at},
        {"policy_version", credential.policy_version},
        {"entitlements", std::move(entitlements)},
    };
  }
  const auto serialized = plaintext.dump();
  const std::vector<std::uint8_t> bytes(serialized.begin(), serialized.end());
  auto protected_value = protector_.protect(bytes);
  if (protected_value.outcome != ProtectionOutcome::ok) {
    return core::StateStoreOutcome::unexpected_failure;
  }
  const nlohmann::json envelope = {
      {"format", kEnvelopeFormat},
      {"envelope_version", 1},
      {"protected_blob_hex", encode_hex(protected_value.bytes)},
  };
  carrier = envelope.dump();
  if (carrier.size() > maximum_carrier_size) {
    carrier.clear();
    return core::StateStoreOutcome::unexpected_failure;
  }
  return core::StateStoreOutcome::ok;
}

core::LicenseStateRead LicenseStateCodec::decode(const std::string_view carrier) const {
  if (carrier.empty() || carrier.size() > maximum_carrier_size) {
    return {core::StateStoreOutcome::corrupt, std::nullopt};
  }
  try {
    const auto envelope = nlohmann::json::parse(carrier);
    if (!envelope.is_object() || envelope.value("format", "") != kEnvelopeFormat) {
      return {core::StateStoreOutcome::corrupt, std::nullopt};
    }
    if (!envelope.contains("envelope_version") || !envelope["envelope_version"].is_number_unsigned()) {
      return {core::StateStoreOutcome::corrupt, std::nullopt};
    }
    if (envelope["envelope_version"].get<std::uint64_t>() != 1U) {
      return {core::StateStoreOutcome::unsupported, std::nullopt};
    }
    const auto protected_hex = envelope.at("protected_blob_hex").get<std::string>();
    if (protected_hex.empty() || !is_lower_hex(protected_hex)) {
      return {core::StateStoreOutcome::corrupt, std::nullopt};
    }
    const auto unprotected = protector_.unprotect(decode_hex(protected_hex));
    if (unprotected.outcome == ProtectionOutcome::corrupt) {
      return {core::StateStoreOutcome::corrupt, std::nullopt};
    }
    if (unprotected.outcome != ProtectionOutcome::ok) {
      return {core::StateStoreOutcome::unexpected_failure, std::nullopt};
    }
    const std::string plaintext(unprotected.bytes.begin(), unprotected.bytes.end());
    const auto document = nlohmann::json::parse(plaintext);
    const auto& version = document.at("schema_version");
    if (version.at("major").get<std::uint64_t>() != 1U ||
        version.at("minor").get<std::uint64_t>() != 0U) {
      return {core::StateStoreOutcome::unsupported, std::nullopt};
    }
    if (document.at("state_generation").get<std::uint64_t>() != 1U) {
      return {core::StateStoreOutcome::corrupt, std::nullopt};
    }
    core::LicenseState state;
    if (document.contains("registration")) {
      const auto& registration = document.at("registration");
      const auto device_id = core::DeviceReference::parse(registration.at("device_id").get<std::string>());
      if (!device_id) {
        return {core::StateStoreOutcome::corrupt, std::nullopt};
      }
      state.registration = core::RegistrationAssociation{.device_id = *device_id};
    }
    if (document.contains("credential")) {
      const auto& source = document.at("credential");
      const auto& entitlements_source = source.at("entitlements");
      if (!entitlements_source.is_array()) {
        return {core::StateStoreOutcome::corrupt, std::nullopt};
      }
      core::InstalledCredential credential{
          .credential_id = source.at("credential_id").get<std::string>(),
          .license_grant_id = source.at("license_grant_id").get<std::string>(),
          .binding_id = source.at("binding_id").get<std::string>(),
          .device_id = source.at("device_id").get<std::string>(),
          .authority_revision = source.at("authority_revision").get<std::uint64_t>(),
          .credential_generation = source.at("credential_generation").get<std::uint64_t>(),
          .issued_at = source.at("issued_at").get<std::int64_t>(),
          .policy_version = source.at("policy_version").get<std::uint64_t>(),
      };
      for (const auto& entry : entitlements_source) {
        auto entitlement = entitlement_from_json(entry);
        if (!entitlement) {
          return {core::StateStoreOutcome::corrupt, std::nullopt};
        }
        credential.entitlements.push_back(std::move(*entitlement));
      }
      if (!valid_installed_credential(credential)) {
        return {core::StateStoreOutcome::corrupt, std::nullopt};
      }
      state.credential = std::move(credential);
    }
    return {core::StateStoreOutcome::ok, std::move(state)};
  } catch (const nlohmann::json::exception&) {
    return {core::StateStoreOutcome::corrupt, std::nullopt};
  }
}

WindowsLicenseStateStore::WindowsLicenseStateStore(
    std::filesystem::path root,
    LicenseStateCodec& codec,
    IStateRootSecurity& security,
    CommitObserver observer)
    : root_(std::move(root)), codec_(codec), security_(security), observer_(std::move(observer)) {}

const std::filesystem::path& WindowsLicenseStateStore::root() const noexcept {
  return root_;
}

std::filesystem::path WindowsLicenseStateStore::state_path() const {
  return root_ / kStateFileName;
}

std::filesystem::path WindowsLicenseStateStore::temporary_path() const {
  constexpr std::array<char, 16> digits{'0', '1', '2', '3', '4', '5', '6', '7', '8',
                                        '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  std::array<std::uint8_t, 8> bytes{};
  if (BCryptGenRandom(nullptr, bytes.data(), static_cast<ULONG>(bytes.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) {
    return {};
  }
  std::string suffix;
  suffix.reserve(bytes.size() * 2U);
  for (const auto byte : bytes) {
    suffix.push_back(digits[byte >> 4U]);
    suffix.push_back(digits[byte & 0x0fU]);
  }
  return root_ /
         (".license-state.v1." + std::to_string(GetCurrentProcessId()) + "." + suffix + ".tmp");
}

core::LicenseStateRead WindowsLicenseStateStore::read() {
  const auto path = state_path();
  const FileHandle file(CreateFileW(
      path.c_str(),
      GENERIC_READ,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
      nullptr,
      OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
      nullptr));
  if (file.get() == INVALID_HANDLE_VALUE) {
    const auto error = GetLastError();
    return (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
               ? core::LicenseStateRead{core::StateStoreOutcome::absent, std::nullopt}
               : core::LicenseStateRead{core::StateStoreOutcome::unexpected_failure, std::nullopt};
  }
  LARGE_INTEGER size{};
  if (GetFileSizeEx(file.get(), &size) == FALSE || size.QuadPart <= 0 ||
      size.QuadPart > static_cast<LONGLONG>(LicenseStateCodec::maximum_carrier_size)) {
    return {core::StateStoreOutcome::corrupt, std::nullopt};
  }
  std::string carrier(static_cast<std::size_t>(size.QuadPart), '\0');
  DWORD read{};
  if (ReadFile(file.get(), carrier.data(), static_cast<DWORD>(carrier.size()), &read, nullptr) == FALSE ||
      read != carrier.size()) {
    return {core::StateStoreOutcome::unexpected_failure, std::nullopt};
  }
  return codec_.decode(carrier);
}

core::StateStoreOutcome WindowsLicenseStateStore::prepare_for_mutation() {
  const auto prepared = security_.prepare(root_);
  if (prepared != core::StateStoreOutcome::ok) {
    return prepared;
  }
  const auto candidate = temporary_path();
  if (candidate.empty()) {
    return core::StateStoreOutcome::unexpected_failure;
  }
  {
    const FileHandle probe(CreateFileW(
        candidate.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_NEW,
        FILE_ATTRIBUTE_TEMPORARY,
        nullptr));
    if (probe.get() == INVALID_HANDLE_VALUE) {
      return GetLastError() == ERROR_ACCESS_DENIED ? core::StateStoreOutcome::privilege_required
                                                   : core::StateStoreOutcome::unexpected_failure;
    }
  }
  if (DeleteFileW(candidate.c_str()) == FALSE) {
    return core::StateStoreOutcome::unexpected_failure;
  }
  return core::StateStoreOutcome::ok;
}

core::StateStoreOutcome WindowsLicenseStateStore::write(const core::LicenseState& state) {
  std::string carrier;
  const auto encoded = codec_.encode(state, carrier);
  if (encoded != core::StateStoreOutcome::ok) {
    return encoded;
  }
  const auto candidate = temporary_path();
  if (candidate.empty()) {
    return core::StateStoreOutcome::unexpected_failure;
  }
  const auto cleanup = [&candidate]() noexcept {
    std::error_code error;
    std::filesystem::remove(candidate, error);
  };
  try {
    if (observer_) {
      observer_(CommitPoint::before_temp_write);
    }
    {
      const FileHandle file(CreateFileW(
          candidate.c_str(),
          GENERIC_WRITE,
          0,
          nullptr,
          CREATE_NEW,
          FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
          nullptr));
      if (file.get() == INVALID_HANDLE_VALUE) {
        return GetLastError() == ERROR_ACCESS_DENIED ? core::StateStoreOutcome::privilege_required
                                                     : core::StateStoreOutcome::unexpected_failure;
      }
      DWORD written{};
      const auto complete = carrier.size() <= MAXDWORD &&
                            WriteFile(file.get(), carrier.data(), static_cast<DWORD>(carrier.size()), &written,
                                      nullptr) != FALSE &&
                            written == carrier.size();
      if (!complete) {
        cleanup();
        return GetLastError() == ERROR_ACCESS_DENIED ? core::StateStoreOutcome::privilege_required
                                                     : core::StateStoreOutcome::unexpected_failure;
      }
      if (observer_) {
        observer_(CommitPoint::after_temp_write);
      }
      if (FlushFileBuffers(file.get()) == FALSE) {
        cleanup();
        return core::StateStoreOutcome::unexpected_failure;
      }
      if (observer_) {
        observer_(CommitPoint::after_temp_flush);
      }
    }
    if (observer_) {
      observer_(CommitPoint::before_replace);
    }
    if (MoveFileExW(
            candidate.c_str(), state_path().c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == FALSE) {
      cleanup();
      return GetLastError() == ERROR_ACCESS_DENIED ? core::StateStoreOutcome::privilege_required
                                                   : core::StateStoreOutcome::unexpected_failure;
    }
    if (observer_) {
      observer_(CommitPoint::after_replace);
    }
    return core::StateStoreOutcome::ok;
  } catch (...) {
    cleanup();
    return core::StateStoreOutcome::unexpected_failure;
  }
}

}  // namespace axlic::windows
