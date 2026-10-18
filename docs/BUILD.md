# Build the current custom 2.1.0 runtime

Players can use the [current release](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/tag/v2.1.0-custom-20261017). This repository supplies the changed-source overlay, not a complete upstream checkout.

1. Check out official ReSkate tag v2.1.0 at `ba1245a5af51892e90f2be8637dd69a7e20f5b77`.
2. From that source tree, run `git apply --check PATH_TO_THIS_REPO/mods.patch`, then `git apply PATH_TO_THIS_REPO/mods.patch`. This binary-capable patch also applies deletions; copying src alone is insufficient.
3. Download native_assets.bin and native_assets.sha256 from this same release. Verify the hash and put the archive in `assets/one_up/native_assets.bin`.
4. Install Visual Studio 2022 C++ tools, the Windows SDK, CMake 3.24+, Python 3.11+, and Rust/Cargo with the x86_64-pc-windows-msvc toolchain. Use the upstream locked dependencies; Cargo output is pinned to the workspace by CMake.
5. From the patched root run:

```powershell
python contrib/throwdown-flags/package_asset_mod.py --output build/lean-playtest-x64/playtest-package/Mods/SmokeyDawg58-ReSkate_Throwdown_Assets
cmake --preset lean-playtest-x64
cmake --build --preset lean-playtest-release --parallel 4
ctest --test-dir build/lean-playtest-x64 -C Release --output-on-failure
```

The published build passed all 90 configured Release checks. Runtime music services are retained; standalone interactive audio probes are excluded from the lean default target. Logging, tracing, profiling, crash uploads and shipped symbols are disabled. Preserve the new upstream shader/renderer guards.

SNAPSHOT.json pins the upstream, integration tree, changed files and runtime hashes. BUILD_SOURCES.json records the verified build inputs. The patch was reapplied independently to the pinned upstream and exactly matched the integration tree. Native artwork remains external; distribute the matching asset mod with the runtime. See [ASSET_PACK.md](ASSET_PACK.md).
