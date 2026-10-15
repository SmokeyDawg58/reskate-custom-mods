# Multiplayer readiness diagnostics — 15 October 2026

**Verdict: ready for a controlled multiplayer playtest. Live testing between separate PCs is still required.**

The candidate is the exact owner-confirmed runtime, SHA-256 e2d9626b49be3d8659c1e27a4c4ba5798c6461de65837e7a35253a0c5057e3a8. No gameplay code or installed game files changed during this audit.

| Check | Result |
| --- | --- |
| Complete automated suite | 78 passed, 0 failed |
| Protocol robustness | The protocol suite passed 40,000 malformed/mutated inputs |
| Host/client session mesh | Existing simulated host/guest tests passed, including source/world validation and scoring restrictions |
| Custom rules and messages | Mode rules/wire, 1-UP referee/coordinator, native lifetime, flag placement and challenge registry passed |
| Deathrace route transfer | Existing >1,024-gate fragmentation, reordering, duplicate/missing fragment and sender checks passed |
| Native asset contract | Exact 405,097,123-byte archive retained; native race asset contracts passed |
| Four simulated server clients | 4/4 remained connected, 0 failed/skipped/dropped sends |
| Sixteen simulated server clients | 16/16 remained connected, 0 failed/skipped/dropped sends |
| Sixteen clients with a two-second server stall | 16/16 remained connected; stale poses were shed and normal sending recovered |
| Owner's installed files | Game, Steam API, custom DLL and launcher match the supported/pinned hashes |
| Owner's launcher | Online mode; automatic updates off |
| Owner's competition eligibility | Latest game startup reports stock scoring/physics across 129 checked mods |
| Owner's native UI validation | Fast, steady HUD confirmed; native panel/camera and repeated Quit recovery previously confirmed |

The load runs use simulated time and a fake Steam transport. They exercise the production server host and codecs, but exclude Steam relay latency, game rendering and physical controllers. The 16-client run measures the server; it does not promise 16-player custom challenge UI support.

The first suite run inside the sandbox passed 72/78; six fixture/localhost tests were denied filesystem/network operations. The unchanged suite then passed 78/78 with normal access. Both original logs are retained locally.

## Required live checks

1. At least two different PCs/Steam accounts must use this exact candidate and protocol versions 50/12/5.
2. On one stock map, verify hosting/inviting/joining, two consecutive races, checkpoints, results agreement and native Start/Back/Quit for both host and guests.
3. Add a third player to check all medal positions and remote name/photo/time rows.
4. Check a guest leaving, a challenge leader leaving, a reconnect, and a second match without stuck controls or HUD loss.
5. Then switch between two identical custom-map packages with saved routes and repeat. Test a larger route afterward.

Very large routes are fragmented, but the underlying throwdown channel has a per-sender message budget. The editor's uncapped gate count is not a guarantee of unlimited network burst size. A route exceeding the first test's size needs explicit transfer/late-join verification. Stock or mismatched custom clients cannot participate in these custom challenges reliably; match the release hashes rather than only the displayed 2.0.7 version.

No live Steam multiplayer result is claimed by this report. See PLAYTEST.md for the run order and log capture instructions, and diagnostics.json for machine-readable results.
