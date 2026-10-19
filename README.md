# ReSkate 2.1.1 custom Throwdowns

**[Latest custom update](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/tag/v2.1.1-custom-20261019-deathrace-routes) | [DLL](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.1-custom-20261019-deathrace-routes/ReSkate.dll) | [Launcher](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.1-custom-20261019-deathrace-routes/ReSkateLauncher.exe) | [Assets 1.0.15](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.1-custom-20261019-deathrace-routes/SmokeyDawg58-ReSkate_Throwdown_Assets-1.0.15.zip)**

Official ReSkate 2.1.1 with the existing R.E.S.K.A.T.E. integration, Deathrace builder and enroll-once/host-Start multiplayer flow. This update restores the original Chevron, retains its complete texture chain on custom maps, and fixes route-menu confirmation leaking into gate placement. It also publishes the accumulated named JSON route library, sharing/folder actions, recovery, editor and presentation changes.

**All 92 automated checks pass.** Both installed Improved-map inputs were independently rebuilt and inspected offline. Live gameplay on this exact build remains pending, so this is a playtest prerelease. Read the [detailed release notes](docs/RELEASE_NOTES_20261019_211_DEATHRACE_ROUTES.md) for the full change list, evidence and limits.

## Install

Download all three files above. Close the game and launcher yourself, back up the current files, and put the DLL and launcher beside Skate.exe. Replace the complete Throwdown asset folder under your active Mods directory; keep layout.toc, initfs_win32, Win32 and Audio directly inside SmokeyDawg58-ReSkate_Throwdown_Assets, with one enabled copy. Version-suffixed folders are supported. ModData setups use their active Mods directory. Every PC needs this matching runtime pair and Assets 1.0.15. Launch normally and allow generated files to rebuild if prompted.

## Routes and controls

Tap Start to name/save; hold Start 1.5 seconds to load. The Load menu shows the current map's routes. Use D-pad Right for pencil rename and then folder reveal. Names use a keyboard. Saves are readable, shareable JSON under Mods/ReSkateRoutes/Deathrace/<map>/<route name>.json. The Load .json button imports a validated route. Opening the library migrates compatible older saves while preserving the originals. New worlds start without silently loading old gates.

The compact complete asset ZIP is about 119 MB. Duplicate native storage was removed losslessly; required artwork, resolution, audio and registrations are preserved. Releases contain only the DLL, launcher and complete matching asset pack. Source, exact mods.patch, src/, BUILD_SOURCES.json and SNAPSHOT.json remain here. Historical releases have not been deleted or edited.

See [what to test](PLAYTEST.md), [validation](DIAGNOSTICS.md), [build instructions](docs/BUILD.md), [asset details](docs/ASSET_PACK.md) and [changelog](CHANGELOG.md). Protocol 52, mode wire 16 and 1-UP wire 5 are preserved. No game executable, Steam DLL, personal routes, settings or crash dumps are shipped.

Maintained by SmokeyDawg58. Upstream authorship, GPL-3.0, recorded R.E.S.K.A.T.E. contribution credits and substantial OpenAI Codex AI assistance are retained; see [credits](CONTRIBUTORS.md).
