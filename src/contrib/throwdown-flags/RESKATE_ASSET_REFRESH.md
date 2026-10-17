> Current package and complete export instructions: [Assets 1.0.6](ASSET_PACK_1_0_6.md). The versioned recipe below is retained as authoring history.

# R.E.S.K.A.T.E. artwork and asset pack

The supplied transparent pink skateboard logo is `assets/throwdowns/reskate-logo-original.png`. Asset mod **1.0.2** uses it for the R.E.S.K.A.T.E. picker card, Throwdown pages, HUD, map icons, physical flag and placement preview. Other challenge artwork and the working native HUD animations are preserved.

## Change the artwork without changing native identities

`reskate_logo_swap.cpp` is the current Studio SDK author. Build it against ReSkate Studio's core, SDK and modding libraries using the CLI's include/link settings. Put the supported native converter beside the tool as `Native/dxtex.dll`.

```text
logo-swap <game-root> <current.fbproject> <logo.png> <updated.fbproject>
```

Use the full current ten-flag project from the matching release's Asset Source ZIP. The tool writes a separate FBPROJECT and FBMOD, plus decoded texture previews; it never deploys to the game. It replaces only the existing Rime logo chunk and the R.E.S.K.A.T.E. fabric chunk. Resource keys, formats, dimensions, mip chains, imports, bundles and every other payload remain unchanged. Verify that exactly these two chunk payloads differ before compiling.

Compile the output FBMOD with Studio's `compile-mod <game-root> <staging-folder> <mod.fbmod>`. Then run:

```powershell
python contrib/throwdown-flags/build_oneup_native_bundle.py <staging-folder>/Patch assets/one_up/native_assets.bin
python contrib/throwdown-flags/package_asset_mod.py --output <new-folder>/SmokeyDawg58-ReSkate_Throwdown_Assets
```

`build_oneup_native_bundle.py` writes the archive checksum. Regenerate `Engine/Vfs/native_asset_mod_manifest.h` from all 19 entries in the exported `asset-manifest.json`, bump the package/runtime asset version together, then rebuild both runtime files and run the complete Release test suite. Never change artwork without updating the pinned hashes.

The earlier `reskate_asset_refresh.cpp` documents the additive step that created the tenth custom flag and seven-letter challenge from the nine-flag project. It is retained for provenance; use the pixel-only author for a logo replacement on the current project.

## Player package

Asset mod 1.0.2 contains 17 compiled native payload files and two selected quiet WAVs. Required gates use variation 23; Finish uses variation 4 at matching quiet volume; Guides stay silent. Exact WAV bytes also reside in the lean DLL. The large texture archive remains outside the DLL.

Every player installs the matching DLL, launcher and complete mod. Replace the previous asset folder rather than enabling duplicate versions. `package_asset_mod.py` creates the Studio-supported flat mod layout and Thunderstore-compatible metadata. No Thunderstore account publication is implied.
