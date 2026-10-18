# ReSkate 2.1.0 custom — enrollment and HUD playtest

**Playtest prerelease. Known issues remain:** challenge HUD can disappear after travel to a custom map, and Deathrace interaction/map cards can still show Spot Battle branding. These were reported on this exact installed runtime after its automated checks passed. This release preserves the latest work for shared testing; it does not claim these two issues are fixed.

## Downloads and installation

- [Full package](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261018-enrollment-playtest/ReSkate-2.1.0-Enrollment-Playtest-Full.zip): matching DLL and launcher, complete Assets 1.0.6, backup installer and file checker.
- [Runtime Update](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261018-enrollment-playtest/ReSkate-2.1.0-Enrollment-Playtest-Runtime-Update.zip): for an installation that already has the complete Assets 1.0.6 folder.
- [Complete asset pack](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261018-enrollment-playtest/SmokeyDawg58-ReSkate_Throwdown_Assets-1.0.6-For-2.1.0.zip) · [source overlay](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261018-enrollment-playtest/ReSkate-2.1.0-Enrollment-Playtest-Source-Overlay.zip) · [asset authoring source](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261018-enrollment-playtest/ReSkate-Throwdown-Asset-Source-1.0.6.zip).

Close ReSkate and its launcher yourself before installing. Extract the package, run Install-Playtest.ps1 for the game folder, then Preflight-Playtest.ps1. Backups preserve replaced files. All participating PCs need this matching custom runtime. Session protocol 52, mode wire 16 and 1-UP wire 5 are unchanged; matching the complete build is still required because lobby behavior changed.

## Changes since the previous GitHub release

### Enrollment and host-controlled starts

- Custom challenge enrollment is the complete guest action. Guests wait for the host, with Leave available and no Ready button or ready count.
- The host starts the enrolled roster. 1-UP no longer needs an extra Ready step either.
- Host authority, capacity limits, initial position confirmation and countdown input holds remain enforced.
- Leaving an unstarted Deathrace permits enrollment again instead of permanently suppressing its native flag.

### Shared flags and challenge details

- A Deathrace draft is kept private while its route is being edited. Its shared offer is advertised after Save; re-editing withdraws the completed offer.
- 1-UP native flag offers retain the full 64-bit match identity, position, facing, player/timer settings and capacity. Join adopts the matching unstarted native registration; pre-start Leave allows the offer to return.
- Join-page identity is resolved from the selected native flag rather than the donor Spot Battle label. The custom card supplies the actual challenge name, logo, rules, players and mode-specific gate/deadline or penalties/timer details.
- The nearby interaction adapter also uses the focused native flag's identity and restores borrowed presentation fields when ownership changes. **This adapter does not yet resolve the reported incorrect nearby/map cards in all cases.** Normal stock flags are not globally renamed.

### HUD settings and map lifetime

- Custom HUD root selection accepts the live mounted HUD even when stock Compass and D-Pad sections are hidden. It excludes the authored default root and handles ambiguous candidates conservatively.
- Map teardown clears borrowed asset anchors and cached primary input references. Lookup skips expired anchors and continues through resident asset domains.
- Challenge widgets retry incomplete construction independently, so a delayed compass model does not block creation of the other models.
- **The custom-map HUD failure remains open.** Read-only inspection of the installed build found a live stock HUD and constructed custom widget models, while the custom surface's visible set was empty. This observation narrows the investigation; it is not a confirmed root cause or a completed fix.

### Hippy jumps and recovery

- Hippy takeoff is filtered from accidental board-loss recovery, including descent and a short landing handoff. A real bail still takes priority.
- Existing two-second bail and one-second accidental board-loss grace periods, checkpoint recovery speed protection and cues remain included.

### Stock waiting-banner lifecycle

- Native Skate Jam, Spot Battle and S.K.A.T.E. waiting presentations track their local queue lifetime. Activity entry, Leave, Quit, end and map teardown retire the waiting display; a fresh queue receives a fresh lifetime.
- Cleanup targets presentation owned by the retired queue rather than removing unrelated backend activity. **The reported stuck Skate Jam banner still needs an in-game confirmation on this build.**

## Retained features

The custom 2.1.0 integration retains upstream shader/renderer safeguards and music features; the styled challenge titles and sideboards; camera-aware gate compass; 125% editor defaults and 90-degree snapping; X rotation rings, yaw/tilt controls and smooth motion; Undo/Redo; camera-follow orientation; Chevron wall, floor and air placement with three-axis snapping and clearance; saved routes; checkpoint rewind; finish grace and quiet cues. Earlier confirmations of these features are historical and do not certify every combination in this follow-up.

## Assets

The complete Assets 1.0.6 pack is included and separately downloadable with refreshed playtest installation notes. All 17 native payloads and both external WAV files are byte-identical to complete 1.0.6. Existing complete installations need the Runtime Update only. There is no invented asset version bump for documentation-only changes. The unchanged authoring project and native_assets.bin build archive are supplied separately.

## Validation and known limits

- All 90 configured optimized Release checks passed, with no failed or skipped cases. Coverage includes enrollment without Ready, host-start roster handling, Leave/re-enroll, position holds, hippy versus bail handling, HUD root selection, queue notice lifetimes and identity-owner mismatch checks.
- The source patch reconstructs the exact committed source tree from the pinned official 2.1.0 base. Runtime hashes match the installed build; package manifests, archive contents and SHA256 checks are verified.
- Backup-installer fixture checks passed for backups, settings/unrelated-mod preservation, running-program protection, missing assets, rollback after an injected copy failure and Mods/ModData routing.
- **Reported unresolved:** disappearing challenge HUD on custom maps; incorrect Spot Battle branding on some Deathrace interaction/map cards.
- **Not yet verified on this exact build:** complete multiplayer enroll/host-start flow, native waiting-banner cleanup, hidden-stock-HUD combinations, hippy versus bail behavior, repeated map-travel HUD survival and longer sessions.
- Earlier repeated Improved-map load success was confirmed on the previous 2.1.0 runtime, not on this follow-up. It is not presented as validation of the changed build.

No game executable, personal settings, private captures, debugger/recorder tools, runtime logs or debug symbols are shipped. Production logging, tracing, profiling and crash uploads remain disabled. Publishing does not launch, stop, control or update either program on the owner's PC.

Maintained by SmokeyDawg58 with substantial OpenAI Codex AI assistance. Upstream ReSkate authorship and GPL-3.0 attribution are retained. This GitHub release creates no Thunderstore listing.
