#include "axlic/core/identity.hpp"
#include "axlic/windows/cng_identity_provider.hpp"
#include "axlic/windows/machine_state.hpp"
#include "axlic/windows/machine_state_repository.hpp"
#include "axlic/windows/mutation_lock.hpp"

#include <nlohmann/json.hpp>

#define NOMINMAX
#include <windows.h>
#include <aclapi.h>
#include <bcrypt.h>
#include <ncrypt.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace {

using Json = nlohmann::json;

class AllowTestRoot final : public axlic::windows::IStateRootSecurity {
 public:
  axlic::core::StateStoreOutcome prepare(const std::filesystem::path& root) override {
    std::error_code error;
    std::filesystem::create_directories(root, error);
    return error ? axlic::core::StateStoreOutcome::unexpected_failure : axlic::core::StateStoreOutcome::ok;
  }
};

class AbsentTpm final : public axlic::core::IIdentityProvider {
 public:
  axlic::core::ProviderKind provider_kind() const noexcept override { return axlic::core::ProviderKind::tpm; }
  std::string_view provider_name() const noexcept override { return "Microsoft Platform Crypto Provider"; }
  std::string_view scheme_id() const noexcept override { return "axl-win-cng-tpm-p256-v1"; }
  axlic::core::ProviderOutcome probe() override {
    return axlic::core::ProviderOutcome::absent_or_permanently_unusable;
  }
  axlic::core::ProviderResponse establish(const std::string&) override {
    return {axlic::core::ProviderOutcome::absent_or_permanently_unusable, std::nullopt};
  }
  axlic::core::ProviderResponse load(const std::string&) override {
    return {axlic::core::ProviderOutcome::key_not_found_or_corrupt, std::nullopt};
  }
  void remove(const std::string&) noexcept override {}
};

class DelayingProvider final : public axlic::core::IIdentityProvider {
 public:
  explicit DelayingProvider(axlic::core::IIdentityProvider& inner) : inner_(inner) {}
  axlic::core::ProviderKind provider_kind() const noexcept override { return inner_.provider_kind(); }
  std::string_view provider_name() const noexcept override { return inner_.provider_name(); }
  std::string_view scheme_id() const noexcept override { return inner_.scheme_id(); }
  axlic::core::ProviderOutcome probe() override { return inner_.probe(); }
  axlic::core::ProviderResponse establish(const std::string& key_name) override {
    Sleep(750);
    return inner_.establish(key_name);
  }
  axlic::core::ProviderResponse load(const std::string& key_name) override { return inner_.load(key_name); }
  void remove(const std::string& key_name) noexcept override { inner_.remove(key_name); }

 private:
  axlic::core::IIdentityProvider& inner_;
};

