# Build the custom 2.1.0 update

Current player downloads: [DLL](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261018-reskate-deathrace/ReSkate.dll), [launcher](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261018-reskate-deathrace/ReSkateLauncher.exe) and [Assets 1.0.7](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261018-reskate-deathrace/SmokeyDawg58-ReSkate_Throwdown_Assets-1.0.7.zip). Source and the exact reconstruction patch are versioned here; no separate source ZIP is attached.

1. Check out official ReSkate v2.1.0 at `ba1245a5af51892e90f2be8637dd69a7e20f5b77`.
2. Apply `mods.patch` with `git apply --check` and then `git apply`. It includes binary assets and deletions; copying `src` alone is insufficient.
3. Use the complete native player pack linked above. The lean build reads the external pack, so the optional embedded `assets/one_up/native_assets.bin` is not required for that configuration. Asset-authoring/export inputs are under `src/contrib/throwdown-flags` and `src/assets`; `native_assets.sha256` pins the internal build archive. Native pack tests use the complete extracted pack via DINGOSDK_PLAYTEST_PACKAGE_ROOT.
4. Install the upstream required Visual Studio 2022 C++ tools, Windows SDK, CMake 3.24+, Python 3.11+ and locked Rust/Cargo dependencies. Build with the lean-playtest-x64 preset, Release, and DINGOSDK_LEAN_LOGGING=OFF. Set DINGOSDK_PLAYTEST_PACKAGE_ROOT to the matching asset-kit/Full root for native-asset tests.
5. Build and run the configured ctest suite. All 91 checks passed on the prepared runtime. Production logging, tracing, profiling and crash uploads stay off. The opt-in logging source option is for diagnosis only; a logging-enabled build is not the shipped candidate.

SNAPSHOT.json records the exact branch input, canonical tree, runtime and asset hashes. BUILD_SOURCES.json pins the raw build inputs. Source reconstruction was independently reapplied to the pinned official base. See the detailed release notes for merge scope and live-validation limits.
