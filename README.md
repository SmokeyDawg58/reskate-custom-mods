# ReSkate custom Throwdowns — multiplayer playtest

## Downloads

Use the [latest tested multiplayer release](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/tag/v2.0.7-custom-20261015-multiplayer-flags). **Every player needs the matching DLL and launcher plus the required asset mod.**

| Download | Use |
| --- | --- |
| [Full playtest package](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.0.7-custom-20261015-multiplayer-flags/ReSkate-2.0.7-Multiplayer-Playtest-Full-20261015.zip) | Recommended for new players: matching DLL, launcher, complete required asset mod, backup installer and preflight checker. |
| [Small runtime update](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.0.7-custom-20261015-multiplayer-flags/ReSkate-2.0.7-Multiplayer-Runtime-Update-20261015.zip) | Existing players with the complete asset mod: replace both DLL and launcher together. |
| [Required asset mod](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.0.7-custom-20261015-multiplayer-flags/SmokeyDawg58-ReSkate_Throwdown_Assets-1.0.0.zip) | Logos, physical flags, gates, menus and HUD artwork. Extract as a mod folder; matching runtime also required. |
| [DLL](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.0.7-custom-20261015-multiplayer-flags/ReSkate.dll) / [Launcher](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.0.7-custom-20261015-multiplayer-flags/ReSkateLauncher.exe) | Individual matching runtime files. Both are already in the packages above. |

The DLL is about14MB; assets are supplied in the visible Mods folder. Existing version1.0.0 native asset files are unchanged. The runtime accepts the plain mod name or a numeric version suffix added by extraction. Keep layout.toc, Win32 and initfs_win32 directly inside the mod folder. See [PLAYTEST.md](PLAYTEST.md) for installation.

## Tested behavior

79 automated checks pass. The owner confirmed on two PCs that the guest sees and joins through the native Deathrace flag, controls return after finish/DNF, and guest Quit leaves the host race running. The working fast, steady HUD and native Start/Back/Quit flow are retained. Logging/profiling/crash capture and shipped debug symbols are disabled. See [DIAGNOSTICS.md](DIAGNOSTICS.md) for scope.

Known follow-up: The guest flag currently inherits some Spot Battle wording and its waiting action can say Start. Only the host can start the custom race. Correct Deathrace wording and a guest Ready counter are being worked on separately; they are not included in this tested snapshot.

## Build from source

Start from [official ReSkate](https://github.com/Dingo-Shenanigans/ReSkate) commit `0f8af627105c1079ef25f36ad83b159ae9d1df1f` and apply mods.patch after git apply --check succeeds. src/ contains the modified overlay. SNAPSHOT.json pins each source file and the original native asset archive. Download native_assets.bin from the same release into assets/one_up/native_assets.bin, verify its checksum, then export the required mod with contrib/throwdown-flags/package_asset_mod.py. Configure lean-playtest-x64, build lean-playtest-release and run the full Release CTest suite. The normal development preset retains optional diagnostics and embedded assets.

The standalone asset ZIP follows the Studio-supported mod layout and Thunderstore package format. No Thunderstore account listing is claimed. No game executable, Steam DLL, unrelated mods or personal settings are distributed. ReSkate and its modified source use GPL-3.0; see LICENSE.
