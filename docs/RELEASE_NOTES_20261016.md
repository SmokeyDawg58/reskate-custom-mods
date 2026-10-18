> Historical 2.0.7 release notes. Use [current 2.1.0 notes](RELEASE_NOTES_20261017.md) and the [current download](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/tag/v2.1.0-custom-20261017).

# ReSkate 2.0.7 custom — Deathrace editor, recovery and HUD

This release gathers the local playtest work since the previous public R.E.S.K.A.T.E. artwork release. Use the matching DLL, launcher and **complete Throwdown Assets 1.0.6** on every PC. The asset pack is updated from the previous public 1.0.2 package.

## Downloads

- **[Full playtest package](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.0.7-custom-20261016-deathrace-editor/ReSkate-2.0.7-Playtest-Full-20261016-Deathrace-Editor.zip) — recommended:** the matching DLL, launcher, all assets, backup installer, file checker and playtest notes.
- **[Required asset mod 1.0.6](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.0.7-custom-20261016-deathrace-editor/SmokeyDawg58-ReSkate_Throwdown_Assets-1.0.6.zip):** the complete mod-folder package, also included in Full.
- **[Asset authoring source](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.0.7-custom-20261016-deathrace-editor/ReSkate-Throwdown-Asset-Source-1.0.6.zip):** current Studio project, compiled FBMOD, original logos, gate-compass art, quiet sound inputs and author/export code.
- Individual [DLL](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.0.7-custom-20261016-deathrace-editor/ReSkate.dll) and [launcher](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.0.7-custom-20261016-deathrace-editor/ReSkateLauncher.exe), plus the source overlay and checksums, are available alongside these downloads.

## Deathrace builder

- Place the game's blue/yellow stunt Chevron alongside Required and Guide gates. It is a stationary world object: moving the camera does not rotate an already placed marker.
- Place Chevrons in open air, against walls or on the floor. Direct surface aiming mounts to that surface; distant ground does not pull an airborne marker downward.
- Upright Chevrons stay vertical at oblique camera angles. Snap aligns rows along **left/right, up/down and forward/back** axes. Clear movement toward another axis switches direction; small drift stays on the current line. Repeated placements avoid accumulated diagonal error.
- Ground clearance checks the full marker footprint from above, including lower corners of upright or tilted arrows and one-sided road meshes. New placements lift clear when needed. Existing saved object positions are not silently rewritten.
- **X toggles rotation mode.** Colored rotation rings show the active tool. D-pad Left/Right changes yaw and Up/Down changes tilt; the visual movement is smoothed. Outside rotation mode, Left/Right is Undo/Redo and Up/Down changes size. Hold Select / Clear all remains last in the builder instructions.
- New objects start at **125% size**, with **90-degree snapping** selected. Saved objects retain their authored size and pose.
- Saving without a Finish now explains what is missing. Gates and marker previews survive map changes, and complete routes preserve rotation/tilt and optional markers when saved, reopened and shared.

## Recovery and race flow

- Full bails and failed-trick falls return to the last cleared Required gate. Accidental standing board loss is detected too; deliberately getting off the board remains available.
- Full bails receive **two seconds** of grace; accidental rolling board loss receives **one second**. Finish crossings remain eligible during the grace period, allowing a player to slide through before recovery. These timing/finish combinations still need wider live checks.
- **D-pad Up returns to earlier cleared Required gates**, eventually the start, to escape a bad checkpoint. Crossing a Required gate again updates the recovery point so missed-gate recovery does not remain stuck at an older manual selection. Guide gates and Chevrons are not recovery checkpoints.
- Repeated bail or missed-gate respawns cannot accumulate extra speed. Recovery uses measured movement and an absolute velocity handoff, retaining the working speed protection.
- Recovery plays the owner's selected quiet **C / quick cue**. The Required gate uses quiet variation 23; Finish uses quiet variation 4 at matching loudness; Guides remain silent. The countdown audio path for 3/2/1/GO is included; full live audio coverage remains part of playtesting.
- Guest flag joining, Ready counts, host-only Start and guest-only Quit are included. A guest can leave without ending the host's challenge. Temporary competition restrictions restore the player's tools after finish/DNF.
- Server announcements are reduced to started/ended messages with the relevant time, without repeated hold-A join instructions.

## HUD and presentation

- The Deathrace brush title sits above the compass in a styled frame. The final correction centers the **visible lettering and 96-unit logo together**, with the icon to the left and their paint baselines aligned. It keeps the existing frame dimensions rather than widening the bar again.
- Placement and clocks sit close to the top/left edges. Required progress has its own styled box below placement. The moving race clock counts down; Time to Beat remains fixed.
- The larger next-gate symbol sits on the compass strip and follows the **aiming camera**, keeping a fixed size. It targets Required gates and then Finish, skips decorative/Guide markers, and clears when the race ends or the player leaves.
- Persistent previous-gate and Throwdown Panel prompts are aligned at the top-right edge. Side leaderboards retain their right-edge, side-height placement.
- Shared challenge headers, dynamic name/value/result frames and native player-data presentation are included. Existing native Start/Back/Quit handoff, HUD visibility ownership and the fixed presenter catalogue remain protected by regression checks.
- The supplied pink R.E.S.K.A.T.E. logo remains across its pages, HUD, map icons and flags; seven-letter rules remain included.

## Assets and packaging

Assets **1.0.6** include the map-resident gates, static stunt Chevron and glow, transparent compass gate image, logos/flags and current sized title/result frames. The complete authoring project contains **2,147 resources**. The player mod has **17 compiled native files plus two quiet WAVs**, with every required payload hash pinned to this runtime.

The lean DLL is about **14.6 MB**. The large native asset archive stays outside it. Diagnostic logging, profiling, crash capture and shipped debug symbols are disabled. No game executable, Steam DLL, personal settings or unrelated mods are included. This mod-folder package has Thunderstore-compatible metadata; no Thunderstore account upload is implied.

## Validation and remaining checks

All **79 automated Release checks pass**. The full kit, runtime resources, source reconstruction, asset hashes and ZIP contents are verified. The owner confirmed Chevron ground clearance, all three snap directions, and the larger compass gate's size, position and camera-relative movement on the preceding installed build; those features are unchanged by the final title-only correction. Earlier live checks confirmed repeated fall recovery with retained speed/quiet cue, deliberate-dismount exclusion, gates after map changes, and guest flag/controls/guest-only Quit behavior in their reported builds.

The owner confirmed the final title/logo alignment on the matching installed build. The complete release still needs wider multi-PC, disconnect/rejoin, long-session and grace-period finish testing. **Automated checks and earlier narrow confirmations do not certify every multiplayer scenario.** One native join confirmation may still call the challenge Spot Battle; that live investigation was deferred by the owner. R.E.S.K.A.T.E. rules playtesting was also deferred.

Close the game and launcher yourself before installation. Use the Full package's backup installer and file checker. Do not mix older runtimes or asset packs. See the detailed playtest checklist and SHA256SUMS.txt.

## Credits

Maintained by **SmokeyDawg58**, with substantial assistance from **OpenAI Codex** in investigation, implementation, tests, artwork integration, packaging and documentation. The owner directs the work and performs the reported gameplay checks. Codex is credited as an AI contributor, not a separate human or invented GitHub account. Upstream ReSkate attribution and GPL-3.0 are retained.
