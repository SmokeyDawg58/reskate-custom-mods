# ReSkate 2.1.0 — local reskate-mode-210 main merge

This is the current custom ReSkate 2.1.0 build, with the R.E.S.K.A.T.E. letters-game feature, enrollment-only multiplayer flow, the feature branch's custom-map protections and the Deathrace follow-up. LOCAL ONLY: nothing has been pushed to GitHub.

## reskate-mode-210 merge

Incoming branch commit: 00c5fa2b4dedebb8849cdb289d44ef20a25f78ea, merging the letters-game playtest on the current 2.1.0 enrollment main f6301ad. The local main branch contains that ancestry. Nothing is pushed.

- R.E.S.K.A.T.E. spectators cannot skate while somebody else takes their turn.
- TRICK COPIED!, FAILED TO SET TRICK and Not a valid trick feedback is carried over.
- Return positions spread players around the start and face them into the zone.
- A landing must hold for one second; falling/stepping off before then misses the attempt.
- Flips into grinds/grabs and manuals landed into remain part of the trick, in order.
- Flick diagrams follow the native Default Stance Is Goofy gameplay setting.
- The diagnostic-only DINGOSDK_LEAN_LOGGING option is available in source and explicitly OFF in this shipped build.

This merge preserves the Deathrace version already in the workspace before the branch request. Dedicated Deathrace source/assets and its shared editor, bail, checkpoint/recovery, advertisement, enrollment and host-start code are unchanged by this feature merge. Existing integration safeguards for non-Skate strike limits, guest custom zones, HUD slot allocation and world teardown are retained. The earlier Deathrace/map work below is carried forward, not reworked by this branch merge.

## Existing custom-map and HUD protections retained

- Merge the reskate-mode branch's external Throwdown asset ownership rule. Dynamically selected HUD variants, private gate roots and their shader now survive a custom map replacing the shared core bundle, for both the DLL-extracted bundle and the separately installed asset pack. It works in either mod order and accepts the numeric versioned pack folder.
- Merge the branch's missing-GUID lookup protection. The supported game's original lookup can probe forever when a custom map lacks a requested stock effect. The guard checks each slot once; a genuinely absent key receives the native no-items handle. Existing keys still go through the original function. Both native entry points are fingerprinted before installation; unreadable tables retain the game's original behavior. Regression tests cover collisions, empty slots, full missing-key tables and all probe strides.
- Keep the newer 2.1.0 render-resource, light, mesh, shader, spatial and other map protections already present. No older engine, protocol-50, server or music backport replaces those files.
- Keep the challenge HUD independent of stock Compass/D-Pad preferences and preserve native world/manager lifetime cleanup. Update exact selected flag/map-card identity before the menu's gameplay early return, so a Deathrace card opened from the map can receive its own title, logo and details.

## Earlier Deathrace work preserved

- Explicit Save publishes a complete route. A finish mesh by itself does not advertise a lobby. Editing withdraws publication; peers receive no unfinished gate geometry, route spawn or finish. Leaving/canceling the host draft sends its End. The stock donor relay no longer advertises a custom queue separately. The host still has to Save again after edits.
- Passed gates disappear locally. D-pad Up reveals the checkpoint selected for rewind and the gates ahead; passing those revealed gates hides them again. Decorative Chevrons remain visible. Each player's authoritative race progress is retained.
- The compass uses the regular blue/yellow gate arch for checkpoints and a new red checkered Finish arch for the last gate. Existing fixed size and camera-relative bearing behavior remain.
- Replace the oversized start banner with a bounded title/logo row and short wrapped rules. Its frame and heading use the selected challenge's theme. The existing five-second shared countdown and three/two/one start timing remain synchronized.
- A Chevron can yaw, tilt and roll. X opens rotation; Left/Right yaw and Up/Down tilt. R3 switches Up/Down between Tilt and Roll for Chevrons; L3 retains snap-step down. Outside rotation, Left/Right Undo/Redo and Up/Down Size remain separate. Camera following, snapping defaults and existing rings remain.
- Start flag mesh and its placement preview are 40% taller. Only the private Deathrace mesh transform changes; activity bounds, player spawn, teleport marker and stock flags retain their original transforms.
- Undo/Redo history survives leaving and reopening the local route. A restored course without session history receives a bounded placement history, including the exact Finish/tilt/roll settings. Joining somebody else's course does not overwrite the owner's saved draft/history.
- Gates can be aimed into open air. A nearby direct surface still supplies contact; an absent or distant surface leaves the preview at the aimed air depth.
- Missing an opening alone no longer teleports you. Crossing a later Required/Finish gate before the next required checkpoint warns that a gate was missed and returns you to the last valid cleared checkpoint. Guides/Chevrons and backward crossings do not trigger this rule. Existing momentum limits and true-bail recovery remain.
- Hippy-jump recognition covers riding/airborne to offboard handoffs, delayed upward samples and the descending jump. The jump remains eligible for checkpoint progress and clears its false recovery grace. A true native bail still has priority. The two-second bail and one-second accidental runout grace remain.
- A separate right-side Route Info panel shows Required/Guide/Chevron counts, Finish status, selected object, placement height, size, route length and saved/draft status. Switching tools shows a short top-middle object label below the compass, with a private-style fade where the supported native color layout exposes alpha. Hold Select stays last in the controls.

