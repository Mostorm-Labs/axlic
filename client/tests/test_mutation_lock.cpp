#include "axlic/windows/mutation_lock.hpp"

#include <catch2/catch_test_macros.hpp>

#define NOMINMAX
#include <windows.h>

#include <string>
#include <thread>

using axlic::core::LockOutcome;
using axlic::windows::WindowsMutationLock;

TEST_CASE("Windows mutation lock serializes independent process handles") {
  const auto name = std::wstring(L"Local\\AxLicense.A1.Test.") + std::to_wstring(GetCurrentProcessId());
  constexpr auto test_sddl = L"D:P(A;;GA;;;SY)(A;;GA;;;BA)(A;;GA;;;OW)";
  WindowsMutationLock first(name, test_sddl, 0);
  WindowsMutationLock second(name, test_sddl, 0);

  REQUIRE(first.acquire() == LockOutcome::acquired);
  auto second_result = LockOutcome::unexpected_failure;
  std::jthread contender([&] { second_result = second.acquire(); });
  contender.join();
  REQUIRE(second_result == LockOutcome::busy);
  first.release();
  std::jthread successor([&] {
    second_result = second.acquire();
    second.release();
  });
  successor.join();
  REQUIRE(second_result == LockOutcome::acquired);
}
