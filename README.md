# ReSkate custom mods

Custom mod source for collaborating on ReSkate 2.0.6. This snapshot is the working build verified before the next spawn/HUD changes. Updates are pushed only when the owner asks.

The repository contains the modifications, their integration patch, tests, native asset authoring helpers and build notes. The original project is [Dingo-Shenanigans/ReSkate](https://github.com/Dingo-Shenanigans/ReSkate), under GPL-3.0. Modified upstream source keeps its existing license; see LICENSE. ReSkateStudio itself is not included.

## Included

- Custom 1-UP referee, scoring, penalties, multiplayer messages, countdown, lobby, Start/Leave ownership and native HUD.
- Deathrace route editor, smooth native gates, checkpoint bonuses, ordered finish checks and gate cleanup.
- The custom Throwdown registry, twelve-logo picker and custom modes, including their rules and integration.
- Native flag placement/preview, challenge colors, map details, distant bubbles, and branding lifetime fixes.
- Related trainer, camera and runtime integration changes, plus regression tests.

`src/` contains readable changed files. `mods.patch` applies the complete integration, including deleted files, to the pinned official source. `SNAPSHOT.json` lists file hashes and the exact base. No game executable, compiled game resources, credentials, saved games or private captures are published.

## Build or integrate

1. Clone the official ReSkate source and check out `314d31fe7bf73866b4ae9b6a8195de50ced26312` (v2.0.6). Start from a clean checkout.
2. Apply this repository's `mods.patch` with `git apply --check` first, then `git apply`.
3. Obtain the matching `assets/one_up/native_assets.bin` from the owner's existing handoff or locally extract it from the matching custom DLL using `tools/extract_native_assets.py`. Its required SHA-256 is in SNAPSHOT.json. This archive contains compiled game dependencies and stays out of the public source repository.
4. Follow the patched upstream CMake build instructions with Visual Studio C++ and CMake. Build Release and run `ctest --test-dir <build-directory> -C Release --output-on-failure`.

To collaborate on an existing fork, review `mods.patch` and selectively integrate the relevant `src/Extension/Throwdowns`, `src/Extension/Modes`, `src/Extension/UI/NativeMenu`, `src/Engine` and CMake changes. Do not apply a full patch blindly to a different upstream revision.

The current matching build passed all 77 tests. The owner verified repeated Deathrace/1-UP Start and Leave, menu unlocking, flags, map details, placement branding, distant bubbles and pre-start gate cleanup. Native Throwdown Panel/celebration research is deferred. Spawn orientation and 1-UP gameplay HUD persistence/style are the next work and are not claimed fixed in this snapshot.

## Code map

| Area | Source |
| --- | --- |
| 1-UP rules/coordinator/wire | src/Extension/Throwdowns/one_up_* |
| Native queue/placement lifecycle | src/Extension/Throwdowns/native_throwdowns.* and one_up_placement.* |
| Mode registry | src/Extension/Throwdowns/challenge_registry.* |
| Deathrace and custom mode rules | src/Extension/Modes/ |
| Picker, setup, lobby, gameplay HUD | src/Extension/UI/NativeMenu/one_up_menu.cpp and throwdown_modes_ui.inl |
| Map/bubble/flag identity | src/Extension/UI/NativeMenu/throwdown_map_identity.inl and src/Extension/Throwdowns/native_flag_* |
| Flag authoring helpers | src/contrib/throwdown-flags/ |

Keep changes in branches and review differences before merging. Publishing this baseline does not authorize automatic future pushes.
