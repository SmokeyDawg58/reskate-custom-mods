# Asset pack 1.0.2

Players: download the [required mod folder](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.0.7-custom-20261015-reskate-refresh/SmokeyDawg58-ReSkate_Throwdown_Assets-1.0.2.zip), or use the full package. Every player needs the matching runtime and complete assets.

Developers: [download the current authoring source](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.0.7-custom-20261015-reskate-refresh/ReSkate-Throwdown-Asset-Source-1.0.2.zip). It contains the 1,635-resource Studio project, compiled FBMOD, exact supplied pink PNG, selected quiet WAVs and the authoring/export source. No SDK binaries or game files are included.

Source tools live in [src/contrib/throwdown-flags](../src/contrib/throwdown-flags). Start with [RESKATE_ASSET_REFRESH.md](../src/contrib/throwdown-flags/RESKATE_ASSET_REFRESH.md). `reskate_logo_swap.cpp` updates only existing artwork payloads; `reskate_asset_refresh.cpp` preserves the earlier additive creation step. `package_asset_mod.py` exports the complete required mod, and `build_oneup_native_bundle.py` builds its source archive.

The exact supplied PNG is [reskate-logo-original.png](../src/assets/throwdowns/reskate-logo-original.png). Quiet audio inputs are in [src/assets/throwdowns/audio](../src/assets/throwdowns/audio). The large compiled `native_assets.bin` stays on the matching release; it is not an embedded DLL resource.

Rebuild both runtime files after changing pinned asset hashes. Do not mix asset versions. The folder format is Thunderstore-compatible; publishing here does not publish a Thunderstore listing.
