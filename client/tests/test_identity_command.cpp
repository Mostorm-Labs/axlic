#include "axlic/core/identity_command.hpp"

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include <string>

namespace {

axlic::core::IdentityRecord record() {
  return {
      .scheme_id = "axl-win-cng-software-p256-v1",
      .identity_epoch = 1,
      .identity_value = std::string(128, 'a'),
      .provider_kind = axlic::core::ProviderKind::software,
      .provider_name = "Microsoft Software Key Storage Provider",
      .key_name = "Auditoryworks.AxLicense.Identity.v1.00112233445566778899aabbccddeeff",
  };
}

}  // namespace

TEST_CASE("identity command success exposes only the frozen public identity fields") {
  const auto rendered = axlic::core::render_identity_result({axlic::core::IdentityCode::ok, false, record()});
  const auto document = nlohmann::json::parse(rendered.json);

  REQUIRE(rendered.exit_code == 0);
  REQUIRE(document.at("code") == "OK");
  REQUIRE(document.at("data") == nlohmann::json{{"identity_state", "ready"},
                                                 {"scheme_id", "axl-win-cng-software-p256-v1"},
                                                 {"identity_epoch", 1},
                                                 {"identity_value", std::string(128, 'a')}});
  REQUIRE(rendered.json.find("Microsoft Software Key Storage Provider") == std::string::npos);
  REQUIRE(rendered.json.find("Auditoryworks.AxLicense.Identity.v1.") == std::string::npos);
}

TEST_CASE("identity command error families preserve code category and retryability") {
  using axlic::core::IdentityCode;
  const struct {
    IdentityCode code;
    const char* expected_code;
    const char* category;
    bool retryable;
    int exit_code;
  } cases[] = {
      {IdentityCode::local_write_privilege_required, "LOCAL_WRITE_PRIVILEGE_REQUIRED", "local_state", false, 3},
      {IdentityCode::local_state_busy, "LOCAL_STATE_BUSY", "local_state", true, 5},
      {IdentityCode::local_state_corrupt, "LOCAL_STATE_CORRUPT", "local_state", false, 3},
      {IdentityCode::local_state_unsupported, "LOCAL_STATE_UNSUPPORTED", "local_state", false, 3},
      {IdentityCode::identity_provider_unavailable, "IDENTITY_PROVIDER_UNAVAILABLE", "identity", true, 5},
      {IdentityCode::identity_provider_unsupported, "IDENTITY_PROVIDER_UNSUPPORTED", "identity", false, 3},
      {IdentityCode::identity_recovery_required, "IDENTITY_RECOVERY_REQUIRED", "identity", false, 3},
  };
  for (const auto& item : cases) {
    const auto rendered = axlic::core::render_identity_result({item.code, item.retryable, std::nullopt});
    const auto document = nlohmann::json::parse(rendered.json);
    REQUIRE(rendered.exit_code == item.exit_code);
    REQUIRE(document.at("code") == item.expected_code);
    REQUIRE(document.at("category") == item.category);
    REQUIRE(document.at("retryable") == item.retryable);
    REQUIRE_FALSE(document.contains("data"));
  }
}