std::string random_key_name() {
  std::array<std::uint8_t, 16> bytes{};
  if (BCryptGenRandom(nullptr, bytes.data(), static_cast<ULONG>(bytes.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) {
    return {};
  }
  constexpr std::array<char, 16> digits{'0', '1', '2', '3', '4', '5', '6', '7', '8',
                                        '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  std::string result = "Auditoryworks.AxLicense.Identity.v1.";
  for (const auto byte : bytes) {
    result.push_back(digits[byte >> 4U]);
    result.push_back(digits[byte & 0x0fU]);
  }
  return result;
}

axlic::core::IdentityRecord fixture_identity(const char digit) {
  return {
      .scheme_id = "axl-win-cng-software-p256-v1",
      .identity_epoch = 1,
      .identity_value = std::string(128, digit),
      .provider_kind = axlic::core::ProviderKind::software,
      .provider_name = "Microsoft Software Key Storage Provider",
      .key_name = std::string("Auditoryworks.AxLicense.Identity.v1.") + std::string(32, digit),
  };
}

std::optional<axlic::windows::CommitPoint> commit_point(const std::string_view name) {
  if (name == "before_temp_write") {
    return axlic::windows::CommitPoint::before_temp_write;
  }
  if (name == "after_temp_write") {
    return axlic::windows::CommitPoint::after_temp_write;
  }
  if (name == "after_temp_flush") {
    return axlic::windows::CommitPoint::after_temp_flush;
  }
  if (name == "before_replace") {
    return axlic::windows::CommitPoint::before_replace;
  }
  if (name == "after_replace") {
    return axlic::windows::CommitPoint::after_replace;
  }
  return std::nullopt;
}

int emit(const Json& value, const int code = 0) {
  std::cout << value.dump() << '\n';
  return code;
}

int state_write(const std::filesystem::path& root, const char digit, const std::string_view failpoint) {
  axlic::windows::DpapiProtector protector;
  axlic::windows::MachineStateCodec codec(protector);
  AllowTestRoot security;
  const auto selected = commit_point(failpoint);
  axlic::windows::WindowsMachineStateStore store(root, codec, security, [selected](const auto point) {
    if (selected && point == *selected) {
      TerminateProcess(GetCurrentProcess(), 91);
    }
  });
  if (store.prepare_for_mutation() != axlic::core::StateStoreOutcome::ok ||
      store.write(fixture_identity(digit)) != axlic::core::StateStoreOutcome::ok) {
    return emit({{"code", "INTERNAL_ERROR"}}, 4);
  }
  return emit({{"code", "OK"}});
}

std::string_view state_code(const axlic::core::StateStoreOutcome outcome) {
  switch (outcome) {
    case axlic::core::StateStoreOutcome::ok:
      return "OK";
    case axlic::core::StateStoreOutcome::absent:
      return "LOCAL_STATE_ABSENT";
    case axlic::core::StateStoreOutcome::corrupt:
      return "LOCAL_STATE_CORRUPT";
    case axlic::core::StateStoreOutcome::unsupported:
      return "LOCAL_STATE_UNSUPPORTED";
    case axlic::core::StateStoreOutcome::privilege_required:
      return "LOCAL_WRITE_PRIVILEGE_REQUIRED";
    case axlic::core::StateStoreOutcome::unexpected_failure:
      return "INTERNAL_ERROR";
  }
  return "INTERNAL_ERROR";
}

int state_read(const std::filesystem::path& root) {
  axlic::windows::DpapiProtector protector;
  axlic::windows::MachineStateCodec codec(protector);
  AllowTestRoot security;
  axlic::windows::WindowsMachineStateStore store(root, codec, security);
  const auto result = store.read();
  Json output = {{"code", state_code(result.outcome)}};
  if (result.identity) {
    output["identity_value"] = result.identity->identity_value;
  }
  return emit(output, result.outcome == axlic::core::StateStoreOutcome::ok ? 0 : 3);
}

int identity_software(const std::filesystem::path& root, const std::string_view lock_name) {
  axlic::windows::DpapiProtector protector;
  axlic::windows::MachineStateCodec codec(protector);
  AllowTestRoot security;
  axlic::windows::WindowsMachineStateStore store(root, codec, security);
  constexpr auto test_sddl = L"D:P(A;;GA;;;SY)(A;;GA;;;BA)(A;;GA;;;OW)";
  axlic::windows::WindowsMutationLock lock(
      std::wstring(lock_name.begin(), lock_name.end()), test_sddl, 30'000U);
  AbsentTpm tpm;
  axlic::windows::CngIdentityProvider software(axlic::core::ProviderKind::software);
  DelayingProvider delayed_software(software);
  axlic::core::IdentityManager manager(
      tpm,
      delayed_software,
      store,
      lock,
      axlic::core::IdentityPolicy{.allow_software_fallback = true},
      random_key_name);
  const auto result = manager.ensure_identity();
  if (result.code != axlic::core::IdentityCode::ok || !result.identity) {
    return emit({{"code", axlic::core::identity_code_name(result.code)}}, 3);
  }
  return emit({{"code", "OK"},
               {"scheme_id", result.identity->scheme_id},
               {"identity_epoch", result.identity->identity_epoch},
               {"identity_value", result.identity->identity_value}});
}

int count_software_keys() {
  NCRYPT_PROV_HANDLE provider{};
  if (NCryptOpenStorageProvider(&provider, MS_KEY_STORAGE_PROVIDER, 0) != ERROR_SUCCESS) {
    return emit({{"code", "IDENTITY_PROVIDER_UNAVAILABLE"}}, 3);
  }
  constexpr std::wstring_view prefix = L"Auditoryworks.AxLicense.Identity.v1.";
  int count{};
  PVOID enumeration_state{};
  NCryptKeyName* key_name{};
  while (NCryptEnumKeys(
             provider, nullptr, &key_name, &enumeration_state, NCRYPT_MACHINE_KEY_FLAG | NCRYPT_SILENT_FLAG) ==
         ERROR_SUCCESS) {
    if (key_name != nullptr && key_name->pszName != nullptr && std::wstring_view(key_name->pszName).starts_with(prefix)) {
      ++count;
    }
    NCryptFreeBuffer(key_name);
    key_name = nullptr;
  }
  if (key_name != nullptr) {
    NCryptFreeBuffer(key_name);
  }
  if (enumeration_state != nullptr) {
    NCryptFreeBuffer(enumeration_state);
  }
  NCryptFreeObject(provider);
  return emit({{"code", "OK"}, {"count", count}});
}

int cleanup_key(const std::filesystem::path& root) {
  axlic::windows::DpapiProtector protector;
  axlic::windows::MachineStateCodec codec(protector);
  AllowTestRoot security;
  axlic::windows::WindowsMachineStateStore store(root, codec, security);
  const auto state = store.read();
  if (state.outcome != axlic::core::StateStoreOutcome::ok || !state.identity) {
    return emit({{"code", "LOCAL_STATE_CORRUPT"}}, 3);
  }
  axlic::windows::CngIdentityProvider provider(state.identity->provider_kind);
  provider.remove(state.identity->key_name);
  return emit({{"code", "OK"}});
}

int prepare_protected_root(const std::filesystem::path& root) {
  axlic::windows::WindowsStateRootSecurity security;
  const auto result = security.prepare(root);
  if (result != axlic::core::StateStoreOutcome::ok) {
    return emit({{"code", state_code(result)}}, 3);
  }
  PACL dacl{};
  PSECURITY_DESCRIPTOR descriptor{};
  auto mutable_root = root.native();
  if (GetNamedSecurityInfoW(
          mutable_root.data(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, &dacl, nullptr,
          &descriptor) != ERROR_SUCCESS) {
    return emit({{"code", "INTERNAL_ERROR"}}, 4);
  }
  SECURITY_DESCRIPTOR_CONTROL control{};
  DWORD revision{};
  const bool protected_dacl =
      GetSecurityDescriptorControl(descriptor, &control, &revision) != FALSE && (control & SE_DACL_PROTECTED) != 0;
  std::array<std::uint8_t, SECURITY_MAX_SID_SIZE> system_sid{};
  std::array<std::uint8_t, SECURITY_MAX_SID_SIZE> administrators_sid{};
  std::array<std::uint8_t, SECURITY_MAX_SID_SIZE> users_sid{};
  DWORD system_size = static_cast<DWORD>(system_sid.size());
  DWORD administrators_size = static_cast<DWORD>(administrators_sid.size());
  DWORD users_size = static_cast<DWORD>(users_sid.size());
  const bool sids_ready =
      CreateWellKnownSid(WinLocalSystemSid, nullptr, system_sid.data(), &system_size) != FALSE &&
      CreateWellKnownSid(WinBuiltinAdministratorsSid, nullptr, administrators_sid.data(), &administrators_size) !=
          FALSE &&
      CreateWellKnownSid(WinBuiltinUsersSid, nullptr, users_sid.data(), &users_size) != FALSE;
  bool system_full{};
  bool administrators_full{};
  bool users_root_read_execute{};
  bool users_children_read_execute{};
  bool only_expected_aces = dacl != nullptr && dacl->AceCount == 4U;
  if (sids_ready && dacl != nullptr) {
    for (DWORD index = 0; index < dacl->AceCount; ++index) {
      void* raw_ace{};
      if (GetAce(dacl, index, &raw_ace) == FALSE) {
        only_expected_aces = false;
        continue;
      }
      auto* ace = static_cast<ACCESS_ALLOWED_ACE*>(raw_ace);
      auto* sid = &ace->SidStart;
      constexpr auto inheritance = static_cast<BYTE>(OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);
      if (ace->Header.AceType != ACCESS_ALLOWED_ACE_TYPE) {
        only_expected_aces = false;
      } else if (EqualSid(sid, system_sid.data()) != FALSE) {
        system_full = ace->Header.AceFlags == inheritance && ace->Mask == FILE_ALL_ACCESS;
      } else if (EqualSid(sid, administrators_sid.data()) != FALSE) {
        administrators_full = ace->Header.AceFlags == inheritance && ace->Mask == FILE_ALL_ACCESS;
      } else if (EqualSid(sid, users_sid.data()) != FALSE) {
        constexpr auto child_inheritance = static_cast<BYTE>(inheritance | INHERIT_ONLY_ACE);
        if (ace->Header.AceFlags == 0U && ace->Mask == (FILE_GENERIC_READ | FILE_GENERIC_EXECUTE)) {
          users_root_read_execute = true;
        } else if (
            ace->Header.AceFlags == child_inheritance && ace->Mask == (GENERIC_READ | GENERIC_EXECUTE)) {
          users_children_read_execute = true;
        } else {
          only_expected_aces = false;
        }
      } else {
        only_expected_aces = false;
      }
    }
  }
  LocalFree(descriptor);
  const bool valid = protected_dacl && only_expected_aces && system_full && administrators_full &&
                     users_root_read_execute && users_children_read_execute;
  return emit({{"code", valid ? "OK" : "ACL_INVALID"}, {"acl_valid", valid}}, valid ? 0 : 3);
}

}  // namespace

int main(const int argc, const char* const argv[]) {
  try {
    if (argc == 5 && std::string_view(argv[1]) == "state-write" && std::string_view(argv[3]).size() == 1U) {
      return state_write(argv[2], argv[3][0], argv[4]);
    }
    if (argc == 3 && std::string_view(argv[1]) == "state-read") {
      return state_read(argv[2]);
    }
    if (argc == 4 && std::string_view(argv[1]) == "identity-software") {
      return identity_software(argv[2], argv[3]);
    }
    if (argc == 3 && std::string_view(argv[1]) == "cleanup-key") {
      return cleanup_key(argv[2]);
    }
    if (argc == 3 && std::string_view(argv[1]) == "prepare-protected-root") {
      return prepare_protected_root(argv[2]);
    }
    if (argc == 2 && std::string_view(argv[1]) == "count-software-keys") {
      return count_software_keys();
    }
    return emit({{"code", "COMMAND_INVALID"}}, 2);
  } catch (...) {
    return emit({{"code", "INTERNAL_ERROR"}}, 4);
  }
}
