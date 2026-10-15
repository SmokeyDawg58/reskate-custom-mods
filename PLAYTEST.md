# Lean multiplayer playtest

Everyone needs the same **complete client ZIP**, including its Mods folder. The DLL alone is insufficient. The asset mod supplies the custom logos, flags, gates, menus and HUD artwork; assets are no longer embedded in the lean DLL.

## Install

1. Start with a normal supported ReSkate 2.0.7 installation. Extract the complete client ZIP to its own folder.
2. Fully close ReSkate and its launcher yourself.
3. Run `powershell -NoProfile -ExecutionPolicy Bypass -File .\Install-Playtest.ps1 -GameDirectory "E:\Games\ReSkate"`, replacing the game path with yours. The installer backs up files it replaces, adds the required mod and preserves other mods/settings. ModData installations use ModData/Default/Mods automatically.
4. Keep launcher automatic updates OFF and offline mode OFF for this pinned build.
5. Run `powershell -NoProfile -ExecutionPolicy Bypass -File .\Preflight-Playtest.ps1 -GameDirectory "E:\Games\ReSkate"`. Every player must get PASS.
6. Launch ReSkate yourself. First check the custom Throwdown grid, gate editor and native Start panel for artwork and steady HUD visibility.

Manual folder layout: ReSkate.dll and ReSkateLauncher.exe beside Skate.exe; `Mods/SmokeyDawg58-ReSkate_Throwdown_Assets/layout.toc`, `Win32`, `initfs_win32` and metadata inside the active Mods directory. Keep the mod folder name unchanged. No separate native_assets.bin installation is needed. The standalone Thunderstore-format asset ZIP can be installed through the launcher's mod manager, but the matching custom runtime is still required.

## First session

| Step | Action and pass condition |
| --- | --- |
| Connect | Two separate PCs/Steam accounts; host a session and join via Steam. Both see the same stock map and each other. |
| Build | Place a short route with 3–5 Required gates, one Guide and one Finish. Guest sees the same route and joins. |
| Race | Check start facing, GO clearing, checkpoints, optional Guide and matching final order/time. |
| Menus | On both PCs, use Start → Back → Start → Quit. Native camera/movement behavior works and normal menus recover. |
| Repeat | Start a second race immediately. Let one player DNF; verify readable results and no soft lock/HUD loss. |
| Expand | Add a third player; verify gold/silver/bronze, remote names/photos and leaderboard times. |
| Stress | Try leader/guest departures, reconnect, two matching custom maps with saved routes, then a larger course and poorer connection. |

Per-frame clocks can differ slightly with transit; final host-owned finish times/order should agree. Start small: very large route transfers still share a bounded message budget. The server's 16-client simulation is not a promise of 16-player custom challenge UI support.

This build deliberately records no diagnostic log. If something fails, gather a short recording/screenshots, both preflight reports, map/mod versions, host/guest roles and exact reproduction steps. Do not expect the old ReSkate.log to describe this lean session.
