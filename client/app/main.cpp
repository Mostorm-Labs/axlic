#include "axlic/core/a0_fixture_adapter.hpp"
#include "axlic/core/runtime_license.hpp"
#include "axlic/wire/credential.hpp"

#include <nlohmann/json.hpp>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#ifndef AXLIC_A0_FIXTURE_ADAPTER
#error "A0 axlic requires the explicit fixture adapter"
#endif

namespace {

using Json = nlohmann::json;

Json command_result(
    const bool ok,
    std::string_view code,
    std::string_view category,
    std::optional<Json> data = std::nullopt) {
  Json result = {
      {"contract_version", "1.0"},
      {"ok", ok},
      {"code", code},
      {"category", category},
      {"retryable", false},
      {"correlation_ref", "a0-local"},
  };
  if (data) {
    result["data"] = std::move(*data);
  }
  return result;
}

int emit(const Json& result, const int exit_code) {
  std::cout << result.dump() << '\n';
  return exit_code;
}

std::optional<std::filesystem::path> fixture_path() {
  char* raw = nullptr;
  std::size_t length = 0;
  if (_dupenv_s(&raw, &length, "AXLIC_A0_FIXTURE_STATE") != 0 || raw == nullptr || length <= 1U) {
    std::free(raw);
    return std::nullopt;
  }
  const std::unique_ptr<char, decltype(&std::free)> value(raw, &std::free);
  return std::filesystem::path(value.get());
}

axlic::core::RuntimeLicense load_runtime() {
  const auto path = fixture_path();
  if (!path) {
    return axlic::core::RuntimeLicense::absent();
  }
  const auto material = axlic::core::A0FixtureAdapter(*path).load();
  if (!material.present) {
    return axlic::core::RuntimeLicense::absent();
  }
  return axlic::core::RuntimeLicense::from_verification(
      axlic::wire::verify_credential(material.artifact, material.trusted_keys));
}

std::string_view state_name(const axlic::core::CredentialState state) {
  switch (state) {
    case axlic::core::CredentialState::valid:
      return "valid";
    case axlic::core::CredentialState::absent:
      return "absent";
    case axlic::core::CredentialState::invalid:
    case axlic::core::CredentialState::unsupported:
      return "invalid";
  }
  return "invalid";
}

int credential_failure(const axlic::core::CredentialState state) {
  Json data = {{"credential_state", state_name(state)}};
  if (state == axlic::core::CredentialState::absent) {
    return emit(command_result(false, "CREDENTIAL_NOT_FOUND", "credential", data), 3);
  }
  if (state == axlic::core::CredentialState::unsupported) {
    return emit(command_result(false, "CREDENTIAL_UNSUPPORTED", "credential", data), 3);
  }
  return emit(command_result(false, "CREDENTIAL_INVALID", "credential", data), 3);
}

int status_command(const axlic::core::RuntimeLicense& runtime) {
  const auto status = runtime.status();
  if (status.credential_state != axlic::core::CredentialState::valid) {
    return credential_failure(status.credential_state);
  }
  Json data = {
      {"credential_state", "valid"},
      {"authority_revision", *status.authority_revision},
      {"credential_generation", *status.credential_generation},
  };
  return emit(command_result(true, "OK", "none", data), 0);
}

int entitlement_command(const axlic::core::RuntimeLicense& runtime, const std::string_view entitlement_id) {
  const auto status = runtime.status();
  if (status.credential_state != axlic::core::CredentialState::valid) {
    return credential_failure(status.credential_state);
  }
  const auto now = std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();
  const auto entitlement = runtime.entitlement(entitlement_id, now);
  Json data = {{"entitlement_id", entitlement_id}, {"granted", entitlement.granted}};
  if (entitlement.value_u64) {
    data["value_u64"] = *entitlement.value_u64;
  }
  return emit(command_result(true, "OK", "none", data), 0);
}

}  // namespace

int main(const int argc, const char* const argv[]) {
  try {
    if (argc < 2) {
      return emit(command_result(false, "COMMAND_INVALID", "command"), 2);
    }
    const std::string_view command(argv[1]);
    if ((command == "status" && argc != 2) || (command == "entitlement" && argc != 3) ||
        (command != "status" && command != "entitlement")) {
      return emit(command_result(false, "COMMAND_INVALID", "command"), 2);
    }
    const auto runtime = load_runtime();
    if (command == "status") {
      return status_command(runtime);
    }
    return entitlement_command(runtime, argv[2]);
  } catch (...) {
    return emit(command_result(false, "INTERNAL_ERROR", "internal"), 4);
  }
}
