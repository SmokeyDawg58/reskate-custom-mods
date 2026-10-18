# Install and playtest

## Install

1. Close the game and launcher yourself. Extract Full to a staging folder.
2. In PowerShell, run `./Install-Playtest.ps1 -GameDirectory "YOUR_GAME_FOLDER"`, then `./Preflight-Playtest.ps1 -GameDirectory "YOUR_GAME_FOLDER"` from that extracted folder. Backups are stored under the game folder's PlaytestBackups.
3. Keep the complete asset mod together and enable one copy. The installer handles the active Mods/ModData layout. Do not move only the DLL or only a nested Win32 folder.
4. If complete Assets 1.0.6 are already installed, Runtime Update supplies just the matching runtime pair and checks the existing assets. Keep your settings and other mods.
5. Launch yourself and allow generated mod files to rebuild if prompted. All players need this custom 2.1.0 runtime, session protocol 52 and complete Assets 1.0.6.

## Confirmed

The owner repeated Skate3Improved ↔ Skate2Improved loads and confirmed the skater appears and moves after every load. This exact runtime also passed all 90 automated Release checks.

## Remaining gameplay checklist

- Builder previews, gates/Chevrons, X rotation, three-axis snapping, Undo/Redo, Size and saved-route reopening after map changes.
- Steady HUD, title/logo, compass, persistent shortcuts, bail/board-loss recovery, deliberate dismount, rewind followed by new progress, speed protection and grace-period finish crossings.
- Matching PCs: flags, readiness, host-only Start, guest-local Quit, continuing host race and restored controls; then disconnect/rejoin and a longer session.
- Quiet Required/Finish/recovery cues, silent Guide gates and countdown audio; new upstream menus, drone and chosen music service alongside challenge controls.

The native Spot Battle join-label investigation and dedicated R.E.S.K.A.T.E. rules test remain deferred. Report the build, map, host/guest role and exact steps for any problem. Do not post private settings or credentials.
