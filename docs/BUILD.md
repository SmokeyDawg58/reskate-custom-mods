# Reconstruct the archived custom 2.1.1 build

1. Check out official ReSkate `db99ed4d724dd77edd2abf62f3de0dbc5bb6717e`.
2. Run `git apply --check mods.patch`, then `git apply mods.patch`. This includes deleted files and binary source art; copying src alone is insufficient. Expected Git tree: `93a0c59e355ed878f08c30a8124399b99096e50e`.
3. Extract Assets 1.0.27 into asset-kit/Mods/SmokeyDawg58-ReSkate_Throwdown_Assets. Configure `lean-playtest-x64`, Release, `DINGOSDK_LEAN_LOGGING=OFF`, `DINGOSDK_VERSION=2.1.1`, `DINGOSDK_PLAYTEST_PACKAGE_ROOT=<asset-kit>`.
4. Use upstream’s pinned Visual Studio/CMake/Rust dependencies. Build Release and run CTest: 93 checks. The lean build needs the matching external asset pack; the optional large authoring archive is excluded from Git.

Canonical changed blobs are in src; raw compiled inputs are pinned in BUILD_SOURCES.json. Source authoring tools and private shader deltas are under src/contrib/throwdown-flags. Verification uses a separate temporary index/object store; the integration index/working files are preserved. This is not the separate stock 2.1.2 plugin package.
