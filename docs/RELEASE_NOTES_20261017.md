# ReSkate 2.1.0 custom — map-change fix and Throwdowns

## Downloads

**[Full package — recommended](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261017/ReSkate-2.1.0-Custom-Full.zip)** includes the matching DLL, launcher, complete Assets 1.0.6 mod folder, backup installer and file checker.

- [Runtime Update](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261017/ReSkate-2.1.0-Custom-Runtime-Update.zip): for installations that already have complete Assets 1.0.6.
- [Required Assets 1.0.6 mod folder](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261017/SmokeyDawg58-ReSkate_Throwdown_Assets-1.0.6-For-2.1.0.zip): logos, flags, gates, Chevrons, compass and HUD artwork. Also included in Full.
- [DLL](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261017/ReSkate.dll) · [Launcher](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261017/ReSkateLauncher.exe) · [Source overlay](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261017/ReSkate-2.1.0-Custom-Source-Overlay.zip) · [Asset authoring project/source](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.0-custom-20261017/ReSkate-Throwdown-Asset-Source-1.0.6.zip).

Every PC must use this matching custom 2.1.0 runtime and complete assets. Older custom 2.0.7 clients use a different session protocol. Existing complete Assets 1.0.6 remain compatible: native/audio bytes have not changed.

## Tested result

**90 automated Release checks passed.** The owner confirmed repeated Skate3Improved ↔ Skate2Improved map changes on the exact published runtime: **“Every load works.”** Wider multi-PC, disconnect/rejoin, grace-period finish and long-session checks remain pending.

The DLL is **30.3 MB** (30,261,248 bytes); the launcher is **5.3 MB**. The upstream music/Spotify and other 2.1.0 features are retained. The large native archive stays outside the DLL. Logging, profiling, tracing, crash uploads and shipped debug symbols remain disabled.

## Detailed changes

## Source merge

The supplied DLL and launcher match the official release SHA256 digests. The custom source was preserved separately and merged into official tag v2.1.0 (ba1245a5af51892e90f2be8637dd69a7e20f5b77). Fifteen conflicts were reviewed and resolved. This is a fresh source build with the custom features and upstream changes combined.

## Map-change failure

The old candidate loaded Skate3Improved but crashed on another map change. A fresh recording caught a fatal read at Skate.exe+0x3f7ceed. Upstream 2.1.0 includes the exact shader-registry fix: invalid lookup indices are repaired and registrations of destroyed databases are excluded before stale entries enter the registry. This guard and the new renderer-resource, shader-permutation and mesh-settings fixes are retained unchanged and enabled in the lean build.

The earlier Chevron dependency correction also remains: copying its mesh into a replacement map carries the referenced 848-byte geometry and correct chunk/texture metadata. Both actual Improved map inputs passed the isolated merge audit.

## Custom behavior retained

The Deathrace editor retains gates and stationary stunt Chevrons, floating/surface placement, upright/tilted ground clearance and aggressive horizontal/vertical/depth row snapping. X rotation rings, smooth yaw/tilt, camera-follow preview, Undo/Redo, Size and 125%/90-degree defaults remain included.

HUD work retains the approved title/logo alignment, styled boxes, edge spacing, right-side boards, Required progress, persistent shortcuts and fixed-size camera-relative next-gate compass. The steady visibility/presenter lifecycle remains. Seven-letter R.E.S.K.A.T.E. branding and the provided artwork remain across challenge pages and flags.

Recovery retains last-cleared Required gates, progressive D-pad Up rewind, updating recovery after clearing a gate again, preserved movement without repeated speed gain, accidental board loss and deliberate dismount controls. Full bails retain two seconds of grace and accidental rolling board loss one second. Quiet cues, silent Guide gates and countdown audio paths remain included. Guest readiness, host-only Start, guest-local Quit and control restoration remain.

## Compatibility work

The new full-screen controller filter runs before custom challenge-menu button suppression. New drone follow hands control back through the preserved camera recovery path; drone follow cannot authorize editor placement rays. The camera regression covers that separation. Grouped score parsing, map download cancellation and UTF-8 paths are retained.

All new music services start outside the disabled-diagnostics conditional. Spotify Connect uses the official locked Rust inputs and static Windows C runtime. Its output is pinned to the source workspace for reproducible linking. Interactive audio inspection helpers are excluded from the lean default build; they are not automated checks or player payloads, and runtime audio features remain included.

The retired reporting API is replaced by disabled implementations of the 2.1.0 API. Environment switches cannot enable logging, profiling, tracing or uploads. No debug symbols ship. Asset data remains complete 1.0.6; large native content remains external to the DLL. Use matching protocol 52 clients, custom modes 16 and 1-UP 5.

## Validation

90 configured checks passed, covering custom rules/wire formats, camera recovery, HUD/model ownership, input, mod merging and disabled diagnostics. The actual map fixture retained inspected content and passed exact geometry checks, 1,162 chunk-bearing bundles and 646 texture records. All three shader guard instruction contracts match the supported game.

The owner confirmed repeated Skate3Improved → Skate2Improved → Skate3Improved transitions on this exact installed build: “Every load works.” The skater appeared and moved after each load. This is a human gameplay confirmation, not an independently recorded native trace. Wider multi-PC, disconnect/rejoin and long-session tests remain pending. The native join-label wording and R.E.S.K.A.T.E. rules playtest remain deferred. Original map inputs had six lighting declared-size warnings; this is not a zero-warning full payload certification.

## Credits

Maintained by SmokeyDawg58 with substantial OpenAI Codex AI assistance. Upstream ReSkate authorship and GPL-3.0 are retained. This GitHub release does not create a Thunderstore listing.

Pinned upstream: `ba1245a5af51892e90f2be8637dd69a7e20f5b77`. Exact source reconstruction and download checksums are supplied.
