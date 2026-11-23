#pragma once

#include "axlic/core/device_reference.hpp"
#include "axlic/core/identity.hpp"
#include "axlic/wire/credential.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace axlic::core {

// Local association with the platform-assigned device reference. Convenience
// reference only, never canonical authority.
struct RegistrationAssociation {
  DeviceReference device_id;

  bool operator==(const RegistrationAssociation&) const = default;
};

// Locally installed credential as committed by the installation pipeline.
// The artifact was verified by the frozen A0 verifier at install time; the
// stored copy is therefore the trusted offline truth. An empty entitlements
// vector is a legal revocation credential. policy_version is channel-side
// metadata carried by the delivery/reconcile messages and never part of the
// CBOR payload.
struct InstalledCredential {
  std::string credential_id;
  std::string license_grant_id;
  std::string binding_id;
  std::uint64_t authority_revision{};
  std::uint64_t credential_generation{};
  std::int64_t issued_at{};
  std::vector<wire::Entitlement> entitlements;
  std::uint64_t policy_version{};

  bool operator==(const InstalledCredential&) const = default;
};

// Held license state: registration association plus installed credential
// (whose (authority_revision, credential_generation) is the anti-rollback
// high-watermark). Either half may be absent independently.
struct LicenseState {
  std::optional<RegistrationAssociation> registration;
  std::optional<InstalledCredential> credential;

  bool operator==(const LicenseState&) const = default;
};

struct LicenseStateRead {
  StateStoreOutcome outcome{StateStoreOutcome::unexpected_failure};
  std::optional<LicenseState> state;
};

// Mirrors IMachineStateStore semantics for the parallel license-state file.
class ILicenseStateStore {
 public:
  virtual ~ILicenseStateStore() = default;
  virtual LicenseStateRead read() = 0;
  virtual StateStoreOutcome prepare_for_mutation() = 0;
  virtual StateStoreOutcome write(const LicenseState& state) = 0;
};

}  // namespace axlic::core
