# Validation and unresolved reports

This follow-up is a playtest prerelease. All 90 optimized Release checks passed; source reconstruction, package checksums and backup-installer fixtures passed. These checks do not establish correct in-game rendering.

## Reported on this exact runtime

- Challenge HUD still disappears after map travel to a custom map.
- Deathrace nearby/map cards can still show Spot Battle branding despite a correct map marker logo.

Read-only inspection found the live stock HUD and created custom models, but an empty custom surface visible set. This is an observation, not a proven cause. Private memory captures and analysis tools are excluded from this repository and downloads.

## Still awaiting gameplay confirmation

Complete multiplayer enrollment/host Start, native Skate Jam waiting-banner cleanup, HUD options off, repeated custom-map HUD survival, hippy jumps versus real bail recovery, disconnect/rejoin and longer sessions. Earlier map-load success belongs to the previous build and is not carried forward as proof for this changed runtime.

See diagnostics.json for exact runtime hashes, RELEASE_NOTES_20261018.md under docs for detailed changes, and PLAYTEST.md for the test steps. Logging, profiling, tracing, crash uploads and shipped symbols remain disabled.
