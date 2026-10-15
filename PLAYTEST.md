# Multiplayer playtest

Use the client ZIP attached to this release on every test PC. The DLL already contains its native assets. Keep the source-only native_assets.bin download for rebuilding, not installation.

## Before launching

1. Prepare a normal ReSkate 2.0.7 installation using the supported game build. Extract the playtest ZIP to its own folder.
2. Fully close ReSkate and its launcher yourself. Backups are automatic when using Install-Playtest.ps1. Alternatively, back up the existing DLL/launcher and copy only the two matching files beside Skate.exe.
3. Run: powershell -NoProfile -ExecutionPolicy Bypass -File .\Install-Playtest.ps1 -GameDirectory "E:\Games\ReSkate"
4. In the launcher, keep automatic updates OFF and offline mode OFF for this pinned test. The installer preserves your mods/settings and never starts or stops programs.
5. Run: powershell -NoProfile -ExecutionPolicy Bypass -File .\Preflight-Playtest.ps1 -GameDirectory "E:\Games\ReSkate"
6. Every participant should get PASS. Compare the release name, DLL hash and chosen custom-map package/version before testing. Changing scoring/physics mods can exclude a player from challenges; the owner's current startup eligibility check passed.

Substitute each person's actual game directory. Keep the extracted scripts beside release-manifest.json and the two binaries. No game executable, third-party cosmetics/maps, saved settings or credentials are distributed.

## First session: two players, stock map

| Step | Action | Pass condition |
| --- | --- | --- |
| Connect | One player hosts in-game; the other joins by Steam invite/code | Both see each other and the same map |
| Set up | Host places a short route: start, 3-5 Required gates, a Guide and a Finish | Guest sees the same route and joins the queue |
| Race 1 | Start together; cross gates in order; deliberately skip the Guide | Correct spawn facing, Guide optional, Required/Finish count and no stuck GO |
| HUD | Open/close Start and Back once while racing on each PC | Native camera/movement transition, steady HUD and responsive menus |
| Finish | Finish at different times | Everyone sees matching final order and times; names/photos/medals are correct |
| Race 2 | Immediately start another race; one player DNFs | No soft lock; long DNF time fits; countdown moves and Time to Beat stays fixed |
| Quit | Guest quits, then host quits via the native panel | Guest leaves cleanly; challenge-leader Quit ends the challenge; normal Start/Map menus recover on each PC |

Treat the results as a shared verdict. Per-frame countdown digits may differ slightly with network transit; final host-owned finish times/order should agree.

## Expand after the first session passes

- Add a third player for gold/silver/bronze and a long name.
- Swap the challenge leader; reconnect a guest; leave during countdown and during the race.
- Check 1-UP turn/score/strikes and native Quit on both PCs, then Spot Battle/Skate Jam compatibility.
- Switch between two custom maps, using identical package versions on every PC. Reopen each saved route and run two matches.
- Test a larger route and late joining. Do not begin with thousands of gates: uncapped editing still uses bounded network transfers and a shared message budget.
- Repeat a short race on a poorer connection. Record ping, visible stalls and any disagreement. Only a live session validates Steam relay behavior.

## If something fails

Leave the affected screen as-is if practical. Note the release, host/guest role, map, challenge, player count and exact steps. Record a short clip/screenshot.

Run: powershell -NoProfile -ExecutionPolicy Bypass -File .\Collect-PlaytestLogs.ps1 -GameDirectory "E:\Games\ReSkate"

The collector copies the latest local log and build/mod inventory into a new folder. It does not upload anything, copy login/settings files, collect crash memory, or control the game. Review logs for player names/Steam IDs before sharing. Gather reports from host and affected guest, using the same approximate timestamp.

## Scope

All 78 automated checks and simulated server runs passed. The local HUD/menu flow is owner-confirmed. This is a playtest candidate; live host/guest native gates, timing, maps and teardown are the next acceptance gate. Do not call it fully online-validated until those steps pass.
