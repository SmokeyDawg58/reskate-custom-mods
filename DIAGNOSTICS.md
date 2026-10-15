# Lean multiplayer playtest diagnostics — 15 October 2026

**Ready for a controlled playtest. Actual Steam multiplayer and first-launch rendering of this lean packaging variant remain pending.**

- 79 automated checks passed, including all existing applicable gameplay/network/native-lifetime checks and the new asset/dependency and disabled-diagnostics checks. Crash-reporting tests are not applicable because that feature is compiled out.
- All 17 native files match the original working embedded archive byte for byte. The required mod validates every file hash, mounts once, and remains a single dependency if its optional scan duplicate is disabled. Missing and same-size corrupted data are rejected.
- Logging sinks, intensive traces, model dumps, profiling, memory tracing, crash reporting, pose recording and trainer telemetry are disabled in this build. Tests request trace/console/profiling and verify no output files, sinks, recorded messages or profiling workers appear.
- No native asset archive is embedded in the DLL. No CodeView/COFF debug symbols or PDB files are shipped. The small linker optimization metadata is retained.
- The installer passed isolated workspace checks for backups, unchanged other mods/settings, the running-program guard, missing asset detection and rollback after an injected copy failure. It was not run against the owner's game.
- Fresh simulations retained 4/4 and 16/16 server clients with zero failed/skipped/dropped sends, including a two-second simulated server stall. These use simulated time and fake Steam transport, not real network players. They do not promise 16-player custom challenge UI support.

The working HUD curves, visibility latch, timings, native Start/Back/Quit flow and race rules were preserved. Native level cleanup hooks are retained even where their historical function names mention logging: they also release map/UI resources. A first manual launch must confirm the new mod-folder packaging in game; automated passes do not prove rendering or Steam relay behavior.

Use the full client ZIP on every PC, run its preflight checker, and follow PLAYTEST.md. Native game data and unrelated third-party cosmetics/maps are not distributed. The standalone asset mod follows the provided Studio mod layout and the Thunderstore package format; it has not been uploaded to a Thunderstore account.
