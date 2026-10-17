# Current Throwdown asset pack: 1.0.6

Players should use the complete asset mod from the same release as the DLL and launcher. Version 1.0.6 is the current package; earlier 1.0.2–1.0.5 notes describe intermediate authoring steps.

The current Asset Source ZIP contains `native-throwdown-badges.fbproject`, its compiled `.fbmod`, the exact supplied pink R.E.S.K.A.T.E. logo, transparent gate-compass artwork, the selected quiet WAV inputs and the source tools in this folder. Open this complete project in a current ReSkate Studio. It already includes every required authoring stage; do not rebuild it by applying old intermediate recipes out of order.

The project has 2,147 resources. The player mod contains 17 compiled native files and two quiet WAVs. Those 19 payloads are pinned by `asset-manifest.json` and the matching runtime's `Engine/Vfs/native_asset_mod_manifest.h`. The recovery C cue is an additional small WAV embedded in the runtime; its source is in `assets/throwdowns/runtime-audio`.

## What this pack supplies

- Ten challenge themes, logos, pages, sized HUD/results frames, map icons and flags, including the pink R.E.S.K.A.T.E. logo.
- Start/Required/Guide/Finish route visuals, resident across the supported map registries.
- The native static blue/yellow stunt Chevron and its separate glow layer. Camera-facing billboard arrows are not used.
- Styled title frames and the transparent gate image used by the next-gate compass. Live position, size and title centering are runtime layout changes and do not require new native artwork.

## Export the complete current project

Build the asset-authoring tools against a current ReSkate Studio SDK. Follow the Studio CLI's include/link settings and keep its native texture converter at `Native/dxtex.dll` beside texture-authoring helpers. The SDK, converter and original game files are not redistributed here.

1. Open the included current FBPROJECT and export the matching FBMOD, or use the already included exact FBMOD.
2. Compile with Studio: `compile-mod <game-root> <new-staging-folder> <mod.fbmod>`.
3. From patched ReSkate source, run `python contrib/throwdown-flags/build_oneup_native_bundle.py <new-staging-folder>/Patch assets/one_up/native_assets.bin`.
4. Export to a new folder: `python contrib/throwdown-flags/package_asset_mod.py --output <new-folder>/SmokeyDawg58-ReSkate_Throwdown_Assets`.
5. If any payload changes, update the asset version and every pinned hash together, rebuild both runtime files and rerun the full Release tests before distribution. Never overwrite a previous source project or mix a new asset payload with old runtime pins.

Keep `layout.toc`, `Win32` and `initfs_win32` directly inside the named mod folder. The full playtest installer handles the game's active Mods/ModData layout. Install one matching version, rather than enabling duplicate copies. The mod-folder package uses Thunderstore-compatible metadata; a GitHub upload does not create a Thunderstore listing.

## Source history

`reskate_asset_refresh.cpp` adds the tenth flag, and `reskate_logo_swap.cpp` replaces only its existing logo/cloth texture chunks. `deathrace_builder_assets.cpp` supplies route objects and map-registry residency; `deathrace_fixed_markers.cpp` installs the static stunt Chevron. `deathrace_navigation_assets.cpp` adds title frames and an older compass prototype; the current `deathrace_compass_texture.cpp` supplies the transparent native image. The earlier fill-only compass prototype is retained for provenance and is not the active runtime widget. Shader and native flag helpers are included alongside the authors.

The matching source patch includes the exact quiet Required/Finish/recovery inputs, compass SVG/PNG, original logo and all current authoring code. Large compiled projects and `native_assets.bin` are release downloads. The lean DLL does not embed the large native archive.
