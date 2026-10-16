# ReSkate custom Throwdowns

Custom challenges for ReSkate 2.0.7, including Deathrace and seven-letter R.E.S.K.A.T.E.

## Get the current playtest

**Use the [current release](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/tag/v2.0.7-custom-20261015-reskate-refresh). Every PC needs the matching DLL, launcher and complete asset mod 1.0.2.** Replace older versions together.

| Download | Contents |
| --- | --- |
| **[Full playtest package](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.0.7-custom-20261015-reskate-refresh/ReSkate-2.0.7-Playtest-Full-20261015-Reskate-Refresh.zip) — start here** | DLL, launcher, all required assets, backup installer and file checker |
| [Required asset mod 1.0.2](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.0.7-custom-20261015-reskate-refresh/SmokeyDawg58-ReSkate_Throwdown_Assets-1.0.2.zip) | Logos, flags, gates and HUD artwork; also in the full package |
| [DLL](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.0.7-custom-20261015-reskate-refresh/ReSkate.dll) and [launcher](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.0.7-custom-20261015-reskate-refresh/ReSkateLauncher.exe) | Individual matching runtime files; both are required |
| [Asset authoring source](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.0.7-custom-20261015-reskate-refresh/ReSkate-Throwdown-Asset-Source-1.0.2.zip) | Current Studio project, FBMOD, original logo, selected WAVs and authoring code |

The pink R.E.S.K.A.T.E. skateboard logo is used on its picker, pages, HUD, map markers and flags. The Deathrace header is larger and centered. Finish audio now follows the final result transition; bail recovery returns on the board at the last Required gate and keeps its measured speed request until physics applies it. Required/Finish audio stays quiet; Guides stay silent. The separate Required progress box and working clock layout remain.

Guests choose Ready; only the host starts after joined guests are ready. Guest Quit leaves the host's race running. Every player uses the same build (mode protocol 13); do not mix older runtimes or asset packs.

## Install

1. Close the game and launcher yourself.
2. Extract the full package and follow [PLAYTEST.md](PLAYTEST.md). The installer backs up replaced files and preserves other mods/settings.
3. Keep the complete mod at `Mods/SmokeyDawg58-ReSkate_Throwdown_Assets`, with `layout.toc`, `Win32` and `initfs_win32` directly inside it. The installer also supports ModData. Replace the previous asset folder rather than enabling duplicates.
4. Run the included file checker, then launch yourself.

## Validation and known issue

All **79 automated checks pass**, including a new solo-finish audio transition regression. The asset audit verifies exactly two changed texture chunks, with all identities, bundles, HUD animations and other resource payloads preserved. The lean DLL is about 14.5 MB; logging, profiling, crash capture and shipped debug symbols are disabled.

The owner previously confirmed native guest flag Join, controls returning after finish/DNF, and guest-only Quit on two PCs. Missed-gate recovery speed and the separate progress layout were also confirmed. **The latest finish-sound, bail-recovery and title changes still need an in-game check.** This is a playtest, not certification of every multiplayer scenario.

Known follow-up: One native join confirmation may still say Spot Battle; live inspection was deferred by the owner.

## Repository guide

| Location | Purpose |
| --- | --- |
| [PLAYTEST.md](PLAYTEST.md) | Installation and friend-playtest checks |
| [DIAGNOSTICS.md](DIAGNOSTICS.md) | Verified scope and remaining checks |
| [Build guide](docs/BUILD.md) | Recreate the matching runtime |
| [Asset-pack guide](docs/ASSET_PACK.md) | Logo source, authoring tools and export instructions |
| [Archived releases](docs/ARCHIVED_RELEASES.md) | Previous downloads, kept for reference |
| `src/` and `mods.patch` | Complete modified overlay against pinned official source, including logo/audio inputs |
| `tools/playtest/` | Backup installer and checker |
| `SNAPSHOT.json` | Exact source hashes and upstream version |

## Credits and AI assistance

Maintained by **SmokeyDawg58**, with substantial assistance from **OpenAI Codex** for investigation, code, tests, artwork integration, packaging and documentation. The maintainer directs the work, performs the reported gameplay checks and controls publication. See [CONTRIBUTORS.md](CONTRIBUTORS.md).

Based on [official ReSkate](https://github.com/Dingo-Shenanigans/ReSkate). Source is GPL-3.0; see [LICENSE](LICENSE). No game executable, Steam DLL, unrelated mods or personal settings are distributed. The mod folder is Thunderstore-compatible; no Thunderstore account listing is claimed.
