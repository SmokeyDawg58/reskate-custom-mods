# ReSkate custom Throwdowns

Custom challenges for ReSkate 2.0.7, including Deathrace and seven-letter R.E.S.K.A.T.E.

## Get the current playtest

**[Current release](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/tag/v2.0.7-custom-20261016-deathrace-editor) · [Full package](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.0.7-custom-20261016-deathrace-editor/ReSkate-2.0.7-Playtest-Full-20261016-Deathrace-Editor.zip) · [Required Assets 1.0.6](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.0.7-custom-20261016-deathrace-editor/SmokeyDawg58-ReSkate_Throwdown_Assets-1.0.6.zip)**

Every PC needs the matching DLL, launcher and complete asset mod. Use Full for a new installation; it includes everything below plus the backup installer and file checker.

| Download | Contents |
| --- | --- |
| **[Full playtest package](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.0.7-custom-20261016-deathrace-editor/ReSkate-2.0.7-Playtest-Full-20261016-Deathrace-Editor.zip)** | Matching runtime, launcher, complete assets and installation tools |
| [Required asset mod 1.0.6](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.0.7-custom-20261016-deathrace-editor/SmokeyDawg58-ReSkate_Throwdown_Assets-1.0.6.zip) | Logos, flags, map-resident gates, static Chevrons, compass image and HUD artwork |
| [DLL](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.0.7-custom-20261016-deathrace-editor/ReSkate.dll) and [launcher](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.0.7-custom-20261016-deathrace-editor/ReSkateLauncher.exe) | The same lean runtime pair included in Full |
| [Asset authoring source](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.0.7-custom-20261016-deathrace-editor/ReSkate-Throwdown-Asset-Source-1.0.6.zip) | Current Studio project/FBMOD, logo/audio inputs and author/export code |

Deathrace now has stationary stunt Chevrons with air/wall/floor placement, three-axis row snapping, X rotation rings, Undo/Redo and 125%/90-degree defaults. Checkpoint rewind, protected recovery speed, quiet cues, camera-relative compass navigation and edge-aligned HUD are included. [Detailed changes and validation notes](docs/RELEASE_NOTES_20261016.md).

## Install

1. Close ReSkate and its launcher yourself.
2. Extract Full and run its backup installer for your game folder, then run the file checker. Follow [PLAYTEST.md](PLAYTEST.md).
3. Keep the complete mod at `Mods/SmokeyDawg58-ReSkate_Throwdown_Assets`, with `layout.toc`, `Win32` and `initfs_win32` directly inside. The installer also handles ModData. Replace older matching packages rather than enabling duplicates.
4. Launch the game yourself. Friends should use this same runtime and Assets 1.0.6.

All **79 automated checks pass**. Owner-confirmed behavior and remaining multiplayer checks are separated in [DIAGNOSTICS.md](DIAGNOSTICS.md). This is a controlled playtest. One native join confirmation may still say Spot Battle; that investigation remains deferred.

## Repository guide

| Location | Purpose |
| --- | --- |
| [Detailed release notes](docs/RELEASE_NOTES_20261016.md) | Everything added since the last public build |
| [PLAYTEST.md](PLAYTEST.md) | Installation and friend-playtest checklist |
| [DIAGNOSTICS.md](DIAGNOSTICS.md) | Automated results, human confirmations and limits |
| [Build guide](docs/BUILD.md) | Reproduce the published runtime |
| [Asset-pack guide](docs/ASSET_PACK.md) | Find and rebuild the complete current assets |
| [Archived releases](docs/ARCHIVED_RELEASES.md) | Historical downloads, clearly separated from the current build |
| `src/`, `mods.patch`, `SNAPSHOT.json` | Complete changed-source overlay, independently applied to pinned upstream |
| `tools/playtest/` | Backup installer and file checker |

Maintained by **SmokeyDawg58**, with substantial assistance from **OpenAI Codex**. See [CONTRIBUTORS.md](CONTRIBUTORS.md). Based on [official ReSkate](https://github.com/Dingo-Shenanigans/ReSkate); source remains GPL-3.0 under [LICENSE](LICENSE). No game executable, Steam DLL or personal settings are distributed. GitHub hosting does not create a Thunderstore listing.
