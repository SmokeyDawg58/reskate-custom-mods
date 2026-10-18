# ReSkate custom Throwdowns

Custom Deathrace, 1-UP and seven-letter R.E.S.K.A.T.E. for **ReSkate 2.1.0**.

## Download the current build

**[Latest release](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/tag/v2.1.0-custom-20261017) · [Full package](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261017/ReSkate-2.1.0-Custom-Full.zip) · [Required asset mod](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261017/SmokeyDawg58-ReSkate_Throwdown_Assets-1.0.6-For-2.1.0.zip)**

| Download | Use it for |
| --- | --- |
| **[Full package — recommended](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261017/ReSkate-2.1.0-Custom-Full.zip)** | New installations: matching runtime, launcher, all assets, backup installer and file checker |
| [Runtime Update](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261017/ReSkate-2.1.0-Custom-Runtime-Update.zip) | Updating a setup that already has complete Assets 1.0.6 |
| [Required Assets 1.0.6](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261017/SmokeyDawg58-ReSkate_Throwdown_Assets-1.0.6-For-2.1.0.zip) | Complete logos, flags, gates, Chevrons, compass and HUD mod folder; included in Full |
| [DLL](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261017/ReSkate.dll) and [launcher](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261017/ReSkateLauncher.exe) | Individual matching runtime files |
| [Asset authoring source](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261017/ReSkate-Throwdown-Asset-Source-1.0.6.zip) | Complete Studio project/FBMOD, artwork, selected sound inputs and author/export code |

**Map-change fix confirmed:** the owner repeated Skate3Improved ↔ Skate2Improved transitions on this exact build and reported “Every load works.” All **90 automated Release checks pass**. Wider multiplayer checks remain pending; see [DIAGNOSTICS.md](DIAGNOSTICS.md).

This release merges the working custom challenges with official 2.1.0 and its shader/renderer crash fixes. Deathrace retains stationary stunt Chevrons, air/wall/floor placement, three-axis snapping, X rotation rings, Undo/Redo, checkpoint rewind, recovery speed protection, quiet cues, the gate compass and styled edge HUD. [Detailed release notes](docs/RELEASE_NOTES_20261017.md).

## Install

1. Close ReSkate and its launcher yourself.
2. Extract Full. Run its backup installer for your game folder, then the file checker. Follow [PLAYTEST.md](PLAYTEST.md).
3. Keep `layout.toc`, `Win32` and `initfs_win32` directly inside the complete Throwdown Assets folder under `Mods`. The installer handles the active Mods/ModData location. Enable one copy only.
4. Every player needs this matching custom 2.1.0 runtime and complete Assets 1.0.6. Older 2.0.7 runtime clients cannot join this session protocol.

The DLL is **30.3 MB**; the native assets stay external. Logging, profiling, tracing, crash uploads and shipped debug symbols are disabled. Existing complete Assets 1.0.6 need no native/audio update. This distribution refreshes their 2.1.0 instructions.

## Repository guide

| Location | Purpose |
| --- | --- |
| [Release notes](docs/RELEASE_NOTES_20261017.md) | Changes, compatibility and validation scope |
| [PLAYTEST.md](PLAYTEST.md) | Install steps and remaining gameplay checks |
| [DIAGNOSTICS.md](DIAGNOSTICS.md) | Automated results and scoped human confirmation |
| [Build guide](docs/BUILD.md) | Apply the verified overlay to official 2.1.0 and build |
| [Asset-pack guide](docs/ASSET_PACK.md) | Find and rebuild the complete assets |
| [Archived releases](docs/ARCHIVED_RELEASES.md) | Historical versions; use the current download for playing |
| `src/`, `mods.patch`, `SNAPSHOT.json`, `BUILD_SOURCES.json` | Changed-source overlay and exact provenance |
| `tools/playtest/` | Backup installer and file checker |

Maintained by **SmokeyDawg58**, with substantial **OpenAI Codex AI assistance**. See [CONTRIBUTORS.md](CONTRIBUTORS.md). Based on [official ReSkate](https://github.com/Dingo-Shenanigans/ReSkate); GPL-3.0 attribution and [LICENSE](LICENSE) are retained. No game executable, personal settings, private captures or unrelated mods are distributed. No Thunderstore listing is created by this GitHub publication.
