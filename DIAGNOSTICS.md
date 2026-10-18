# Diagnostics and validation

## Passed on this release

- All 90 configured optimized Release checks; no failed or skipped cases.
- Exact source reconstruction from official 2.1.0, matching runtime/asset hashes, archive integrity and disabled diagnostics.
- Both actual Improved maps in an isolated merge fixture: 1,162 chunk-bearing bundles and 646 texture records; exact Chevron geometry retained; three native shader-guard fingerprints match.
- Backup-installer fixtures: preservation, running-program protection, missing-assets rejection, rollback on injected copy failure and Mods/ModData routing.
- Human gameplay confirmation on the exact published runtime: repeated Skate3Improved → Skate2Improved → Skate3Improved loops; skater visible and moving after each load. The owner reported “Every load works.” See user-validation.json. No independent native trace is claimed for that run.

## Remaining checks

Wider multi-PC host/guest, disconnect/rejoin, longer sessions, finish crossings during bail/board-loss grace periods, and complete audio coverage remain playtest work. Historical narrow confirmations of custom features were made on earlier builds and do not certify every 2.1.0 combination. One native join confirmation may still say Spot Battle; live wording inspection and the dedicated R.E.S.K.A.T.E. rules playtest were deferred.

Original map inputs contain six lighting declared-size warnings. The focused geometry/metadata check does not certify every third-party map payload.

Runtime logging, profiling, tracing, crash uploads and shipped debug symbols are disabled. No private captures, local paths, personal settings or game binaries are included in the release.
