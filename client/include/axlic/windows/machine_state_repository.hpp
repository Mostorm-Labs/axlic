#pragma once

#include "axlic/core/identity.hpp"
#include "axlic/windows/machine_state.hpp"

#include <filesystem>
#include <functional>

namespace axlic::windows {

enum class CommitPoint { before_temp_write, after_temp_write, after_temp_flush, before_replace, after_replace };
using CommitObserver = std::function<void(CommitPoint)>;

class IStateRootSecurity {
 public:
  virtual ~IStateRootSecurity() = default;
  virtual core::StateStoreOutcome prepare(const std::filesystem::path& root) = 0;
};

class WindowsStateRootSecurity final : public IStateRootSecurity {
 public:
  core::StateStoreOutcome prepare(const std::filesystem::path& root) override;
};

class WindowsMachineStateStore final : public core::IMachineStateStore {
 public:
  WindowsMachineStateStore(
      std::filesystem::path root,
      MachineStateCodec& codec,
      IStateRootSecurity& security,
      CommitObserver observer = {});

  core::StateRead read() override;
  core::StateStoreOutcome prepare_for_mutation() override;
  core::StateStoreOutcome write(const core::IdentityRecord& identity) override;

  [[nodiscard]] const std::filesystem::path& root() const noexcept;
  [[nodiscard]] std::filesystem::path state_path() const;

 private:
  [[nodiscard]] std::filesystem::path temporary_path() const;

  std::filesystem::path root_;
  MachineStateCodec& codec_;
  IStateRootSecurity& security_;
  CommitObserver observer_;
};

[[nodiscard]] std::filesystem::path program_data_state_root();

}  // namespace axlic::windows
