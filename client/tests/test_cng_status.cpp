#include "axlic/windows/cng_identity_provider.hpp"

#include <catch2/catch_test_macros.hpp>

#define NOMINMAX
#include <windows.h>
#include <ncrypt.h>

using axlic::core::ProviderOutcome;
using axlic::windows::CngOperation;
using axlic::windows::map_cng_status;

TEST_CASE("CNG status normalization distinguishes downgrade-safe provider absence") {
  REQUIRE(map_cng_status(ERROR_SUCCESS, CngOperation::probe) == ProviderOutcome::usable);
  REQUIRE(map_cng_status(NTE_PROV_TYPE_NOT_DEF, CngOperation::probe) ==
          ProviderOutcome::absent_or_permanently_unusable);
  REQUIRE(map_cng_status(NTE_BAD_PROVIDER, CngOperation::probe) ==
          ProviderOutcome::absent_or_permanently_unusable);
  REQUIRE(map_cng_status(NTE_NOT_SUPPORTED, CngOperation::probe) ==
          ProviderOutcome::absent_or_permanently_unusable);
  REQUIRE(map_cng_status(NTE_NOT_SUPPORTED, CngOperation::establish) ==
          ProviderOutcome::absent_or_permanently_unusable);
  REQUIRE(map_cng_status(NTE_DEVICE_NOT_FOUND, CngOperation::establish) ==
          ProviderOutcome::absent_or_permanently_unusable);
}

TEST_CASE("CNG transient access and corrupt-key statuses remain distinct") {
  REQUIRE(map_cng_status(NTE_DEVICE_NOT_READY, CngOperation::probe) == ProviderOutcome::temporarily_unavailable);
  REQUIRE(map_cng_status(NTE_BAD_KEYSET, CngOperation::load) == ProviderOutcome::key_not_found_or_corrupt);
  REQUIRE(map_cng_status(NTE_BAD_KEY_STATE, CngOperation::load) == ProviderOutcome::key_not_found_or_corrupt);
  REQUIRE(map_cng_status(NTE_PERM, CngOperation::establish) == ProviderOutcome::access_denied);
  REQUIRE(map_cng_status(static_cast<SECURITY_STATUS>(0x81234567L), CngOperation::load) ==
          ProviderOutcome::unexpected_failure);
}
