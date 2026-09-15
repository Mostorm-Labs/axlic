#include "axlic/core/identity.hpp"

#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <string>
#include <utility>

namespace {

using axlic::core::IIdentityProvider;
using axlic::core::IMachineStateStore;
using axlic::core::IMutationLock;
using axlic::core::IdentityCode;
using axlic::core::IdentityManager;
using axlic::core::IdentityPolicy;
using axlic::core::IdentityRecord;
using axlic::core::LockOutcome;
using axlic::core::ProviderKind;
using axlic::core::ProviderOutcome;
using axlic::core::ProviderResponse;
using axlic::core::StateRead;
using axlic::core::StateStoreOutcome;

IdentityRecord record_for(const ProviderKind kind, std::string key_name = "pinned-key") {
  return IdentityRecord{
      .scheme_id = kind == ProviderKind::tpm ? "axl-win-cng-tpm-p256-v1" : "axl-win-cng-software-p256-v1",
      .identity_epoch = 1,
      .identity_value = std::string(128, kind == ProviderKind::tpm ? 'a' : 'b'),
      .provider_kind = kind,
      .provider_name = kind == ProviderKind::tpm ? "Microsoft Platform Crypto Provider"
                                                 : "Microsoft Software Key Storage Provider",
      .key_name = std::move(key_name),
  };
}

class FakeProvider final : public IIdentityProvider {
 public:
  explicit FakeProvider(const ProviderKind kind) : kind_(kind) {}

  ProviderKind provider_kind() const noexcept override { return kind_; }
  std::string_view provider_name() const noexcept override { return record_for(kind_).provider_name; }
  std::string_view scheme_id() const noexcept override { return record_for(kind_).scheme_id; }
  ProviderOutcome probe() override {
    ++probe_calls;
    return probe_result;
  }
  ProviderResponse establish(const std::string& key_name) override {
    ++establish_calls;
    auto response = establish_result;
    if (response.outcome == ProviderOutcome::usable && !response.identity) {
      response.identity = record_for(kind_, key_name);
    }
    return response;
  }
  ProviderResponse load(const std::string&) override {
    ++load_calls;
    return load_result;
  }
  void remove(const std::string&) noexcept override { ++remove_calls; }

  ProviderKind kind_;
  ProviderOutcome probe_result{ProviderOutcome::usable};
  ProviderResponse establish_result{ProviderOutcome::usable, std::nullopt};
  ProviderResponse load_result{ProviderOutcome::usable, std::nullopt};
  int probe_calls{};
  int establish_calls{};
  int load_calls{};
  int remove_calls{};
};

class FakeStateStore final : public IMachineStateStore {
 public:
  StateRead read() override {
    ++read_calls;
    if (read_calls == 1 || !second_read) {
      return first_read;
    }
    return *second_read;
  }
  StateStoreOutcome prepare_for_mutation() override {
    ++prepare_calls;
    return prepare_result;
  }
  StateStoreOutcome write(const IdentityRecord& record) override {
    ++write_calls;
    written = record;
    return write_result;
  }

  StateRead first_read{StateStoreOutcome::absent, std::nullopt};
  std::optional<StateRead> second_read;
  StateStoreOutcome prepare_result{StateStoreOutcome::ok};
  StateStoreOutcome write_result{StateStoreOutcome::ok};
  std::optional<IdentityRecord> written;
  int read_calls{};
  int prepare_calls{};
  int write_calls{};
};

class FakeLock final : public IMutationLock {
 public:
  LockOutcome acquire() override {
    ++acquire_calls;
    return result;
  }
  void release() noexcept override { ++release_calls; }

  LockOutcome result{LockOutcome::acquired};
  int acquire_calls{};
  int release_calls{};
};

IdentityManager manager(
    FakeProvider& tpm,
    FakeProvider& software,
    FakeStateStore& state,
    FakeLock& lock,
    const bool allow_fallback = true) {
  return IdentityManager(tpm, software, state, lock, IdentityPolicy{allow_fallback}, [] {
    return std::string("Auditoryworks.AxLicense.Identity.v1.00112233445566778899aabbccddeeff");
  });
}

}  // namespace

TEST_CASE("a usable TPM is selected without establishing a software identity") {
  FakeProvider tpm(ProviderKind::tpm);
  FakeProvider software(ProviderKind::software);
  FakeStateStore state;
  FakeLock lock;

  const auto result = manager(tpm, software, state, lock).ensure_identity();

  REQUIRE(result.code == IdentityCode::ok);
  REQUIRE(result.identity->provider_kind == ProviderKind::tpm);
  REQUIRE(tpm.establish_calls == 1);
  REQUIRE(software.establish_calls == 0);
}

