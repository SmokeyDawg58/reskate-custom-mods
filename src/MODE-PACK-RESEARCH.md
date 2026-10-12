# Native Throwdown pack: source audit and Deathrace implementation

This records the implemented code, the game's verified asset/ABI dependencies, and remaining gaps. Historical sections of `THROWDOWN-INTEGRATION.md` describe earlier builds; the current selection behavior, native checkpoint objects and wire version are described here.

## Shared path

`Extension/Throwdowns/challenge_registry.*` registers twelve picker entries: the three stock Throwdowns, native 1-Up, and eight added modes. Each added tile has its own callback. The stock S.K.A.T.E. entry continues to use the game's implementation; the imported generic 1-Up/S.K.A.T.E. rules are not duplicate menu entries.

`Extension/UI/NativeMenu/one_up_menu.cpp` and `throwdown_modes_ui.inl` publish private native Rime models for configuration, placement hints, intro/countdown, score, timer, standings and completion. `Extension/Modes/game_modes.cpp` handles the session/runtime adapters; `mode_rules.*` contains deterministic rules and the private version-10 wire codec. Session receive/tick/send dispatch is in `Extension/Multiplayer/Session/session.cpp`. The normal upstream multiplayer protocol is 47 after the update.

## Mode audit

| Mode | Rules and runtime data | Presentation / material gaps |
|---|---|---|
| Spot Jam | Trainer-observed landed lines in a placed area accumulate. | Native score/timer/standings; the imported telemetry adapter needs comparison with exact native landed-trick scoring. |
| 1-Up | Existing `one_up_match`, runtime, native bridge and skating-score tap: timed turns, beat the previous total, 1/U/P elimination, last standing wins. | Native HUD/start animation/flag, 1–6 players, default 20 seconds, no rounds slider/scoring circle/multiplier. Existing pause Throwdown Panel and winner showcase integration still need completion. |
| Hall of Meat | Official bail tracker, `set_forced` and `take_finished_bail`; only finished scored bails feed the mode. | Native mode counters and leaderboard. Official standalone overlay is suppressed while a custom mode owns play. |
| Deathrace | Ordered finite-width gate-line crossings; per-gate yaw/width, per-racer deadline, finish elapsed time and DNF ranking. Each ordinary gate adds 2 seconds once. | Native Spot Battle start flag, tutorial checkpoint meshes, red finish, native side leaderboard/clock. See builder below. |
| Domination | Best line captures placed spots; holding scores one point per second. | Native notices and standings; no new 3D territory decal is claimed. |
| Graffiti | Larger lines steal tags on the imported grind/gap telemetry paths. | Native tag notices and score; actual THPS-like grind-path/gap recognition requires further native-data validation. |
| Skate Tag | Positions sent every 150 ms; initial it is `players[game % n]`, with a 3-second no-tag-back period. | Native target/player notices and standings. Multiplayer contact/latency still needs session testing. |
| S.K.A.T.E. | Stock picker entry remains the actual challenge. Imported generic rules/trick-name tap exist in the source, with flip/grab/grind/manual options. | The generic implementation is not presented as a second stock card. |
| Infection | Caught players stay infected; survival time scores. | Native infected/player notices. Remote Grim Reaper costume application is not wired in this adapter. |
| Hide & Seek | `turn_s` is hiding time; `target` carries hiding milliseconds left and `aux` carries finds. Found players become seekers. | Native blackout during hiding and input pause; hider nametags/map dots/chat bubbles suppressed. Hot/cold data is published through native hints. Multiplayer behavior needs testing. |

## Deathrace building and play

Choose Deathrace in Throwdowns, configure Players and the starting clock, then Place Route. The first object is the game's Spot Battle flag-placement sequence. Its completed position and full facing are captured; the temporary native queue is retired using its normal destroy route. It does not run S.K.A.T.E.'s scoring-area stage.

The builder then stays active while objects are placed:

- **LT / RT** selects Start / Checkpoint / Finish (keyboard Z / X).
- **A / Enter** places the selected object; another checkpoint remains ready to place. Selecting Start reopens native flag placement and moves the existing start instead of appending another.
- **D-pad Up / mouse wheel / Page Up or Down** adjusts yaw; Ctrl + wheel or brackets adjusts gate width. Keyboard Q/E moves the free camera vertically.
- **D-pad Left / Backspace** removes the last gate; the start stays.
- **D-pad Down / F / End** saves the route after it has a start, at least one ordinary checkpoint and exactly one finish.

The route holds at most 16 points (start, up to 14 ordinary gates, finish). Moving the finish replaces it; adding another ordinary gate inserts before the finish. `race_route.h` keeps position, yaw and width arrays aligned and validates finite input. Confirm and trigger edges are latched after flag handoff so a held button does not place an accidental gate.

