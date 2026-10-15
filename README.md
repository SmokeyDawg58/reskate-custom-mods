# ReSkate custom mods

Custom Throwdowns merged with official ReSkate 2.0.7. The [multiplayer playtest release](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/tag/v2.0.7-custom-20261015-multiplayer-playtest) contains the exact owner-confirmed build with a fast, steady native HUD.

## Playtest downloads

Download **ReSkate-2.0.7-Custom-Multiplayer-Playtest-20261015.zip** from that release. It includes the matching DLL/launcher, a backup installer, preflight checker, local log collector and checklist. Follow [PLAYTEST.md](PLAYTEST.md). Everyone must use the same build; keep launcher auto-updates and offline mode off during this pinned test.

The DLL already embeds its native assets. The standalone `native_assets.bin` and `native_assets.sha256` downloads are for rebuilding. `SHA256SUMS.txt` and `release-manifest.json` identify the exact payload. No game executable, third-party maps/cosmetics, personal settings or credentials are included.

## Current verification

**78 automated tests passed.** Simulated production-server runs kept all 4 and 16 clients connected with zero failed sends, including a two-second simulated server stall. Protocol tests exercised 40,000 malformed/mutated inputs. The integration patch reconstructs the pinned upstream source and is checked against every published overlay file.

The owner confirmed the steady HUD, native challenge camera/menu flow and repeated Quit recovery. Earlier manual checks confirmed saved-course map changes preserve gates, logos and HUD. This release includes the subsequent faster HUD appearance correction.

**Ready for a controlled multiplayer playtest; live Steam multiplayer validation is pending.** Simulations do not prove separate-PC gate placement, spawn facing, shared finish times, invitations or leave/reconnect behavior. Start with two players, one stock map and a short route. See [DIAGNOSTICS.md](DIAGNOSTICS.md) and [PLAYTEST.md](PLAYTEST.md) for evidence and acceptance steps. The 16-client server test is not a promise of 16-player custom challenge UI support. Very large courses still share a bounded network-message budget.

## Included mods

- Custom Throwdown grid and mode registry with persistent native logos, flags and map identities.
- 1-UP rules, scoring, penalties, messages, lobby lifecycle and native HUD.
- Deathrace editor with Required, Guide and Finish gates, sizing/rotation/snapping, checkpoint recovery and per-map course memory during the current game session.
- Native Start panel with camera/movement transition, rules/icons, player/time/gate details, side leaderboard and Back/Quit cleanup.
- Stable editor/race/confirmation/results HUD, smooth three-digit millisecond clocks, countdown, fixed Time to Beat and large medal-colored position.
- Competition scoring/physics restrictions and related camera/runtime integration.

## Build or integrate

1. Clone [Dingo-Shenanigans/ReSkate](https://github.com/Dingo-Shenanigans/ReSkate), check out `0f8af627105c1079ef25f36ad83b159ae9d1df1f` and start from a clean checkout.
2. Run `git apply --check <path-to-mods.patch>`, then `git apply <path-to-mods.patch>` from that checkout.
3. Download `native_assets.bin` from the same release and place it at `assets/one_up/native_assets.bin`. Verify its SHA-256 against `SNAPSHOT.json` or the release checksums. Alternatively, read it from the matching DLL using `python tools/extract_native_assets.py <ReSkate.dll> <output.bin>` (requires `pefile`; does not execute the DLL).
4. Install Visual Studio 2022 Desktop development with C++ and CMake 3.24 or newer. In Developer PowerShell, configure `cmake --preset vs2022-x64`, then build `cmake --build --preset release --parallel 4`.
5. Run `ctest --test-dir build/vs2022-x64 -C Release --output-on-failure`. Release binaries are in `build/vs2022-x64/Release/`.

`src/` contains the modified source overlay. `mods.patch` is the complete integration against the pinned upstream revision; it was applied to an isolated validation index and checked against the overlay. `SNAPSHOT.json` records source hashes, asset version and verification scope. Review changes before integrating into another revision.

ReSkate and its modified source use GPL-3.0; see `LICENSE`. Keep source, patch, archive and matching binaries available together when redistributing this build.

## Publishing updates

Push only when the owner explicitly asks. Every authorized publication includes current source and a matching GitHub release with DLL, launcher, native archive, checksums and release manifest. Large compiled files belong in release downloads. Never publish credentials or personal logs/settings. Publishing does not control the owner's game or launcher.
