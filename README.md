# ReSkate 2.1.0 custom Throwdowns

**[Latest update](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/tag/v2.1.0-custom-20261018-reskate-deathrace) · [DLL](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261018-reskate-deathrace/ReSkate.dll) · [Launcher](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261018-reskate-deathrace/ReSkateLauncher.exe) · [Assets 1.0.7](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261018-reskate-deathrace/SmokeyDawg58-ReSkate_Throwdown_Assets-1.0.7.zip)**

R.E.S.K.A.T.E. from reskate-mode-210 is merged into main alongside the current Deathrace fixes and enroll-once/host-Start multiplayer flow. The build also includes the imported missing-GUID custom-map freeze protection and external-pack HUD residency fix. **All 91 optimized automated checks pass.** Live testing of this exact combined runtime remains pending; the download is a playtest prerelease.

Deathrace carries forward Save-only route publication, private drafts, gate hide/rewind, a finish compass icon, compact themed intro, Chevron tilt/roll, taller start flags, restored Undo/Redo, air gates, later-gate-only missed checkpoint recovery, hippy-jump handling and a separate route-info panel. The final R.E.S.K.A.T.E. feature merge preserves that Deathrace implementation. See [detailed changes and limits](docs/RELEASE_NOTES_20261018_RESKATE_DEATHRACE.md) and [preservation audit](docs/DEATHRACE_PRESERVATION_20261018.json).

## Install

Download both runtime files and the complete asset ZIP above. Close the game and launcher yourself, back up your current files, and copy the DLL and launcher beside Skate.exe. Extract the asset ZIP into a folder named SmokeyDawg58-ReSkate_Throwdown_Assets under your active Mods directory (or ModData/Default/Mods). Replace the matching old asset folder, keep layout.toc, initfs_win32, Win32 and Audio directly inside, and enable one copy. Every PC needs this same build and Assets 1.0.7; the previous 1.0.6 pack is insufficient. Launch yourself and rebuild generated mod files if prompted.

Release downloads now contain only the DLL, launcher and updated asset pack. Source remains in this repository, including exact mods.patch, src/, SNAPSHOT.json, BUILD_SOURCES.json and asset-authoring/export inputs. Full, runtime-update and source ZIPs are no longer attached to new releases. Earlier releases remain available as history.

See [checks to try later](PLAYTEST.md), [validation](DIAGNOSTICS.md), [build guide](docs/BUILD.md), [asset guide](docs/ASSET_PACK.md) and [changelog](CHANGELOG.md). Session protocol 52, mode wire 16 and 1-UP wire 5 are unchanged; all players still need matching files. No game executable, Steam DLL, settings, private captures or credentials are shipped. Publication does not install or control either application.

Maintained by SmokeyDawg58. R.E.S.K.A.T.E./map-fix contributions by hunterjedward123, with recorded Claude Opus 5.5 AI co-author credit retained. Integration and checks received substantial OpenAI Codex AI assistance. Upstream authorship and GPL-3.0 remain; see [CONTRIBUTORS.md](CONTRIBUTORS.md).
