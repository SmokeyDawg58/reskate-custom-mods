# ReSkate custom Throwdowns

Custom Deathrace, 1-UP and seven-letter R.E.S.K.A.T.E. for ReSkate 2.1.0.

## Latest work: enrollment and HUD playtest

**[Playtest release](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/tag/v2.1.0-custom-20261018-enrollment-playtest) · [Full package](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261018-enrollment-playtest/ReSkate-2.1.0-Enrollment-Playtest-Full.zip) · [Runtime Update](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261018-enrollment-playtest/ReSkate-2.1.0-Enrollment-Playtest-Runtime-Update.zip)**

This is the latest installed custom build, published for shared testing. **Known issues remain:** the challenge HUD can disappear after changing to a custom map, and Deathrace interaction/map cards can still display Spot Battle. It is a prerelease, not a claim that those reports are resolved.

Enrollment now replaces the extra Ready step: guests enroll and wait for the host to start everyone. This follow-up also contains shared-flag/Leave/rejoin handling, selected-flag join details, HUD-root and asset-lifetime changes, hippy-jump filtering and native waiting-banner cleanup. **All 90 automated Release checks pass.** See [detailed changes and limits](docs/RELEASE_NOTES_20261018.md) and [DIAGNOSTICS.md](DIAGNOSTICS.md).

| Download | Contents |
| --- | --- |
| [Full](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261018-enrollment-playtest/ReSkate-2.1.0-Enrollment-Playtest-Full.zip) | Matching runtime pair, complete Assets 1.0.6, backup installer and checker |
| [Runtime Update](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261018-enrollment-playtest/ReSkate-2.1.0-Enrollment-Playtest-Runtime-Update.zip) | Matching runtime pair; requires complete Assets 1.0.6 already installed |
| [Complete Assets 1.0.6](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261018-enrollment-playtest/SmokeyDawg58-ReSkate_Throwdown_Assets-1.0.6-For-2.1.0.zip) | Native logos, flags, gates, Chevrons, compass/HUD artwork and quiet audio |
| [DLL](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261018-enrollment-playtest/ReSkate.dll) · [Launcher](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261018-enrollment-playtest/ReSkateLauncher.exe) | Individual matching runtime files |
| [Source overlay](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261018-enrollment-playtest/ReSkate-2.1.0-Enrollment-Playtest-Source-Overlay.zip) · [Asset authoring source](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261018-enrollment-playtest/ReSkate-Throwdown-Asset-Source-1.0.6.zip) | Source, exact reconstruction patch and unchanged asset project |

Native/audio payloads remain Assets 1.0.6; the included asset instructions are refreshed. Every PC needs this matching custom runtime. Session 52, modes 16 and 1-UP 5 remain unchanged. Use [PLAYTEST.md](PLAYTEST.md) to install and check it. Keep the complete asset folder together, enable one copy, and close the game and launcher yourself before installation.

The [previous public 2.1.0 release](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/tag/v2.1.0-custom-20261017) remains available. Its earlier Improved-map success report does not verify this changed build.

## Source and project guide

- [Build guide](docs/BUILD.md): pinned official base plus mods.patch.
- [Asset-pack guide](docs/ASSET_PACK.md): complete player pack and authoring project.
- [Changelog](CHANGELOG.md), [previous release notes](docs/RELEASE_NOTES_20261017.md) and [older releases](docs/ARCHIVED_RELEASES.md).
- src/, mods.patch, SNAPSHOT.json and BUILD_SOURCES.json identify the exact source and runtime; tools/playtest/ contains the installer/checker.

Maintained by SmokeyDawg58 with substantial OpenAI Codex AI assistance. Based on [official ReSkate](https://github.com/Dingo-Shenanigans/ReSkate), with original authorship and [GPL-3.0 license](LICENSE) retained; see [CONTRIBUTORS.md](CONTRIBUTORS.md). No game executables, personal settings or private diagnostic evidence are distributed. No Thunderstore listing is created.
