#pragma once

#include "axlic/core/identity.hpp"

namespace axlic::core {

// Shared RAII guard for IMutationLock. Callers construct it once, then branch
// on acquired()/retryable/denied() instead of re-implementing outcome triage;
// release only happens for a lock this guard actually acquired.
class MutationLockGuard {
 public:
  MutationLockGuard(IMutationLock& lock, bool& retryable) : lock_(lock), retryable_(retryable) {
    const auto outcome = lock.acquire();
    acquired_ = outcome == LockOutcome::acquired;
    retryable = outcome == LockOutcome::busy;
    denied_ = outcome == LockOutcome::access_denied;
  }
  ~MutationLockGuard() {
    if (acquired_) {
      lock_.release();
    }
  }
  MutationLockGuard(const MutationLockGuard&) = delete;
  MutationLockGuard& operator=(const MutationLockGuard&) = delete;
  [[nodiscard]] bool acquired() const noexcept { return acquired_; }
  [[nodiscard]] bool denied() const noexcept { return denied_; }

 private:
  IMutationLock& lock_;
  bool& retryable_;
  bool acquired_{};
  bool denied_{};
};

}  // namespace axlic::core
