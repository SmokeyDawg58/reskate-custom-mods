# ReSkate custom Throwdowns — native gates and Skate 2 HUD

**[Current 2.1.1 playtest build](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/tag/v2.1.1-custom-20261022-skate2-hud-native-gates) · [DLL](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.1-custom-20261022-skate2-hud-native-gates/ReSkate.dll) · [Launcher](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.1-custom-20261022-skate2-hud-native-gates/ReSkateLauncher.exe) · [Assets 1.0.27](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.1-custom-20261022-skate2-hud-native-gates/SmokeyDawg58-ReSkate_Throwdown_Assets-1.0.27.zip)**

Compact Deathrace HUD, native animated world gates/chevrons, JSON route library, custom R.E.S.K.A.T.E., and native enroll/join/host Hold Start flow. This update keeps one steady outer gate, improves controller/menu readability and fixes default orientation reset after object changes. [Detailed changes and verification](docs/RELEASE_NOTES_20261022_NATIVE_GATES_HUD.md).

**93 automated checks and 70 actual GPU HUD captures pass.** Live testing of this exact Assets 1.0.27 candidate is pending. The preceding 1.0.26 effects were confirmed visible by the owner.

This is the completed custom **ReSkate 2.1.1** runtime, archived before migration to the official **2.1.2 Lua plugin API**. The forthcoming Thunderstore plugin will be separate and use stock ReSkate. This release DLL does not contain the 2.1.2 plugin host.

## Install

Use all three matching downloads above. Close game and launcher, back up the current runtime pair and asset mod, replace the pair beside Skate.exe, then replace the complete enabled Throwdown Assets folder under Mods. Keep one copy enabled. All PCs need matching files. [Asset details](docs/ASSET_PACK.md).

## Routes and controls

Tap Start to save/name (or play an unchanged loaded route); hold Start to load. The library filters the active map. D-pad/left stick navigates all directions; A selects, X renames, Y deletes after confirmation, RB reveals the JSON file, LB imports JSON, B goes back. Names use a physical keyboard. Saved routes are readable/shareable JSON with flag placement and objects under Mods/ReSkateRoutes/Deathrace. Load restores the flag and brings the player to the course.

Builder: L3 holds faster flight, R3 changes placement snap; while rotating, L3 resets default orientation. Native host Hold Start and Join remain unchanged. Required gates must be cleared in order before Finish.

See [playtest checklist](PLAYTEST.md), [validation](DIAGNOSTICS.md), [build instructions](docs/BUILD.md), and [changelog](CHANGELOG.md). Protocol 52, mode wire 16, 1-UP wire 5. Historical releases and old HUD/source backups are preserved.

Maintained by SmokeyDawg58. Upstream authorship, GPL-3.0, R.E.S.K.A.T.E. contribution credits and substantial OpenAI Codex assistance are retained; see [credits](CONTRIBUTORS.md).
