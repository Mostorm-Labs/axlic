#include "axlic/core/identity_command.hpp"

#include <nlohmann/json.hpp>

#include <string_view>

namespace axlic::core {
namespace {

std::string_view category(const IdentityCode code) {
  switch (code) {
    case IdentityCode::local_write_privilege_required:
    case IdentityCode::local_state_busy:
    case IdentityCode::local_state_corrupt:
    case IdentityCode::local_state_unsupported:
      return "local_state";
    case IdentityCode::identity_provider_unavailable:
    case IdentityCode::identity_provider_unsupported:
    case IdentityCode::identity_recovery_required:
      return "identity";
    case IdentityCode::ok:
      return "none";
    case IdentityCode::internal_error:
      return "internal";
  }
  return "internal";
}

}  // namespace

RenderedCommand render_identity_result(const IdentityResult& result) {
  auto code = result.code;
  if (code == IdentityCode::ok && !result.identity) {
    code = IdentityCode::internal_error;
  }
  nlohmann::json document = {
      {"contract_version", "1.0"},
      {"ok", code == IdentityCode::ok},
      {"code", identity_code_name(code)},
      {"category", category(code)},
      {"retryable", code == result.code ? result.retryable : false},
      {"correlation_ref", "a1-local"},
  };
  if (code == IdentityCode::ok) {
    const auto& identity = *result.identity;
    document["data"] = {
        {"identity_state", "ready"},
        {"scheme_id", identity.scheme_id},
        {"identity_epoch", identity.identity_epoch},
        {"identity_value", identity.identity_value},
    };
  }
  const auto exit_code = code == IdentityCode::ok ? 0 : (result.retryable ? 5 : (code == IdentityCode::internal_error ? 4 : 3));
  return {document.dump(), exit_code};
}

}  // namespace axlic::core
