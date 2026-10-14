# ReSkate custom mods

Custom Throwdowns and related ReSkate 2.0.7 mods for collaboration. Download the matching runtime and native assets from the [latest release](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/latest).

## Downloads

| Release file | Use |
| --- | --- |
| `ReSkate.dll` | Complete custom runtime, including the matching embedded native assets |
| `ReSkateLauncher.exe` | Matching launcher |
| `native_assets.bin` | Native asset archive for building or integrating this source |
| `native_assets.sha256` / `SHA256SUMS.txt` | Verify the downloaded archive and binaries |
| `release-manifest.json` | Exact build, hashes, changes and checks still pending |

To play: close ReSkate yourself, back up your existing DLL and launcher, and copy both downloaded files beside `Skate.exe`. Run the launcher when ready. The DLL already contains the archive; you do not need to install `native_assets.bin` separately to play. Keep all collaborators on the same release. The game executable and ReSkateStudio are not included.

## Included mods

- Custom Throwdown grid and mode registry, including native placement flags, challenge colors, map details and distant logos.
- 1-UP referee, scoring, penalties, multiplayer messages, lobby Start/Leave ownership, corrected spawn facing and native HUD with wider name panels.
- Deathrace with unlimited route gates, one finish, ordered Required gates, optional Guide gates, editable finish deadline, final-30-second countdown and DNF.
- Saved Deathrace courses per map in memory for the current game session. Reopening always enters the gate editor after placing the start flag; edit, hold Back/View to Clear All, or save unchanged. Editor exit asks for confirmation.
- Native Deathrace logo, dark translucent panels with thin red trim in the editor/race/results panels and real controller glyphs.
- Five-percent sizing per press, complete rotation steps, adjustable position/angle snapping, scaled crossing triggers and missed-gate recovery with a bounded rolling start.
- Competition stock physics and trainer restrictions, plus related camera/runtime integration and regression tests.

This is the current installed HUD recovery build, merged with official v2.0.7. All **77 automated tests passed**. The integration patch reconstructs the pinned upstream source, and the DLL embeds the exact downloadable native archive. The owner confirmed custom logos persist across maps, gate placement/cancel cleanup works, and the recovered editor HUD and Size controls look correct.

**Known issues:** Start still opens the normal pause menu rather than reliably opening the Throwdown panel; repeated Quit is unverified. The owner reported a crash switching back and forth between maps with saved Deathrace routes. Results-title overlap, long DNF-time clipping, timer smoothness and initial HUD flashing remain follow-up work. The new medal-colored position display and titled dynamic leaderboard are also pending. This release preserves the installed snapshot; these issues are not claimed fixed. See `release-manifest.json` for exact verification status.

## Build or integrate

1. Clone [Dingo-Shenanigans/ReSkate](https://github.com/Dingo-Shenanigans/ReSkate), check out `0f8af627105c1079ef25f36ad83b159ae9d1df1f` and start from a clean checkout.
2. From that checkout, run `git apply --check <path-to-mods.patch>` and then `git apply <path-to-mods.patch>`.
3. Download **`native_assets.bin` from the same release as this source** and place it at `assets/one_up/native_assets.bin`. Verify its SHA-256 against `SNAPSHOT.json` or the release checksums. Alternatively, extract it from the matching custom DLL with `python tools/extract_native_assets.py <ReSkate.dll> <output.bin>` (requires `pefile`; the DLL is read as data).
4. Install Visual Studio 2022's Desktop development with C++ workload and CMake 3.24 or newer. From Developer PowerShell, configure with `cmake --preset vs2022-x64`, then build with `cmake --build --preset release --parallel 4`.
5. Run `ctest --test-dir build/vs2022-x64 -C Release --output-on-failure`. Release binaries are in `build/vs2022-x64/Release/`.

`src/` contains the modified source files. `mods.patch` is the complete mod integration against the pinned upstream revision, including deletions; it has been applied to a disposable index and checked against the published source. `SNAPSHOT.json` records every source hash and the exact asset version. For an existing fork, review and integrate the relevant changes instead of applying the patch blindly to another revision.

| Area | Source |
| --- | --- |
| 1-UP rules, coordinator and messages | `src/Extension/Throwdowns/one_up_*` |
| Native flag, placement and queue lifetime | `src/Extension/Throwdowns/` |
| Custom mode registry | `src/Extension/Throwdowns/challenge_registry.*` |
| Deathrace, course memory and gate rules | `src/Extension/Modes/` |
| Native grid, setup, lobby and HUD | `src/Extension/UI/NativeMenu/` |
| Mod-specific native asset authoring helpers | `src/contrib/throwdown-flags/` |

ReSkate and its modified source use GPL-3.0; see `LICENSE`. Keep the patch, source, archive and binaries together when redistributing this custom build.

## Publishing updates

Updates are pushed only when the owner explicitly asks. Every authorized push must include the current source and a GitHub release with the matching `ReSkate.dll`, `ReSkateLauncher.exe`, `native_assets.bin`, archive checksum, `SHA256SUMS.txt` and release manifest. Large compiled files belong in release downloads, not Git history. Use secure stored GitHub credentials; never put a token into source, patches, logs or release files. Do not stop or restart the owner's game as part of publishing.
