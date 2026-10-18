# Build the local custom 2.1.0 merge

This local main merge is unpublished. Use the matching local Full and Assets 1.0.7 packages; the older public playtest is not this runtime.

1. Check out official ReSkate v2.1.0 at `ba1245a5af51892e90f2be8637dd69a7e20f5b77`.
2. Apply `mods.patch` with `git apply --check` and then `git apply`. It includes binary assets and deletions; copying `src` alone is insufficient.
3. Rebuild `assets/one_up/native_assets.bin` from the supplied Assets 1.0.7 FBMOD/Studio project with the current ReSkateStudio CLI and `contrib/throwdown-flags/build_oneup_native_bundle.py`, then export the mod with `package_asset_mod.py`. `native_assets.sha256` pins the result supplied with this integration.
4. Install the upstream required Visual Studio 2022 C++ tools, Windows SDK, CMake 3.24+, Python 3.11+ and locked Rust/Cargo dependencies. Build with the lean-playtest-x64 preset, Release, and DINGOSDK_LEAN_LOGGING=OFF. Set DINGOSDK_PLAYTEST_PACKAGE_ROOT to the matching asset-kit/Full root for native-asset tests.
5. Build and run the configured ctest suite. All 91 checks passed on the prepared runtime. Production logging, tracing, profiling and crash uploads stay off. The opt-in logging source option is for diagnosis only; a logging-enabled build is not the shipped candidate.

SNAPSHOT.json records the exact branch input, canonical tree, runtime and asset hashes. BUILD_SOURCES.json pins the raw build inputs. Source reconstruction was independently reapplied to the pinned official base. See the detailed release notes for merge scope and live-validation limits.