Start is blocked until the placed native objects are loaded and created in the current world, and the builder is closed. The runtime uses a full native teleport transform from the flag heading rather than passing an ignored fourth argument to the trainer's position-only `tp` command. The common session path requests on-board placement and holds gameplay input during the native intro/countdown. Session marker restrictions apply while the mode owns play.

During the race, a gate counts only in the intended direction, within its width and height tolerance, and in route order. Duplicate or expired crossings cannot award time. The host adjudicates gate progress and publishes independent remaining time for each racer. Start and finish do not award the two-second bonus. Finish records elapsed time; an expired racer becomes DNF; all finished/out racers advance to native results. A private native leaderboard sits to the side with GATES / TIME, while the existing ActivityCountdown widget shows the local racer's remaining time.

## Native world assets and verified ABI

`race_world.*` instantiates four native mesh children per checkpoint using the game's generic ObjectBlueprint factory and the tutorial prefab's transforms. The donors are `SM_FX_GP_Checkpoint_A_AT`, `SM_FX_GP_Checkpoint_A_AL` and `SM_FX_GP_LightningBolt_A` from the tutorial checkpoint prefab. Six private blueprints under `World/ReSkate/Deathrace/` preserve the original mesh/material links and provide cyan ordinary gates and red finishes through native static shader parameters. Original game blueprints are not edited globally.

`ReSkateStudio/src/cli/one_up_badge.cpp` packages all original mesh/texture/LOD chunks. Shader depots are identified through actual mesh material keys and shared native bundle membership, not guessed filename prefixes. The required depot is in `bam_coregameassets`: `levels/game/bam_levelroot/bam_coregameassets_win32_shaderstate/shaderblockdepot_12937733901070835605`. The final native project contains 104 resources in twelve bundles/ten TOCs.

The targeted Ghidra decompilation verifies the supported build's entry points: descriptor initialization RVA `0x2e90d90`, creation `0x2e93070`, placement `0x2f7d670`, destruction `0x2b689f0`. The creator runs in the verified client/TLS scope used by native remote-skater entities. Prefix checks, exact ownership/context tracking and the existing destruction hook prevent stale entity use. Leaving, results, level/context changes and externally destroyed children invalidate readiness; only owned entities are retired. No guessed vtable setter or external 3D renderer is used.

The placed graphics are actual game objects. Ordered crossing/time-award rules are implemented in the pack, not claimed to be the original tutorial ECS trigger graph. IDA MCP had no active database during this audit; the new targeted decompilation evidence is from Ghidra plus the existing verified source ABI.

Evidence is in `../deathrace-checkpoint-vfx.txt`, `../deathrace-native-mesh-bundles.txt`, `../deathrace-native-assets-compile.log` (or the root build log), `../deathrace-bundle.log`, `../re-tools/ghidra-deathrace-entity-functions.json`, and `../deathrace-ghidra-verified.log`. The author source and compiled native project remain in the workspace, not a new handoff ZIP.

## Selection outline and pulse

The approved transparent 4-by-3 grid and diagonal labels retain their full-size native frames. Tile background states stay empty. Hover, focus and selected states use a private native 6-pixel ellipse outline; artwork uses the native all-white palette and does not become blue. Selected/focused lettering retains white glyphs and uses the native aqua outline FontEffect.

Each tile's private native button graph gates a `TransformOscillator` through its own IsActive signal (hover or focus). Its 0.9-second halves use sine easing between scale 1 and 1.022. The whole logo/name selection pulses without replacing any callback. The native combination preserves property, focus and lifetime bindings. The asset author round-trips dimensions/graphs and verifies all ten original Throwdown defaults remain unchanged.

## Checks and limits

Rules tests cover ordered crossings, independent clocks, the exact two-second bonus, duplicate/expired crossings, deadline behavior, finish/DNF ordering, route capacity, moved start/finish, aligned arrays, control cycling, facing-preserving spawn transforms and wire round trips. The native flag-placement test covers race ownership, handoff once, cancellation and cleanup alongside existing 1-Up behavior.

The DLL embeds the 15-file native pack (108,065,886 bytes), SHA-256 `22a4497c68abe1e3edf028659c978a8d69d34c02349d251d0e3365f897c9d80d`. `verify_oneup_embedded_bundle.py` checks the DLL against this exact archive. Read-only resource probes verify residency; the user performs rendered gameplay checks. No computer-control input is used.

Do not mark the whole pack polished from a passing build. Native winner emotes/showcase cameras, 1-Up's pause Throwdown Panel, costume changes and exact graffiti tagging remain specific integration gaps. Test actual route placing, gate appearance/crossing, solo and peer races, quitting, reopening another challenge and world changes before claiming end-to-end multiplayer validation.
