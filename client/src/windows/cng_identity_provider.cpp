#include "axlic/windows/cng_identity_provider.hpp"

#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <ncrypt.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace axlic::windows {
namespace {

constexpr std::string_view kTpmProviderName = "Microsoft Platform Crypto Provider";
constexpr std::string_view kSoftwareProviderName = "Microsoft Software Key Storage Provider";
constexpr std::string_view kTpmScheme = "axl-win-cng-tpm-p256-v1";
constexpr std::string_view kSoftwareScheme = "axl-win-cng-software-p256-v1";
constexpr std::string_view kQualificationDomain = "AxLicense/IdentityQualification/v1";

const wchar_t* wide_provider_name(const core::ProviderKind kind) {
  return kind == core::ProviderKind::tpm ? MS_PLATFORM_CRYPTO_PROVIDER : MS_KEY_STORAGE_PROVIDER;
}

std::wstring widen_ascii(const std::string_view value) {
  return std::wstring(value.begin(), value.end());
}

class NcryptHandle {
 public:
  NcryptHandle() = default;
  explicit NcryptHandle(const NCRYPT_HANDLE handle) : handle_(handle) {}
  ~NcryptHandle() {
    if (handle_ != 0U) {
      NCryptFreeObject(handle_);
    }
  }
  NcryptHandle(const NcryptHandle&) = delete;
  NcryptHandle& operator=(const NcryptHandle&) = delete;
  NcryptHandle(NcryptHandle&& other) noexcept : handle_(std::exchange(other.handle_, 0U)) {}
  NcryptHandle& operator=(NcryptHandle&& other) noexcept {
    if (this != &other) {
      if (handle_ != 0U) {
        NCryptFreeObject(handle_);
      }
      handle_ = std::exchange(other.handle_, 0U);
    }
    return *this;
  }
  [[nodiscard]] NCRYPT_HANDLE get() const noexcept { return handle_; }
  NCRYPT_HANDLE* put() noexcept { return &handle_; }
  NCRYPT_HANDLE release() noexcept { return std::exchange(handle_, 0U); }

 private:
  NCRYPT_HANDLE handle_{};
};

class BcryptAlgorithm {
 public:
  ~BcryptAlgorithm() {
    if (handle_ != nullptr) {
      BCryptCloseAlgorithmProvider(handle_, 0);
    }
  }
  BcryptAlgorithm(const BcryptAlgorithm&) = delete;
  BcryptAlgorithm& operator=(const BcryptAlgorithm&) = delete;
  BcryptAlgorithm() = default;
  BCRYPT_ALG_HANDLE* put() noexcept { return &handle_; }
  [[nodiscard]] BCRYPT_ALG_HANDLE get() const noexcept { return handle_; }

