#include "axlic/core/license_command.hpp"

#include "axlic/core/command_envelope.hpp"
#include "axlic/core/runtime_license.hpp"

#include <nlohmann/json.hpp>

#include <optional>
#include <string_view>
#include <utility>

namespace axlic::core {
namespace {

using Json = nlohmann::json;

constexpr std::string_view kCorrelationRef = "a2-local";

Json command_result(
    const bool ok,
    const std::string_view code,
    const std::string_view category,
    const bool retryable = false,
    std::optional<Json> data = std::nullopt) {
  return axlic::core::command_result(kCorrelationRef, ok, code, category, retryable, std::move(data));
}

RenderedCommand emit(const Json& result, const int exit_code) {
  return {result.dump(), exit_code};
}

// Reads license state; nullopt signals an error already rendered.
std::optional<std::optional<LicenseState>> read_license_state(
    ILicenseStateStore& store,
    RenderedCommand& on_error) {
  const auto read = store.read();
  switch (read.outcome) {
    case StateStoreOutcome::ok:
      return std::optional<std::optional<LicenseState>>(read.state);
    case StateStoreOutcome::absent:
      // Wrapping is explicit: constructing the outer optional from nullopt_t
      // would disengage the outer itself, which callers read as an error.
      return std::optional<std::optional<LicenseState>>(std::optional<LicenseState>{});
    case StateStoreOutcome::corrupt:
      on_error = emit(command_result(false, "LOCAL_STATE_CORRUPT", "local_state"), kExitCredentialFailure);
      return std::nullopt;
    case StateStoreOutcome::unsupported:
      on_error = emit(command_result(false, "LOCAL_STATE_UNSUPPORTED", "local_state"), kExitCredentialFailure);
      return std::nullopt;
    case StateStoreOutcome::privilege_required:
      on_error = emit(
          command_result(false, "LOCAL_WRITE_PRIVILEGE_REQUIRED", "local_state"), kExitCredentialFailure);
      return std::nullopt;
    case StateStoreOutcome::unexpected_failure:
      break;
  }
  on_error = emit(command_result(false, "INTERNAL_ERROR", "internal"), kExitInternal);
  return std::nullopt;
}

RenderedCommand install_failure(const InstallCode code) {
  if (code == InstallCode::local_state_busy) {
    return emit(command_result(false, "LOCAL_STATE_BUSY", "local_state", true), kExitRetryable);
  }
  if (code == InstallCode::local_state_corrupt || code == InstallCode::local_state_unsupported ||
      code == InstallCode::local_write_privilege_required) {
    return emit(command_result(false, install_code_name(code), "local_state"), kExitCredentialFailure);
  }
  if (code == InstallCode::internal_error) {
    return emit(command_result(false, "INTERNAL_ERROR", "internal"), kExitInternal);
  }
  return emit(command_result(false, install_code_name(code), "credential", false), kExitCredentialFailure);
}

RenderedCommand registration_failure(const RegistrationCode code) {
  if (code == RegistrationCode::local_state_busy) {
    return emit(command_result(false, "LOCAL_STATE_BUSY", "local_state", true), kExitRetryable);
  }
  if (code == RegistrationCode::internal_error) {
    return emit(command_result(false, "INTERNAL_ERROR", "internal"), kExitInternal);
  }
  return emit(command_result(false, registration_code_name(code), "local_state"), kExitCredentialFailure);
}

}  // namespace

RenderedCommand render_status(ILicenseStateStore& license_state) {
  RenderedCommand error{};
  const auto state = read_license_state(license_state, error);
  if (!state.has_value()) {
    return error;
  }
  if (!state->has_value() || !(*state)->credential) {
    return emit(command_result(false, "CREDENTIAL_NOT_FOUND", "credential", false,
                               Json{{"credential_state", "absent"}}),
                kExitCredentialFailure);
  }
  const auto runtime = RuntimeLicense::from_installed(*(*state)->credential);
  const auto status = runtime.status();
  return emit(command_result(
                  true,
                  "OK",
                  "none",
                  false,
                  Json{{"credential_state", "valid"},
                       {"authority_revision", status.authority_revision.value_or(0U)},
                       {"credential_generation", status.credential_generation.value_or(0U)}}),
              kExitOk);
}

RenderedCommand render_entitlement(
    ILicenseStateStore& license_state,
    const std::string_view entitlement_id,
    const std::int64_t now) {
  RenderedCommand error{};
  const auto state = read_license_state(license_state, error);
  if (!state.has_value()) {
    return error;
  }
  if (!state->has_value() || !(*state)->credential) {
    return emit(command_result(false, "CREDENTIAL_NOT_FOUND", "credential", false,
                               Json{{"credential_state", "absent"}}),
                kExitCredentialFailure);
  }
  const auto runtime = RuntimeLicense::from_installed(*(*state)->credential);
  const auto entitlement = runtime.entitlement(entitlement_id, now);
  return emit(command_result(
                  true,
                  "OK",
                  "none",
                  false,
                  Json{{"entitlement_id", entitlement_id}, {"granted", entitlement.granted}}),
              kExitOk);
}

RenderedCommand render_device_register(
    IMachineStateStore& machine_state,
    ILicenseStateStore& license_state) {
  const auto identity = machine_state.read();
  switch (identity.outcome) {
    case StateStoreOutcome::ok:
      break;
    case StateStoreOutcome::absent:
      return emit(command_result(false, "IDENTITY_NOT_FOUND", "identity"), kExitCredentialFailure);
    case StateStoreOutcome::corrupt:
      return emit(command_result(false, "LOCAL_STATE_CORRUPT", "local_state"), kExitCredentialFailure);
    case StateStoreOutcome::unsupported:
      return emit(command_result(false, "LOCAL_STATE_UNSUPPORTED", "local_state"), kExitCredentialFailure);
    case StateStoreOutcome::privilege_required:
      return emit(command_result(false, "LOCAL_WRITE_PRIVILEGE_REQUIRED", "local_state"), kExitCredentialFailure);
    case StateStoreOutcome::unexpected_failure:
      return emit(command_result(false, "INTERNAL_ERROR", "internal"), kExitInternal);
  }
  if (!identity.identity) {
    return emit(command_result(false, "INTERNAL_ERROR", "internal"), kExitInternal);
  }
  RenderedCommand error{};
  const auto state = read_license_state(license_state, error);
  if (!state.has_value()) {
    return error;
  }
  const std::optional<RegistrationAssociation> association =
      state->has_value() ? (*state)->registration : std::nullopt;
  const auto self_report = resolve_self_report(association, identity.identity->identity_value);
  Json data{{"registered", self_report.registered}, {"self_report_value", self_report.value}};
  if (association) {
    data["device_id"] = association->device_id.to_string();
  }
  return emit(command_result(true, "OK", "none", false, std::move(data)), kExitOk);
}

RenderedCommand render_device_register_adopt(
    ILicenseStateStore& license_state,
    IMutationLock& lock,
    const std::string_view device_reference_text) {
  const auto reference = DeviceReference::parse(device_reference_text);
  if (!reference) {
    return emit(command_result(false, "COMMAND_INVALID", "command"), kExitCommandInvalid);
  }
  const auto result = adopt_registration(license_state, lock, *reference);
  if (result.code != RegistrationCode::ok) {
    return registration_failure(result.code);
  }
  return emit(command_result(
                  true, "OK", "none", false, Json{{"device_id", reference->to_string()}}),
              kExitOk);
}

RenderedCommand render_device_register_reset(
    ILicenseStateStore& license_state,
    IMutationLock& lock) {
  const auto result = reset_registration(license_state, lock);
  if (result.code != RegistrationCode::ok) {
    return registration_failure(result.code);
  }
  return emit(command_result(true, "OK", "none", false, Json{{"reset", true}}), kExitOk);
}

RenderedCommand render_install(
    ILicenseStateStore& license_state,
    IMutationLock& lock,
    const std::vector<wire::TrustedPublicKey>& trusted_keys,
    const InstallRequest& request) {
  const auto result = install_credential(license_state, lock, trusted_keys, request);
  if (result.code != InstallCode::ok && result.code != InstallCode::ok_idempotent) {
    return install_failure(result.code);
  }
  // The echo comes from the InstallResult committed under the lock; re-reading
  // state here would race with a concurrent install and could misclassify an
  // already-committed install as a local-state failure.
  Json data{{"result", install_code_name(result.code)}};
  if (result.installed) {
    data["credential_id"] = result.installed->credential_id;
    data["authority_revision"] = result.installed->authority_revision;
    data["credential_generation"] = result.installed->credential_generation;
  }
  return emit(command_result(true, "OK", "none", false, std::move(data)), kExitOk);
}

RenderedCommand render_held_version(ILicenseStateStore& license_state) {
  RenderedCommand error{};
  const auto state = read_license_state(license_state, error);
  if (!state.has_value()) {
    return error;
  }
  const auto held =
      held_version(state->has_value() ? **state : LicenseState{});
  if (!held) {
    return emit(command_result(true, "OK", "none", false, Json{{"hasCredential", false}}), kExitOk);
  }
  return emit(command_result(
                  true,
                  "OK",
                  "none",
                  false,
                  Json{{"hasCredential", true},
                       {"version",
                        {{"authorityRevision", held->authority_revision},
                         {"credentialGeneration", held->credential_generation}}}}),
              kExitOk);
}

}  // namespace axlic::core
