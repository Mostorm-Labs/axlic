#pragma once

#include "axlic/core/license_state.hpp"
#include "axlic/windows/machine_state.hpp"
#include "axlic/windows/machine_state_repository.hpp"

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace axlic::windows {

// DPAPI-protected codec for the license-state plaintext, mirroring
// MachineStateCodec but keyed to its own entropy string and format tag.
class LicenseStateCodec {
 public:
  static constexpr std::size_t maximum_carrier_size = 4U * 1024U * 1024U;

  explicit LicenseStateCodec(IDataProtector& protector);
  [[nodiscard]] core::StateStoreOutcome encode(const core::LicenseState& state, std::string& carrier) const;
  [[nodiscard]] core::LicenseStateRead decode(std::string_view carrier) const;

 private:
  IDataProtector& protector_;
};

class WindowsLicenseStateStore final : public core::ILicenseStateStore {
 public:
  WindowsLicenseStateStore(
      std::filesystem::path root,
      LicenseStateCodec& codec,
      IStateRootSecurity& security,
      CommitObserver observer = {});

  core::LicenseStateRead read() override;
  core::StateStoreOutcome prepare_for_mutation() override;
  core::StateStoreOutcome write(const core::LicenseState& state) override;

  [[nodiscard]] const std::filesystem::path& root() const noexcept;
  [[nodiscard]] std::filesystem::path state_path() const;

 private:
  [[nodiscard]] std::filesystem::path temporary_path() const;

  std::filesystem::path root_;
  LicenseStateCodec& codec_;
  IStateRootSecurity& security_;
  CommitObserver observer_;
};

}  // namespace axlic::windows
