> Current package and complete export instructions: [Assets 1.0.6](ASSET_PACK_1_0_6.md). The versioned recipe below is retained as authoring history.

# Fixed stunt-style Deathrace chevron (asset pack 1.0.4)

The requested full straight arrow was not found among the native stunt challenge shapes. Per the owner request, the builder uses the blue/yellow chevron only; the unrelated MPR sign arrow is excluded.

The existing FloatingArrow_AT and FloatingArrow_A roots retain their identities, but use the native static DirectionalArrow_Square donor, with no camera-facing schematics or parameter controllers. All six working map registry payloads remain exact. Only those two private marker payloads change; their mesh, texture, chunks and isolated native material exports are included in every supported map bundle. All existing HUD, logo, gate and flag payloads remain exact.

Build deathrace_fixed_markers.cpp with current ReSkate Studio libraries (C++20). Run with game-root, the preserved 1.0.3 native-throwdown-badges.fbproject and a new output .fbproject. It reads assets only, without starting the game. Export compile-mod, build_oneup_native_bundle.py and package_asset_mod.py. Matching runtime wire14 retains kind2 for the optional silent chevron, with saved yaw/roll. All peers need the matching asset pack and runtime.