TEST_CASE("a temporarily unavailable TPM fails retryably without software downgrade") {
  FakeProvider tpm(ProviderKind::tpm);
  tpm.probe_result = ProviderOutcome::temporarily_unavailable;
  FakeProvider software(ProviderKind::software);
  FakeStateStore state;
  FakeLock lock;

  const auto result = manager(tpm, software, state, lock).ensure_identity();

  REQUIRE(result.code == IdentityCode::identity_provider_unavailable);
  REQUIRE(result.retryable);
  REQUIRE(software.probe_calls == 0);
  REQUIRE(software.establish_calls == 0);
}

TEST_CASE("a permanently unusable TPM permits policy-approved software establishment") {
  FakeProvider tpm(ProviderKind::tpm);
  tpm.probe_result = ProviderOutcome::absent_or_permanently_unusable;
  FakeProvider software(ProviderKind::software);
  FakeStateStore state;
  FakeLock lock;

  const auto result = manager(tpm, software, state, lock).ensure_identity();

  REQUIRE(result.code == IdentityCode::ok);
  REQUIRE(result.identity->provider_kind == ProviderKind::software);
  REQUIRE(software.establish_calls == 1);
}

TEST_CASE("a TPM that proves permanently unusable during qualification permits software fallback") {
  FakeProvider tpm(ProviderKind::tpm);
  tpm.establish_result = {ProviderOutcome::absent_or_permanently_unusable, std::nullopt};
  FakeProvider software(ProviderKind::software);
  FakeStateStore state;
  FakeLock lock;

  const auto result = manager(tpm, software, state, lock).ensure_identity();

  REQUIRE(result.code == IdentityCode::ok);
  REQUIRE(result.identity->provider_kind == ProviderKind::software);
  REQUIRE(tpm.establish_calls == 1);
  REQUIRE(software.establish_calls == 1);
}

TEST_CASE("fallback policy denial never creates a software identity") {
  FakeProvider tpm(ProviderKind::tpm);
  tpm.probe_result = ProviderOutcome::absent_or_permanently_unusable;
  FakeProvider software(ProviderKind::software);
  FakeStateStore state;
  FakeLock lock;

  const auto result = manager(tpm, software, state, lock, false).ensure_identity();

  REQUIRE(result.code == IdentityCode::identity_provider_unsupported);
  REQUIRE(software.establish_calls == 0);
}

TEST_CASE("a committed TPM identity never falls back after a transient load failure") {
  FakeProvider tpm(ProviderKind::tpm);
  tpm.load_result = {ProviderOutcome::temporarily_unavailable, std::nullopt};
  FakeProvider software(ProviderKind::software);
  FakeStateStore state;
  state.first_read = {StateStoreOutcome::ok, record_for(ProviderKind::tpm)};
  FakeLock lock;

  const auto result = manager(tpm, software, state, lock).ensure_identity();

  REQUIRE(result.code == IdentityCode::identity_provider_unavailable);
  REQUIRE(result.retryable);
  REQUIRE(software.probe_calls == 0);
  REQUIRE(software.establish_calls == 0);
  REQUIRE(lock.acquire_calls == 0);
}

TEST_CASE("a missing committed TPM key requires recovery without creating an identity") {
  FakeProvider tpm(ProviderKind::tpm);
  tpm.load_result = {ProviderOutcome::key_not_found_or_corrupt, std::nullopt};
  FakeProvider software(ProviderKind::software);
  FakeStateStore state;
  state.first_read = {StateStoreOutcome::ok, record_for(ProviderKind::tpm)};
  FakeLock lock;

  const auto result = manager(tpm, software, state, lock).ensure_identity();

  REQUIRE(result.code == IdentityCode::identity_recovery_required);
  REQUIRE(tpm.establish_calls == 0);
  REQUIRE(software.establish_calls == 0);
}

TEST_CASE("a committed software identity remains pinned when TPM later becomes usable") {
  FakeProvider tpm(ProviderKind::tpm);
  FakeProvider software(ProviderKind::software);
  software.load_result = {ProviderOutcome::usable, record_for(ProviderKind::software)};
  FakeStateStore state;
  state.first_read = {StateStoreOutcome::ok, record_for(ProviderKind::software)};
  FakeLock lock;

  const auto result = manager(tpm, software, state, lock).ensure_identity();

  REQUIRE(result.code == IdentityCode::ok);
  REQUIRE(result.identity->provider_kind == ProviderKind::software);
  REQUIRE(tpm.probe_calls == 0);
  REQUIRE(tpm.establish_calls == 0);
  REQUIRE(software.establish_calls == 0);
}
