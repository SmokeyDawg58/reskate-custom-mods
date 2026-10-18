# Install and check the playtest update

This prerelease has known custom-map HUD and incorrect-card-branding issues. See DIAGNOSTICS.md before sharing it as a finished fix.

## Install

1. Close ReSkate and its launcher yourself. Extract Full to a staging folder.
2. Run ./Install-Playtest.ps1 -GameDirectory "YOUR_GAME_FOLDER", then ./Preflight-Playtest.ps1 -GameDirectory "YOUR_GAME_FOLDER". Replaced files are backed up under the game folder's PlaytestBackups.
3. Keep layout.toc, initfs_win32, Win32 and Audio together inside the complete Throwdown Assets folder. Enable one copy. The installer supports the active Mods/ModData layout.
4. Runtime Update requires complete Assets 1.0.6 already present. Every PC needs the same updated runtime pair. Launch the game yourself afterward and allow generated mod files to rebuild if prompted.

## Checks awaiting in-game confirmation

- Save a Deathrace, enroll on another PC and wait for the host. There should be no Ready step or ready count. Host Start should start all enrolled skaters.
- Leave before Start, enroll again, and repeat the host-start check with 1-UP. Draft Deathrace flags should stay private until Save; published 1-UP flags should be joinable on the other PC.
- Check the actual challenge name/logo/rules on the nearby banner, join page and map card. The known Spot Battle fallback remains under investigation.
- Start/finish/leave a normal Skate Jam and check the waiting banner retires; then create a new queue and check its banner appears normally.
- Disable stock Compass and D-Pad HUD sections and change maps twice. Check the custom HUD on each PC; the known disappearing-HUD report remains open.
- Hippy jump without recovery; then test a real bail and accidental board loss with the existing grace periods. Check rotation, three-axis Chevron snapping/clearance, saved routes, gate compass and checkpoint rewind.

Keep build name, map, role and steps with a bug report. Do not post personal settings, credentials or private captures.
