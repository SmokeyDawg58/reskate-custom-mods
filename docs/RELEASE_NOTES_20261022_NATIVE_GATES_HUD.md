# Deathrace: Skate 2 style HUD and native world gates

This is the completed custom ReSkate 2.1.1 build before migration to the official 2.1.2 plugin API. It includes accumulated local work since Assets 1.0.15, with the final HUD cleanup and steady outer gate in Assets 1.0.27. The next plugin package is a separate deliverable.

## What changed

- The builder and race use the approved compact Skate 2 inspired presentation. Both side panels use the same configured opacity (75% by default). The native compass/location banner and lower-left D-pad remain clear.
- Gates and chevrons are native world effects, not projected ImGui geometry. The runtime uses EffectEntity/EffectManager ownership and normal native scene depth. Recovered Skate 2 silhouettes use the game's own mesh/material pipeline.
- The final gate has one outer outline at its authored size. The shrinking three-outline animation is removed for Required, Finish and Guide gates. Animated surface shading remains. Chevrons retain the previous native animation and material.
- Fixed the earlier double application of world placement and loss of inherited scale. The new gate-only simulation changes three scale multiplications relative to Assets 1.0.26; the other 388 instructions, binding layouts and native draw shaders remain unchanged. Shared stock resources and the Chevron effect are unchanged.
- During races, floating gate type/number/distance cards are removed. Builder numbering remains. The native top-right Previous gate prompt remains, without a duplicate bottom prompt. Center countdown and GO remain; the extra bottom red GO notice is removed.
- Native scalable game fonts, special-character names (including ✪Smokey✪), millisecond clocks and ordinal ranks are preserved. The position display supports 1st through 6th, including an ordinal total beside it.
- Header logos/titles and text align vertically. The race identity has a readable panel. Builder glyphs are larger. Its footer says Movement, with a full-size Hold/faster flight control.
- Route actions have visible boxes, larger shortcut icons beside left-aligned captions, and a clear A Select legend. D-pad/left-stick navigation works in all directions. Shortcuts: X rename, Y delete with confirmation, RB show folder, LB import JSON, B back.
- Leave-builder actions have visible backing boxes. Existing native host Hold Start/join flow remains the owner of starting and joining.
- Orientation Reset records the object's default aim rather than a nearby rotated marker. It clears the old Arrow snap rail, retains size/position, and restores automatic camera/floor alignment.

## Preserved gameplay and route features

Required gates remain ordered; Finish is ignored until all are cleared. Duration is configurable. Players line up with the flag and remain stationary for countdown. Native offboard/spread-eagle/rolling recovery and the genuine-bail grace remain. Recovery returns behind the appropriate gate with runway and onboard approach speed.

Routes are named readable JSON, restricted to their map, with display names from map manifests, rename/folder/delete/import actions, stored flag placement and load arrival. Menu presses are consumed through release so Save/Load confirmation cannot place a gate. Unchanged loaded routes can go straight to play. The old HUD/source and prior installed builds remain available as backups.

## Verification

- 93/93 Release checks passed, including route storage, input/navigation, ownership/lifetime, default rotation and one-emitter gate readiness.
- 70 actual offscreen ImGui/DX12 captures passed: 1080p, 4K, 4:3, ultrawide and six ordinal placements. Native fonts and circled-star glyphs were verified.
- Independent asset audit: 2,456 existing resources unchanged, exactly three private gate roots changed, three private dependencies added; all seven previous private mesh/simulation resources preserved.
- DXIL assembled, validated, disassembled and compared; 2,424 age/width/yaw size cases passed.
- Compaction verified 17,827 native references across 40 bundles, 12 TOCs and 381 chunks. Every encoded payload is byte-identical after compaction; 330,725,167 duplicate CAS bytes were removed.
- Full asset ZIP: 31 files, 25 pinned payloads. Runtime/launcher, fonts, atlas, hashes and exact source reconstruction verified.

Offline checks verify implementation, layout and packaging. User screenshots confirm the preceding 1.0.26 effects were visible; they do not establish live success for every change in this 1.0.27 build. This release remains a playtest prerelease. No game/launcher was controlled, no personal routes are shipped, and no historical release is removed.

## Install and compatibility

Use the matching DLL, launcher and full Assets 1.0.27 ZIP on every PC. Close game and launcher, back up current files, replace the runtime pair beside Skate.exe and replace the single enabled Throwdown Assets mod folder. Do not mix the asset versions. This C++ build targets custom ReSkate 2.1.1, protocol 52, mode wire 16, 1-UP wire 5. Do not install its DLL over official 2.1.2 and expect the new plugin host.

A separate migration will package this mod's behavior using the official 2.1.2 generic Lua APIs, with no custom ReSkate source patches. Any SDK gap will be recorded explicitly rather than hidden behind offsets or a custom runtime.

## Source and downloads

Official reconstruction base: `db99ed4d724dd77edd2abf62f3de0dbc5bb6717e`. Exact custom source tree: `93a0c59e355ed878f08c30a8124399b99096e50e`. `mods.patch` includes deletions and binary source art. `src/` contains the changed canonical Git blobs; `BUILD_SOURCES.json` records the raw compiled inputs. The native authoring archive can be reconstructed from the matching asset ZIP. No game executable, Steam DLL, credentials, personal data or debug dumps are included.

Three player downloads only, following the repository's existing policy. Full source and validation notes live in the repository. Historical releases are preserved.
