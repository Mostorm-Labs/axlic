#include "axlic/windows/machine_state_repository.hpp"

#define NOMINMAX
#include <windows.h>
#include <aclapi.h>
#include <bcrypt.h>
#include <sddl.h>
#include <shlobj.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace axlic::windows {
namespace {

constexpr wchar_t kRootSddl[] = L"D:P(A;OICI;FA;;;SY)(A;OICI;FA;;;BA)(A;OICI;GRGX;;;BU)";

class LocalAllocation {
 public:
  ~LocalAllocation() {
    if (value_ != nullptr) {
      LocalFree(value_);
    }
  }
  void** put() noexcept { return &value_; }
  [[nodiscard]] void* get() const noexcept { return value_; }

 private:
  void* value_{};
};

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

std::string random_hex() {
  std::array<std::uint8_t, 8> bytes{};
  if (BCryptGenRandom(nullptr, bytes.data(), static_cast<ULONG>(bytes.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) {
    return {};
  }
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

core::StateStoreOutcome win32_write_failure() {
  return GetLastError() == ERROR_ACCESS_DENIED ? core::StateStoreOutcome::privilege_required
                                               : core::StateStoreOutcome::unexpected_failure;
}

bool write_complete(const HANDLE file, const std::string& bytes) {
  DWORD written{};
  return bytes.size() <= MAXDWORD &&
         WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) != FALSE &&
         written == bytes.size();
}

}  // namespace

core::StateStoreOutcome WindowsStateRootSecurity::prepare(const std::filesystem::path& root) {
  std::error_code error;
  std::filesystem::create_directories(root, error);
  if (error) {
    return error.value() == ERROR_ACCESS_DENIED ? core::StateStoreOutcome::privilege_required
                                                : core::StateStoreOutcome::unexpected_failure;
  }

  LocalAllocation descriptor;
  if (ConvertStringSecurityDescriptorToSecurityDescriptorW(
          kRootSddl,
          SDDL_REVISION_1,
          reinterpret_cast<PSECURITY_DESCRIPTOR*>(descriptor.put()),
          nullptr) == FALSE) {
    return core::StateStoreOutcome::unexpected_failure;
  }
  BOOL dacl_present{};
  BOOL dacl_defaulted{};
  PACL dacl{};
  if (GetSecurityDescriptorDacl(descriptor.get(), &dacl_present, &dacl, &dacl_defaulted) == FALSE ||
      dacl_present == FALSE) {
    return core::StateStoreOutcome::unexpected_failure;
  }
  auto mutable_path = root.native();
  const auto status = SetNamedSecurityInfoW(
      mutable_path.data(),
      SE_FILE_OBJECT,
      DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
      nullptr,
      nullptr,
      dacl,
      nullptr);
  if (status == ERROR_ACCESS_DENIED) {
    return core::StateStoreOutcome::privilege_required;
  }
  return status == ERROR_SUCCESS ? core::StateStoreOutcome::ok : core::StateStoreOutcome::unexpected_failure;
}

WindowsMachineStateStore::WindowsMachineStateStore(
    std::filesystem::path root,
    MachineStateCodec& codec,
    IStateRootSecurity& security,
    CommitObserver observer)
    : root_(std::move(root)), codec_(codec), security_(security), observer_(std::move(observer)) {}

const std::filesystem::path& WindowsMachineStateStore::root() const noexcept {
  return root_;
}

std::filesystem::path WindowsMachineStateStore::state_path() const {
  return root_ / "machine-state.v1.json";
}

std::filesystem::path WindowsMachineStateStore::temporary_path() const {
  const auto suffix = random_hex();
  if (suffix.empty()) {
    return {};
  }
  return root_ /
         (".machine-state.v1." + std::to_string(GetCurrentProcessId()) + "." + suffix + ".tmp");
}

core::StateRead WindowsMachineStateStore::read() {
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
               ? core::StateRead{core::StateStoreOutcome::absent, std::nullopt}
               : core::StateRead{core::StateStoreOutcome::unexpected_failure, std::nullopt};
  }
  LARGE_INTEGER size{};
  if (GetFileSizeEx(file.get(), &size) == FALSE || size.QuadPart <= 0 ||
      size.QuadPart > static_cast<LONGLONG>(MachineStateCodec::maximum_carrier_size)) {
    return {core::StateStoreOutcome::corrupt, std::nullopt};
  }
  std::string carrier(static_cast<std::size_t>(size.QuadPart), '\0');
  DWORD read{};
  if (ReadFile(file.get(), carrier.data(), static_cast<DWORD>(carrier.size()), &read, nullptr) == FALSE ||
      read != carrier.size()) {
    return {core::StateStoreOutcome::unexpected_failure, std::nullopt};
  }
  const auto decoded = codec_.decode(carrier);
  return {decoded.outcome, decoded.identity};
}

core::StateStoreOutcome WindowsMachineStateStore::prepare_for_mutation() {
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
      return win32_write_failure();
    }
  }
  if (DeleteFileW(candidate.c_str()) == FALSE) {
    return core::StateStoreOutcome::unexpected_failure;
  }
  return core::StateStoreOutcome::ok;
}

core::StateStoreOutcome WindowsMachineStateStore::write(const core::IdentityRecord& identity) {
  const auto encoded = codec_.encode(identity);
  if (encoded.outcome != core::StateStoreOutcome::ok) {
    return encoded.outcome;
  }
  const auto candidate = temporary_path();
  if (candidate.empty()) {
    return core::StateStoreOutcome::unexpected_failure;
  }
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
        return win32_write_failure();
      }
      if (!write_complete(file.get(), encoded.carrier)) {
        DeleteFileW(candidate.c_str());
        return win32_write_failure();
      }
      if (observer_) {
        observer_(CommitPoint::after_temp_write);
      }
      if (FlushFileBuffers(file.get()) == FALSE) {
        DeleteFileW(candidate.c_str());
        return win32_write_failure();
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
      DeleteFileW(candidate.c_str());
      return win32_write_failure();
    }
    if (observer_) {
      observer_(CommitPoint::after_replace);
    }
    return core::StateStoreOutcome::ok;
  } catch (...) {
    DeleteFileW(candidate.c_str());
    return core::StateStoreOutcome::unexpected_failure;
  }
}

std::filesystem::path program_data_state_root() {
  PWSTR raw{};
  if (SHGetKnownFolderPath(FOLDERID_ProgramData, KF_FLAG_DEFAULT, nullptr, &raw) != S_OK || raw == nullptr) {
    return {};
  }
  const std::filesystem::path result(raw);
  CoTaskMemFree(raw);
  return result / "Auditoryworks" / "AxLicense";
}

}  // namespace axlic::windows
