# Install and playtest

## Choose a download

| What you need | Download |
| --- | --- |
| **New installation — start here** | [Full playtest package](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.0.7-custom-20261015-multiplayer-flags/ReSkate-2.0.7-Multiplayer-Playtest-Full-20261015.zip) — DLL, launcher, required assets and installer |
| Update an existing installation with the asset pack | [Small update](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.0.7-custom-20261015-multiplayer-flags/ReSkate-2.0.7-Multiplayer-Runtime-Update-20261015.zip) — replace both included files |
| Logos, flags, gates and HUD artwork | [Required asset mod](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/download/v2.0.7-custom-20261015-multiplayer-flags/SmokeyDawg58-ReSkate_Throwdown_Assets-1.0.0.zip) — also included in the full package |

Use only the [current release](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/tag/v2.0.7-custom-20261015-multiplayer-flags). Older releases are archived for reference.

## Install the full package

1. Start with supported ReSkate 2.0.7. Close the game and launcher yourself.
2. Extract the full package to its own folder.
3. Run the included backup installer, replacing the example game path with yours:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Install-Playtest.ps1 -GameDirectory "E:\Games\ReSkate"
```

4. Run the file checker from that same folder:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Preflight-Playtest.ps1 -GameDirectory "E:\Games\ReSkate"
```

Every player must get PASS. The installer backs up files it replaces and preserves other mods/settings. It never starts or closes programs. Keep launcher automatic updates off while testing this pinned build and offline mode off for multiplayer.

## Update an existing installation

If you already have the complete version1.0.0 asset pack, the small update is sufficient. Close both programs yourself, back up the existing DLL and launcher, then replace both files beside `Skate.exe`. Keep your Mods folder and settings. Use the same update on every PC.

## Asset folder

The mod supplies logos, physical flags, gates and menu/HUD artwork. Keep `layout.toc`, `Win32` and `initfs_win32` directly inside `Mods/SmokeyDawg58-ReSkate_Throwdown_Assets`. ModData installations use `ModData/Default/Mods`. A numeric version suffix added by extraction is supported; an extra nested folder is not.

The asset mod alone does not add custom modes to the stock runtime. Both the matching runtime and complete assets are required. Custom maps must also match between players; unrelated map mods are not included.

## Check a session with friends

Start with two PCs/accounts, the same stock map and a short Deathrace route. Check guest flag Join, start facing, checkpoints, finish order/time and native Start → Back → Quit. Let one racer finish or DNF while another continues, then test guest Quit. Repeat in a second match before expanding to more players, custom maps or reconnect tests.

The owner confirmed guest flag Join, controls after finish/DNF, and guest-only Quit. The guest join wording and Ready display are still being improved; the current release may say Spot Battle or Start in those places. Only the host starts the race.

This build records no diagnostic log. When reporting a problem, include both build versions, map, host/guest roles and exact steps, plus a screenshot or short recording if useful. See DIAGNOSTICS.md for what has and has not been verified.
