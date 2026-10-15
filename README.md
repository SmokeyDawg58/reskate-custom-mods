# ReSkate custom Throwdowns — lean playtest

Download the **complete client ZIP** from the [lean playtest release](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/tag/v2.0.7-custom-20261015-lean-playtest). It contains the matching ReSkate.dll, launcher, installer, preflight checker and the required **SmokeyDawg58-ReSkate_Throwdown_Assets** mod folder.

The DLL is about 14 MB instead of 420 MB. The native artwork and game-data pack now lives in the visible Mods folder. Every player needs it for custom logos, flags, gates, menus and HUD artwork. The full ZIP still includes the complete native data; its overall size reflects those assets. A separate Thunderstore-format asset ZIP is available for mod-manager installation. It has not been published to a Thunderstore account.

File logging, debugger/console log sinks, intensive native traces, model dumps, profiling, memory tracing, crash reporting, pose recording and trainer telemetry are disabled. No PDB or CodeView/COFF symbols are shipped. Native gameplay, protective cleanup hooks, HUD behavior, fonts and artwork are retained. The installer preserves unrelated mods/settings and backs up replaced files; it never starts or closes programs.

**79 automated checks passed.** Asset integrity, duplicate mount prevention, disabled diagnostics and installation rollback were checked. Fresh server simulations retained 4 and 16 clients with zero failed sends, including a simulated stall. **Live Steam multiplayer and first-launch rendering of this lean packaging variant still need verification.** See [DIAGNOSTICS.md](DIAGNOSTICS.md) and [PLAYTEST.md](PLAYTEST.md).

## Build

1. Start from official [ReSkate](https://github.com/Dingo-Shenanigans/ReSkate) commit `0f8af627105c1079ef25f36ad83b159ae9d1df1f`. Apply `mods.patch` after `git apply --check` succeeds.
2. Download native_assets.bin from this same release into assets/one_up/native_assets.bin. Verify its SHA-256 against SNAPSHOT.json. It is a source/build archive; the lean game loads the extracted mod folder.
3. Run `python contrib/throwdown-flags/package_asset_mod.py --output build/lean-playtest-x64/playtest-package/Mods/SmokeyDawg58-ReSkate_Throwdown_Assets`.
4. With Visual Studio 2022 C++ tools, CMake 3.24+ and Python 3.11+, configure `cmake --preset lean-playtest-x64`, then build `cmake --build --preset lean-playtest-release --parallel 4`.
5. Run `ctest --test-dir build/lean-playtest-x64 -C Release --output-on-failure`. Distribute the Release DLL/launcher plus the exported required mod folder together.

The normal vs2022-x64 preset remains available for development with embedded assets and diagnostics. The lean preset is the published playtest configuration. src/ is the modified overlay; mods.patch reconstructs it against the pinned upstream commit. SNAPSHOT.json pins source and assets. Large binaries and the source asset archive are release downloads.

The assets use the mod layout documented by the user-provided Studio source: layout.toc/Win32 directly inside a named mod folder. The standalone ZIP includes root manifest.json, README.md and a 256×256 PNG icon as specified by [Thunderstore's package guide](https://wiki.thunderstore.io/mods/creating-a-package). No Studio process is launched and no native asset was reauthored.

ReSkate and its modified source use GPL-3.0; see LICENSE. No game executable or unrelated third-party maps/cosmetics/settings are distributed. Publish only when explicitly requested, with matching source, runtime, launcher, native archive and checksums; never publish credentials or personal logs.
