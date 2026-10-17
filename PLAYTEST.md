# Install and playtest

Use the Full package for a new installation. Close the game and launcher yourself. Run `Install-Playtest.ps1 -GameDirectory <your-game-folder>` from the extracted package, then `Preflight-Playtest.ps1` with the same game folder. Keep the complete named mod folder, including layout.toc, Win32 and initfs_win32. Do not merge old and new asset versions or enable duplicate copies. The installer backs up replaced files and preserves settings/unrelated mods. Launch yourself afterward.

All PCs need the matching DLL, launcher and Assets 1.0.6. Custom challenge protocol is 16; session/1-UP protocols remain 50/5. The backup installer and preflight are also under tools/playtest in the repository.

1. Start a Deathrace and check the final title correction: logo to the left, visible icon and brush lettering on the same center line, group centered inside the frame, no overlap with trim or compass.
2. Build routes after changing maps twice. Place Required/Guide/Finish gates and Chevrons in air, on walls and floors. Test tilted/upright ground clearance, all three snap axes, X yaw/tilt rings, normal D-pad Undo/Redo/Size, defaults and Hold Select Clear all. Save/reopen; missing Finish should explain why saving is refused.
3. Join a friend's flag. Guests should see Ready counts and wait for host Start. Guest Quit must leave the host racing; host Quit should end the challenge. Finish/DNF/Quit must restore tools and normal menus.
4. During racing, check countdown/fixed Time to Beat, Required progress, persistent top-right prompts and side leaderboard. Turn the camera fully around: the next-gate compass image should move without growing and should advance through Required gates to Finish.
5. Test failed-trick falls and accidental board loss versus deliberate dismount. Check quiet cues and speed retention without repeated-respawn speed gain. D-pad Up should rewind through earlier Required gates; re-crossing a gate should move the recovery point forward again.
6. Test exactly two-second full-bail and one-second accidental-runout grace. Try sliding through Finish during grace. This needs fresh live coverage; do not assume every edge case is proven.
7. Use two to four PCs for repeated races, maps, long routes, leave/rejoin, host disconnect and readiness changes. Record the build/asset version and exact symptom when reporting an issue. The code contains seven-letter R.E.S.K.A.T.E.; its dedicated rules playtest remains owner-deferred.

See RELEASE_NOTES.md in the package or docs/RELEASE_NOTES_20261016.md in the repository for the change list. Known issue: one join confirmation may still say Spot Battle.