 private:
  BCRYPT_ALG_HANDLE handle_{};
};

std::array<std::uint8_t, 32> qualification_digest() {
  std::array<std::uint8_t, 32> digest{};
  BcryptAlgorithm algorithm;
  if (BCryptOpenAlgorithmProvider(algorithm.put(), BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) {
    return {};
  }
  const auto status = BCryptHash(
      algorithm.get(),
      nullptr,
      0,
      reinterpret_cast<PUCHAR>(const_cast<char*>(kQualificationDomain.data())),
      static_cast<ULONG>(kQualificationDomain.size()),
      digest.data(),
      static_cast<ULONG>(digest.size()));
  if (status != 0) {
    return {};
  }
  return digest;
}

bool qualifies(const NCRYPT_KEY_HANDLE key) {
  const auto digest = qualification_digest();
  if (digest == std::array<std::uint8_t, 32>{}) {
    return false;
  }
  DWORD signature_size{};
  auto status = NCryptSignHash(
      key,
      nullptr,
      const_cast<PBYTE>(digest.data()),
      static_cast<DWORD>(digest.size()),
      nullptr,
      0,
      &signature_size,
      NCRYPT_SILENT_FLAG);
  if (status != ERROR_SUCCESS || signature_size != 64U) {
    return false;
  }
  std::vector<std::uint8_t> signature(signature_size);
  status = NCryptSignHash(
      key,
      nullptr,
      const_cast<PBYTE>(digest.data()),
      static_cast<DWORD>(digest.size()),
      signature.data(),
      static_cast<DWORD>(signature.size()),
      &signature_size,
      NCRYPT_SILENT_FLAG);
  if (status != ERROR_SUCCESS || signature_size != signature.size()) {
    return false;
  }
  status = NCryptVerifySignature(
      key,
      nullptr,
      const_cast<PBYTE>(digest.data()),
      static_cast<DWORD>(digest.size()),
      signature.data(),
      signature_size,
      NCRYPT_SILENT_FLAG);
  if (status != ERROR_SUCCESS) {
    return false;
  }
  DWORD private_blob_size{};
  return NCryptExportKey(
             key, 0, BCRYPT_ECCPRIVATE_BLOB, nullptr, nullptr, 0, &private_blob_size, NCRYPT_SILENT_FLAG) !=
         ERROR_SUCCESS;
}

std::optional<std::string> public_identity(const NCRYPT_KEY_HANDLE key) {
  DWORD blob_size{};
  auto status = NCryptExportKey(
      key, 0, BCRYPT_ECCPUBLIC_BLOB, nullptr, nullptr, 0, &blob_size, NCRYPT_SILENT_FLAG);
  if (status != ERROR_SUCCESS || blob_size != sizeof(BCRYPT_ECCKEY_BLOB) + 64U) {
    return std::nullopt;
  }
  std::vector<std::uint8_t> blob_bytes(blob_size);
  status = NCryptExportKey(
      key, 0, BCRYPT_ECCPUBLIC_BLOB, nullptr, blob_bytes.data(), blob_size, &blob_size, NCRYPT_SILENT_FLAG);
  if (status != ERROR_SUCCESS || blob_size != blob_bytes.size()) {
    return std::nullopt;
  }
  const auto* header = reinterpret_cast<const BCRYPT_ECCKEY_BLOB*>(blob_bytes.data());
  if (header->dwMagic != BCRYPT_ECDSA_PUBLIC_P256_MAGIC || header->cbKey != 32U) {
    return std::nullopt;
  }
  constexpr std::array<char, 16> digits{'0', '1', '2', '3', '4', '5', '6', '7', '8',
                                        '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  std::string result;
  result.reserve(128U);
  for (auto iterator = blob_bytes.begin() + static_cast<std::ptrdiff_t>(sizeof(BCRYPT_ECCKEY_BLOB));
       iterator != blob_bytes.end();
       ++iterator) {
    result.push_back(digits[*iterator >> 4U]);
    result.push_back(digits[*iterator & 0x0fU]);
  }
  return result;
}

core::ProviderResponse response_from_key(
    const core::ProviderKind kind,
    const NCRYPT_KEY_HANDLE key,
    const std::string& key_name) {
  const auto identity_value = public_identity(key);
  if (!identity_value || !qualifies(key)) {
    return {core::ProviderOutcome::unexpected_failure, std::nullopt};
  }
  return {
      core::ProviderOutcome::usable,
      core::IdentityRecord{
          .scheme_id = std::string(kind == core::ProviderKind::tpm ? kTpmScheme : kSoftwareScheme),
          .identity_epoch = 1,
          .identity_value = *identity_value,
          .provider_kind = kind,
          .provider_name = std::string(kind == core::ProviderKind::tpm ? kTpmProviderName : kSoftwareProviderName),
          .key_name = key_name,
      },
  };
}

}  // namespace

core::ProviderOutcome map_cng_status(const long status, const CngOperation operation) noexcept {
  if (status == ERROR_SUCCESS) {
    return core::ProviderOutcome::usable;
  }
  if (status == NTE_DEVICE_NOT_READY || status == NTE_SILENT_CONTEXT) {
    return core::ProviderOutcome::temporarily_unavailable;
  }
  if (status == NTE_PERM) {
    return core::ProviderOutcome::access_denied;
  }
  if ((status == NTE_BAD_KEYSET || status == NTE_BAD_KEY_STATE) && operation == CngOperation::load) {
    return core::ProviderOutcome::key_not_found_or_corrupt;
  }
  if (operation != CngOperation::load &&
      (status == NTE_PROV_TYPE_NOT_DEF || status == NTE_BAD_PROVIDER || status == NTE_NOT_SUPPORTED ||
       status == NTE_BAD_KEYSET || status == NTE_DEVICE_NOT_FOUND)) {
    return core::ProviderOutcome::absent_or_permanently_unusable;
  }
  return core::ProviderOutcome::unexpected_failure;
}

CngIdentityProvider::CngIdentityProvider(const core::ProviderKind kind) : kind_(kind) {}

core::ProviderKind CngIdentityProvider::provider_kind() const noexcept {
  return kind_;
}

std::string_view CngIdentityProvider::provider_name() const noexcept {
  return kind_ == core::ProviderKind::tpm ? kTpmProviderName : kSoftwareProviderName;
}

std::string_view CngIdentityProvider::scheme_id() const noexcept {
  return kind_ == core::ProviderKind::tpm ? kTpmScheme : kSoftwareScheme;
}

core::ProviderOutcome CngIdentityProvider::probe() {
  NcryptHandle provider;
  const auto status = NCryptOpenStorageProvider(
      reinterpret_cast<NCRYPT_PROV_HANDLE*>(provider.put()), wide_provider_name(kind_), 0);
  return map_cng_status(status, CngOperation::probe);
}

core::ProviderResponse CngIdentityProvider::establish(const std::string& key_name) {
  NcryptHandle provider;
  auto status = NCryptOpenStorageProvider(
      reinterpret_cast<NCRYPT_PROV_HANDLE*>(provider.put()), wide_provider_name(kind_), 0);
  if (status != ERROR_SUCCESS) {
    return {map_cng_status(status, CngOperation::establish), std::nullopt};
  }
  NcryptHandle key;
  const auto wide_key_name = widen_ascii(key_name);
  status = NCryptCreatePersistedKey(
      provider.get(),
      reinterpret_cast<NCRYPT_KEY_HANDLE*>(key.put()),
      NCRYPT_ECDSA_P256_ALGORITHM,
      wide_key_name.c_str(),
      0,
      NCRYPT_MACHINE_KEY_FLAG);
  if (status != ERROR_SUCCESS) {
    return {map_cng_status(status, CngOperation::establish), std::nullopt};
  }
  DWORD export_policy{};
  status = NCryptSetProperty(
      key.get(),
      NCRYPT_EXPORT_POLICY_PROPERTY,
      reinterpret_cast<PBYTE>(&export_policy),
      sizeof(export_policy),
      NCRYPT_PERSIST_FLAG | NCRYPT_SILENT_FLAG);
  if (status == ERROR_SUCCESS) {
    status = NCryptFinalizeKey(key.get(), NCRYPT_SILENT_FLAG);
  }
  if (status != ERROR_SUCCESS) {
    NCryptDeleteKey(key.release(), 0);
    return {map_cng_status(status, CngOperation::establish), std::nullopt};
  }
  auto response = response_from_key(kind_, key.get(), key_name);
  if (response.outcome != core::ProviderOutcome::usable) {
    NCryptDeleteKey(key.release(), 0);
  }
  return response;
}

core::ProviderResponse CngIdentityProvider::load(const std::string& key_name) {
  NcryptHandle provider;
  auto status = NCryptOpenStorageProvider(
      reinterpret_cast<NCRYPT_PROV_HANDLE*>(provider.put()), wide_provider_name(kind_), 0);
  if (status != ERROR_SUCCESS) {
    return {map_cng_status(status, CngOperation::load), std::nullopt};
  }
  NcryptHandle key;
  const auto wide_key_name = widen_ascii(key_name);
  status = NCryptOpenKey(
      provider.get(),
      reinterpret_cast<NCRYPT_KEY_HANDLE*>(key.put()),
      wide_key_name.c_str(),
      0,
      NCRYPT_MACHINE_KEY_FLAG | NCRYPT_SILENT_FLAG);
  if (status != ERROR_SUCCESS) {
    return {map_cng_status(status, CngOperation::load), std::nullopt};
  }
  return response_from_key(kind_, key.get(), key_name);
}

void CngIdentityProvider::remove(const std::string& key_name) noexcept {
  NcryptHandle provider;
  if (NCryptOpenStorageProvider(
          reinterpret_cast<NCRYPT_PROV_HANDLE*>(provider.put()), wide_provider_name(kind_), 0) != ERROR_SUCCESS) {
    return;
  }
  NcryptHandle key;
  const auto wide_key_name = widen_ascii(key_name);
  if (NCryptOpenKey(
          provider.get(), reinterpret_cast<NCRYPT_KEY_HANDLE*>(key.put()), wide_key_name.c_str(), 0,
          NCRYPT_MACHINE_KEY_FLAG | NCRYPT_SILENT_FLAG) == ERROR_SUCCESS) {
    NCryptDeleteKey(key.release(), 0);
  }
}

}  // namespace axlic::windows
