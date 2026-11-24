#pragma once

#include <nlohmann/json.hpp>

#include <optional>
#include <string_view>
#include <utility>

namespace axlic::core {

// Frozen exit-code family shared by every CLI surface: 0 success /
// 2 command-invalid / 3 credential-class or local-state failure / 4 internal /
// 5 retryable.
constexpr int kExitOk = 0;
constexpr int kExitCommandInvalid = 2;
constexpr int kExitCredentialFailure = 3;
constexpr int kExitInternal = 4;
constexpr int kExitRetryable = 5;

// Single source of the command-result JSON envelope (contract_version 1.0).
// The correlation_ref parameter keeps the a1-local/a2-local/a2-scenario
// identities distinct while the field list lives in exactly one place.
inline nlohmann::json command_result(
    const std::string_view correlation_ref,
    const bool ok,
    const std::string_view code,
    const std::string_view category,
    const bool retryable = false,
    std::optional<nlohmann::json> data = std::nullopt) {
  nlohmann::json result = {{"contract_version", "1.0"},
                           {"ok", ok},
                           {"code", code},
                           {"category", category},
                           {"retryable", retryable},
                           {"correlation_ref", correlation_ref}};
  if (data) {
    result["data"] = std::move(*data);
  }
  return result;
}

}  // namespace axlic::core
