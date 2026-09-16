# AxLicense

AxLicense is a Windows C++20 command-line client. The A1 slice adds one machine-wide Windows DeviceIdentity backed by a non-exportable persisted CNG P-256 key, TPM-first provider selection, DPAPI-protected ProgramData state, a protected cross-process mutation lock, and crash-safe atomic state replacement. It does not require a server.

## Build and test

The frozen A0 toolchain is CMake 4.4.2, Visual Studio 2022 17.14 / MSVC 14.44, and Windows SDK 10.0.26100.0.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -T v143
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The public commands currently implemented are:

```text
axlic status
axlic entitlement <entitlement_id>
axlic identity
```

`axlic identity` must be run with machine-state write authority for first establishment. It returns only the public scheme, epoch, and 64-byte P-256 public identity as lowercase hex; provider key names and private material are never part of the CLI contract.

The checked-in `reference/vectors/a0` corpus and `AXLIC_A0_FIXTURE_STATE` remain explicit test-only A0 regression adapters. Production `axlic.exe` does not read that environment variable.
