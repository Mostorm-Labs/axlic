#pragma once

#include "axlic/core/identity.hpp"

#include <cstdint>
#include <string>

namespace axlic::windows {

class WindowsMutationLock final : public core::IMutationLock {
 public:
  WindowsMutationLock();
  WindowsMutationLock(std::wstring name, std::wstring security_sddl, std::uint32_t timeout_ms);
  ~WindowsMutationLock() override;

  WindowsMutationLock(const WindowsMutationLock&) = delete;
  WindowsMutationLock& operator=(const WindowsMutationLock&) = delete;

  core::LockOutcome acquire() override;
  void release() noexcept override;

 private:
  std::wstring name_;
  std::wstring security_sddl_;
  std::uint32_t timeout_ms_{};
  void* handle_{};
  bool owned_{};
};

}  // namespace axlic::windows
