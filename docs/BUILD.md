# Build the custom ReSkate 2.1.1 update

1. Check out official ReSkate `db99ed4d724dd77edd2abf62f3de0dbc5bb6717e` (2.1.1).
2. Apply mods.patch with git apply --check and then git apply. It includes deletions and binary source assets; copying src alone is insufficient. Reconstructed tree: `56bfd9a5a3ec65834b8bcd074d1e7c405ace0d4e`.
3. Extract [Assets 1.0.15](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.1.1-custom-20261019-deathrace-routes/SmokeyDawg58-ReSkate_Throwdown_Assets-1.0.15.zip) into an asset-kit/Mods/SmokeyDawg58-ReSkate_Throwdown_Assets folder. The lean runtime uses the external pack; optional embedded native_assets.bin is not needed for that configuration. Its checksum pins the authoring archive. Authoring/export and lossless compactor sources are under src/contrib/throwdown-flags.
4. Use upstream Visual Studio 2022 C++ tools, Windows SDK, CMake, Python and locked Rust dependencies. Configure lean-playtest-x64 in Release with DINGOSDK_LEAN_LOGGING=OFF, DINGOSDK_VERSION=2.1.1, and DINGOSDK_PLAYTEST_PACKAGE_ROOT pointing to the asset-kit root above Mods.
5. Build and run the complete CTest suite. All 92 checks passed for this source and matching pack; native-package checks verify 19 exact file hashes.

src/ stores exact canonical Git blobs (LF text where applicable); BUILD_SOURCES.json pins raw compiled inputs before Git line-ending normalization. SNAPSHOT.json records the pinned base, exact source tree and release hashes. The patch is independently reapplied before publication. Production logging/profiling/tracing and shipped debug symbols remain off. Authorship and license are retained.
