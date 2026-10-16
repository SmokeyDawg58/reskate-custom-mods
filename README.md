# ReSkate custom Throwdowns

Custom challenges for ReSkate 2.0.7, including Deathrace routes, gates, race clocks, leaderboards and native challenge menus.

## Get the current build

**Use the [current playtest release](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/tag/v2.0.7-custom-20261015-multiplayer-flags).** Every player needs the same DLL and launcher, plus the required asset mod. Older releases are marked **ARCHIVED** and are kept for reference.

| What you need | Download |
| --- | --- |
| **New installation — start here** | [Full playtest package](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.0.7-custom-20261015-multiplayer-flags/ReSkate-2.0.7-Multiplayer-Playtest-Full-20261015.zip) — DLL, launcher, required assets and installer |
| Update an existing installation with the asset pack | [Small update](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.0.7-custom-20261015-multiplayer-flags/ReSkate-2.0.7-Multiplayer-Runtime-Update-20261015.zip) — replace both included files |
| Logos, flags, gates and HUD artwork | [Required asset mod](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.0.7-custom-20261015-multiplayer-flags/SmokeyDawg58-ReSkate_Throwdown_Assets-1.0.0.zip) — also included in the full package |

The full package is the easiest choice. The separate DLL and launcher on the release page are the same files included in it. `native_assets.bin` is for building from source; players should use the asset mod above.

## Install and play

1. Start with a supported ReSkate 2.0.7 installation.
2. Close the game and launcher yourself.
3. Extract the full package, then follow [the installation guide](PLAYTEST.md). Its installer backs up replaced files and preserves other mods and settings.
4. Run the included preflight checker on every PC, then launch the game yourself.

Keep the complete asset mod installed so everyone sees the logos, flags, gates and menus. Its files belong directly inside `Mods/SmokeyDawg58-ReSkate_Throwdown_Assets`. Installations using ModData put it in `ModData/Default/Mods` instead. Numeric version suffixes on the mod folder are supported.

## What has been checked

All 79 automated checks passed for this build. In a two-PC playtest, the owner confirmed that guests can see and join through the Deathrace flag, controls return after finishing or DNF, and a guest can quit without ending the host's race. The working steady HUD is retained. The DLL is about 14 MB; logging, profiling, crash capture and shipped debug symbols are disabled.

**Still being improved:** some guest join screens say Spot Battle, and the guest waiting button can say Start. Only the host starts the custom race. Correct challenge wording and a guest Ready count are being developed separately and are not included in this download yet.

This is a public playtest. See [test results and remaining checks](DIAGNOSTICS.md) for its limits.

## Find your way around

| File or folder | Purpose |
| --- | --- |
| [PLAYTEST.md](PLAYTEST.md) | Install, update and test with friends |
| [DIAGNOSTICS.md](DIAGNOSTICS.md) | Verified results and known limitations |
| [Build guide](docs/BUILD.md) | Recreate this version from source |
| [Archived releases](docs/ARCHIVED_RELEASES.md) | Older builds, clearly separated from the current download |
| [Contributors](CONTRIBUTORS.md) | Maintainer and AI contribution disclosure |
| `src/` | Source files changed from official ReSkate |
| `mods.patch` | Applies those changes to the supported official source version |
| `tools/playtest/` | Backup installer and file checker |
| `SNAPSHOT.json` | Exact source and build identifiers for developers |

## Credits and AI assistance

Maintained by **SmokeyDawg58**. This project was developed with substantial assistance from **OpenAI Codex**, an AI coding assistant, for investigation, code changes, tests, packaging and documentation. The maintainer directs the work, performs the reported gameplay checks and controls publication. See [CONTRIBUTORS.md](CONTRIBUTORS.md).

Based on [official ReSkate](https://github.com/Dingo-Shenanigans/ReSkate). Source is licensed under GPL-3.0; see [LICENSE](LICENSE). No game executable, Steam DLL, unrelated mods or personal settings are distributed. The asset pack uses a Thunderstore-compatible folder format; it has not been published to a Thunderstore account.
