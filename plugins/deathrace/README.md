# Deathrace Lua 1.0.38 — approved local preview

The working Deathrace course builder, native animated gates/chevrons, saved flag relocation and compact Skate 2 HUD now run as a separate Lua/native-assets mod. Adds only Deathrace to Throwdowns. The owner approved this exact package in game: “ok from what I can tell its perfect now”.

**Requires the separate generalized SDK test runtime** at local commit `190d4edb46b7f24337667f843b0c809aa1baaed9`. Those APIs are not all in public ReSkate 2.1.2. This is a prerelease snapshot for the local SDK test setup, not a plug-and-play release for stock 2.1.2. The SDK is not bundled, pushed or submitted as a PR here. No Thunderstore listing is created.

Download `SmokeyDawg58-Deathrace-1.0.38.zip`, create `Mods/SmokeyDawg58-Deathrace` and extract the ZIP contents into that folder (the archive has mod files at its root), and enable one copy after closing the game and launcher. Keep routes outside the package under `Mods/.reskate_plugin_data/SmokeyDawg58-Deathrace/routes/`. Preserve your existing saves when updating. Every player needs matching compatible versions; two-PC multiplayer remains unverified.

## Included behavior

- Native world-depth gates and animated chevrons; no projected HUD world geometry.
- Native join and host Hold Start flow, restored waiting flag and teleport to a saved course.
- Compact 75% opacity HUD, sharp native font atlases, special-character player names and ordinal places.
- Camera-following placement, persistent rotation, floor-mounted chevrons and original orientation reset.
- Route library with all-direction controller navigation and quick actions; held Backspace deletes full Unicode characters.
- Removed gate-progress callouts. Original 48 kHz stereo gate cues request non-positional playback.
- Gate compass artwork follows the native compass visibility preference.

The archive is byte-identical to the approved local package and contains no ReSkate DLL, launcher, executable, test probe or personal route. Four relevant SDK test groups, the offline Lua integration/input/route checks, native package discovery and lossless asset audits passed. Owner playtesting is separate from these offline checks; wider multiplayer and long-session validation remain open.

## Source and assets

`plugins/deathrace/` contains the exact packaged Lua, HUD atlases, sound files and plugin metadata. Compiled native assets are supplied only in the release ZIP. Historical C++ source snapshots, assets and releases are preserved; they are not part of this Lua mod. Existing upstream authorship, GPL-3.0 and contributor credits remain.

New Skate 3/skate. visual choices are subsequent work and are not in this approved snapshot. No SDK PR is opened by this release.