## R.E.S.K.A.T.E. and online behavior retained

The earlier local merge is included: seven-letter rules, native trick/stance/spin recognition, solo set/copy, 320 Skatepedia input entries, trick pictures/callouts/used-trick list, Letters/Turn Timer/Tricks/Zone setup, custom cone editor, guide-bolt fallback on custom maps and zone beacon. Existing pink artwork is preserved. Guests enroll once and wait for the host; there is no Ready requirement. Host Start, capacity and countdown holds, Leave/re-enroll, full-identity 1-UP flags, native join controls, waiting-banner lifecycle, protected recovery speed and quiet audio remain. Wire versions stay session 52, modes 16 and 1-UP 5.

## Assets and install

Use the matching runtime pair and complete Assets **1.0.7 on every PC**. Full includes the pack, backup installer and file checker. Runtime Update alone requires Assets 1.0.7 installed first; the prior 1.0.6 pack does not match. Replace the matching old asset folder instead of enabling two versions. Use the unversioned folder Mods/SmokeyDawg58-ReSkate_Throwdown_Assets or its numeric version suffix. ModData installations are supported by the installer.

Close ReSkate and its launcher yourself before running Install-Playtest.ps1. Then run Preflight-Playtest.ps1 and launch yourself. The installer backs up replaced files, preserves settings/unrelated mods, rejects running programs and rolls back on failure. This package does not start or stop either application.

The pack retains every pre-existing resource's metadata, bundle placement and dependencies. Existing gate, Chevron, shader, logo and HUD resource payloads are byte-identical; only the two private Deathrace registration/placement transform payloads change, alongside the added finish texture. Both external WAVs are unchanged. Asset authoring source and its current Studio project are supplied separately.

## Verification and practical limits

All 91 configured optimized Release checks passed, including real mod-merge fixtures for the external pack on replacement maps, bounded GUID lookup, route publication/privacy, native donor relay, history restoration, gate hide/rewind, later-gate recovery, hippy states, HUD painter order and timed object labels. Native asset preservation, exact source reconstruction, archive checksums and isolated backup-installer tests are recorded separately.

The code addresses the reported missing HUD and the branch's recorded missing-asset freeze. Live validation of this exact combined runtime remains PENDING; this does not claim every possible custom-map crash is eliminated. Selected-map/nearby card behavior, two-PC drafts/enrollment, hippy jumps, new editor layout and long repeated map travel still need in-game confirmation. No request to reproduce now is required; check whenever convenient.

Production logging, profiling, tracing, crash uploads and shipped symbols remain disabled. No game executable, Steam DLL, personal settings, logs, private capture, recorder, debugger or credentials are distributed.

## Credits and provenance

Official ReSkate 2.1.0 base: ba1245a5af51892e90f2be8637dd69a7e20f5b77. Current feature checkpoint: 3f2094887fd3b2b448daa8f529d4b776969b24d6. R.E.S.K.A.T.E. input: SmokeyDawg58/reskate-custom-mods reskate-mode-210 00c5fa2b4dedebb8849cdb289d44ef20a25f78ea (including e81499f and a3ff4c8), authored by hunterjedward123 with the recorded Claude Opus 5.5 co-author credit. SmokeyDawg58 maintenance and upstream authorship are retained. Integration, additional fixes and checks received substantial OpenAI Codex AI assistance. GPL-3.0 is retained. No publication was performed.
