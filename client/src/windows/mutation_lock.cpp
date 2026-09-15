#include "axlic/windows/mutation_lock.hpp"

#define NOMINMAX
#include <windows.h>
#include <sddl.h>

#include <utility>

namespace axlic::windows {
namespace {

constexpr wchar_t kMutationLockName[] = L"Global\\Auditoryworks.AxLicense.MachineMutation.v1";
constexpr wchar_t kMutationLockSddl[] = L"D:P(A;;GA;;;SY)(A;;GA;;;BA)";

}  // namespace

WindowsMutationLock::WindowsMutationLock() : WindowsMutationLock(kMutationLockName, kMutationLockSddl, 30'000U) {}

WindowsMutationLock::WindowsMutationLock(
    std::wstring name,
    std::wstring security_sddl,
    const std::uint32_t timeout_ms)
    : name_(std::move(name)), security_sddl_(std::move(security_sddl)), timeout_ms_(timeout_ms) {}

WindowsMutationLock::~WindowsMutationLock() {
  release();
}

core::LockOutcome WindowsMutationLock::acquire() {
  if (handle_ != nullptr) {
    return core::LockOutcome::unexpected_failure;
  }
  PSECURITY_DESCRIPTOR descriptor{};
  if (ConvertStringSecurityDescriptorToSecurityDescriptorW(
          security_sddl_.c_str(), SDDL_REVISION_1, &descriptor, nullptr) == FALSE) {
    return core::LockOutcome::unexpected_failure;
  }
  SECURITY_ATTRIBUTES attributes{
      .nLength = sizeof(SECURITY_ATTRIBUTES),
      .lpSecurityDescriptor = descriptor,
      .bInheritHandle = FALSE,
  };
  const auto handle = CreateMutexExW(&attributes, name_.c_str(), 0, MUTEX_ALL_ACCESS);
  LocalFree(descriptor);
  if (handle == nullptr) {
    return GetLastError() == ERROR_ACCESS_DENIED ? core::LockOutcome::access_denied
                                                 : core::LockOutcome::unexpected_failure;
  }
  handle_ = handle;
  const auto wait = WaitForSingleObject(handle, timeout_ms_);
  if (wait == WAIT_OBJECT_0 || wait == WAIT_ABANDONED) {
    owned_ = true;
    return core::LockOutcome::acquired;
  }
  CloseHandle(handle);
  handle_ = nullptr;
  if (wait == WAIT_TIMEOUT) {
    return core::LockOutcome::busy;
  }
  return core::LockOutcome::unexpected_failure;
}

void WindowsMutationLock::release() noexcept {
  const auto handle = static_cast<HANDLE>(handle_);
  if (handle == nullptr) {
    return;
  }
  if (owned_) {
    ReleaseMutex(handle);
  }
  CloseHandle(handle);
  handle_ = nullptr;
  owned_ = false;
}

}  // namespace axlic::windows
