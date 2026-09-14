# AxLicense

AxLicense A0 is a Windows C++20 command-line vertical slice that validates a deterministic-CBOR signed credential locally with Windows CNG and evaluates its entitlement snapshot without a server.

## Build and test

The frozen A0 toolchain is CMake 4.4.2, Visual Studio 2022 17.14 / MSVC 14.44, and Windows SDK 10.0.26100.0.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -T v143
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The public A0 commands are:

```text
axlic status
axlic entitlement <entitlement_id>
```

The checked-in `reference/vectors/a0` corpus and the `AXLIC_A0_FIXTURE_STATE` environment variable are explicitly non-production A0 test adapters. They are not a protected-state or trust-store contract.
