#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace axlic::core {

enum class ProviderKind { tpm, software };

enum class ProviderOutcome {
  usable,
  absent_or_permanently_unusable,
  temporarily_unavailable,
  key_not_found_or_corrupt,
  access_denied,
  unexpected_failure,
};

struct IdentityRecord {
  std::string scheme_id;
  std::uint32_t identity_epoch{1};
  std::string identity_value;
  ProviderKind provider_kind{ProviderKind::tpm};
  std::string provider_name;
  std::string key_name;

  bool operator==(const IdentityRecord&) const = default;
};

struct ProviderResponse {
  ProviderOutcome outcome{ProviderOutcome::unexpected_failure};
  std::optional<IdentityRecord> identity;
};

class IIdentityProvider {
 public:
  virtual ~IIdentityProvider() = default;
  [[nodiscard]] virtual ProviderKind provider_kind() const noexcept = 0;
  [[nodiscard]] virtual std::string_view provider_name() const noexcept = 0;
  [[nodiscard]] virtual std::string_view scheme_id() const noexcept = 0;
  virtual ProviderOutcome probe() = 0;
  virtual ProviderResponse establish(const std::string& key_name) = 0;
  virtual ProviderResponse load(const std::string& key_name) = 0;
  virtual void remove(const std::string& key_name) noexcept = 0;
};

enum class StateStoreOutcome { ok, absent, corrupt, unsupported, privilege_required, unexpected_failure };

struct StateRead {
  StateStoreOutcome outcome{StateStoreOutcome::unexpected_failure};
  std::optional<IdentityRecord> identity;
};

class IMachineStateStore {
 public:
  virtual ~IMachineStateStore() = default;
  virtual StateRead read() = 0;
  virtual StateStoreOutcome prepare_for_mutation() = 0;
  virtual StateStoreOutcome write(const IdentityRecord& identity) = 0;
};

enum class LockOutcome { acquired, busy, access_denied, unexpected_failure };

class IMutationLock {
 public:
  virtual ~IMutationLock() = default;
  virtual LockOutcome acquire() = 0;
  virtual void release() noexcept = 0;
};

struct IdentityPolicy {
  bool allow_software_fallback{true};
};

enum class IdentityCode {
  ok,
  local_write_privilege_required,
  local_state_busy,
  local_state_corrupt,
  local_state_unsupported,
  identity_provider_unavailable,
  identity_provider_unsupported,
  identity_recovery_required,
  internal_error,
};

struct IdentityResult {
  IdentityCode code{IdentityCode::internal_error};
  bool retryable{};
  std::optional<IdentityRecord> identity;
};

using KeyNameGenerator = std::function<std::string()>;

class IdentityManager {
 public:
  IdentityManager(
      IIdentityProvider& tpm,
      IIdentityProvider& software,
      IMachineStateStore& state,
      IMutationLock& lock,
      IdentityPolicy policy,
      KeyNameGenerator key_name_generator);

  [[nodiscard]] IdentityResult ensure_identity();

 private:
  [[nodiscard]] IdentityResult load_committed(const IdentityRecord& committed);
  [[nodiscard]] IdentityResult establish_with(IIdentityProvider& provider);

  IIdentityProvider& tpm_;
  IIdentityProvider& software_;
  IMachineStateStore& state_;
  IMutationLock& lock_;
  IdentityPolicy policy_;
  KeyNameGenerator key_name_generator_;
};

[[nodiscard]] std::string_view identity_code_name(IdentityCode code) noexcept;

}  // namespace axlic::core
