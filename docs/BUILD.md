# Build from source

Players do not need to build anything. Use the [current release](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/tag/v2.0.7-custom-20261015-reskate-refresh) instead.

This repository contains the changed files, rather than a full copy of upstream ReSkate. To recreate the published code:

1. Check out official ReSkate commit `0f8af627105c1079ef25f36ad83b159ae9d1df1f`.
2. Run `git apply --check mods.patch`, then `git apply mods.patch` using this repository's patch.
3. Download `native_assets.bin` and `native_assets.sha256` from the same release. Verify the checksum and put the archive in `assets/one_up/native_assets.bin`.
4. With Python 3.11+, CMake 3.24+ and Visual Studio 2022 C++ tools installed, run the asset export command below from the patched source root.
5. Configure and build the lean playtest preset, then run all Release tests.

```powershell
python contrib/throwdown-flags/package_asset_mod.py --output build/lean-playtest-x64/playtest-package/Mods/SmokeyDawg58-ReSkate_Throwdown_Assets
cmake --preset lean-playtest-x64
cmake --build --preset lean-playtest-release --parallel 4
ctest --test-dir build/lean-playtest-x64 -C Release --output-on-failure
```

Distribute the matching Release DLL and launcher together with the exported asset mod. The lean DLL does not embed that mod. The development preset supports optional diagnostics and embedded assets; use the lean preset when reproducing the public playtest.

`SNAPSHOT.json` records source file hashes and the pinned upstream version. The release's `release-manifest.json` and `SHA256SUMS.txt` identify its exact downloads. Both PCs must use matching versions for multiplayer.


The patch includes the exact original pink PNG and quiet WAV inputs. Asset pack 1.0.2 is mandatory for this build. See [ASSET_PACK.md](ASSET_PACK.md) for the authoring project and source tools.
