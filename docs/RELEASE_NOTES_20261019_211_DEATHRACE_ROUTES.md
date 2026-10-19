# ReSkate 2.1.1 custom — original Chevron, route library and Deathrace update

This update moves the custom build from the previous public ReSkate 2.1.0 package to official ReSkate 2.1.1, carries forward the R.E.S.K.A.T.E. integration and online enroll-once/host-Start flow, and publishes the accumulated Deathrace editor, route-sharing and recovery changes. It includes the original Chevron restoration and the Save/Load input fix requested on 19 October.

All 92 automated Release checks pass. Both real Improved-map inputs were also rebuilt and independently inspected offline. This is a playtest prerelease: those checks do not substitute for gameplay confirmation on two PCs. No new live gameplay result is claimed for this exact build.

## Downloads and installation

There are exactly three player downloads: `ReSkate.dll`, `ReSkateLauncher.exe`, and `SmokeyDawg58-ReSkate_Throwdown_Assets-1.0.15.zip`. Download all three. Everyone playing together needs this matching runtime pair and Assets 1.0.15.

Close the game and launcher yourself and back up the existing pair and asset folder. Put the DLL and launcher beside `Skate.exe`. Replace the previous Throwdown asset folder under the active Mods directory with the complete extracted pack. The normal folder is `Mods/SmokeyDawg58-ReSkate_Throwdown_Assets`; version-suffixed folders are supported too. Keep `layout.toc`, `initfs_win32`, `Win32` and `Audio` directly inside the mod folder, without an extra nested folder or duplicate enabled copy. Installations using ModData should use their active Mods directory.

Launch normally and let the launcher rebuild generated files when prompted. Do not copy the offline validation maps into the installation. Source, the exact reconstruction patch, asset-authoring code and detailed documentation remain in the repository; Full, runtime-update, source and diagnostic ZIPs are not added as release attachments. Existing releases are retained.

## Original Chevron restored, including its custom-map texture

- The previous local Assets 1.0.14 substituted a different arrow with a stem. That was the wrong artwork. Assets 1.0.15 restores the two original sideways Chevron blueprints byte for byte under their existing private identities. The in-menu object name remains **Arrow**, as requested; its appearance is the original Chevron.
- The missing custom-map dependency was verified in the earlier generated map: both original mesh layers, their private shader depot and default material mappings were present, but the original Chevron mask texture was absent from the replacement core.
- The matching runtime now explicitly retains that exact texture for the Throwdown asset owner in the core bundle. Its texture resource and streamed pixel chunk follow through the existing dependency pipeline. Similar-looking names, unrelated additions, wrong owners and other bundles remain excluded.
- Both rebuilt Skate2Improved and Skate3Improved copies now contain the original mesh identities, both default material associations, private shader depot, original texture EBX/resource, and its readable 5,592,432-byte pixel chunk. The compiled Chevron material references that retained texture identity.
- The original meshes remain fixed in the authored pose, with no camera-facing schematic. Existing tilt, roll, gentle floor alignment, independent Arrow snapping and decorative/non-colliding behavior are retained. Existing mesh-chunk residency and crash guards remain enabled.
- Only the two Arrow blueprint payloads change from Assets 1.0.14. All other 2,188 native resource payloads and all 2,190 resource registrations/metadata remain identical. This restoration does not roll back the newer menus, logos, map loaders or route work.

## Save and Load cannot also place a gate

- The builder consumes input throughout the Save, Load and Rename dialogs, the Windows file-selection return, and the transition back to the builder.
- The release latch now uses the physical controller sample through the existing menu bypass. A zeroed gameplay sample while the overlay owns input is no longer mistaken for a released button.
- The latch remains armed while another overlay is open or the game loses focus. Confirm/cancel input must physically come up before a new builder press can act. Enter and mouse confirmation use the same release protection.
- Opening Save also consumes that builder tick, so a simultaneous confirm cannot add an object before the naming dialog appears. Save, load/import and cancel transitions retain the release barrier.
- Regression cases cover masked menu samples, held A/B/Start after dismissal, the release frame, a fresh subsequent press, and keyboard/mouse confirmation. Live controller timing is still part of the playtest checklist.

## Named, readable and shareable routes

- **Tap Start to save; hold Start for 1.5 seconds to load.** The hold uses the existing progress-ring treatment. A saved course does not silently load when returning to the world.
- Save and Rename use normal keyboard text entry. No on-screen keyboard or controller letter grid is included.
- The centered Load menu lists only routes for the current map, with their names and Required-gate/object counts. Initial focus is the route row. D-pad Right moves from the route to its pencil and then folder button; Left reverses, and Up/Down move between rows.
- The pencil renames the actual JSON filename and its display name while preserving route identity, geometry and settings. Blank names and conflicting same-map names are rejected instead of overwriting another route.
- The folder button selects the exact JSON file in Explorer for sharing. **Load .json file** opens the native Windows picker, validates the file and current map, then imports it without silently overwriting a different course.
- Routes are readable, indented JSON under `<game folder>/Mods/ReSkateRoutes/Deathrace/<map>/<route name>.json`. Files include the route name, map, settings, start/finish, gate types and positions, yaw, tilt, roll and size. They contain no encrypted or opaque geometry blob.
- Opening the library migrates compatible older root-folder JSON/binary saves into Mods while retaining the originals. Duplicate-course handling and a migration receipt prevent renamed routes from reappearing under their old names. Actual user-file migration occurs when the library is opened, not during installation.
- The launcher treats `Mods/ReSkateRoutes` as save data, not an enabled content mod. Saving or renaming a route therefore does not create a bogus mod entry or invalidate map-content fingerprints.
- Loading retains the newly placed main starting flag and selected race duration. The loaded draft stays private until saved. Existing enroll-once/host-Start behavior is preserved.

## Builder and presentation changes carried forward

- The control panel is labeled **Deathrace builder**, with revised row sizing, spacing and bounds. The separate route-info panel identifies Required gates, Finish gates and Arrows, and reports route counts, height, size and draft state.
- Required gates have build-only sequence labels above them; those labels are not racing HUD markers. Arrow selection keeps the requested user-facing name.
- Rotation uses small 5-degree steps with faster held D-pad changes. Tilt/roll controls, degree readouts and left-stick-click orientation reset are retained. Authored orientation stays independent of later camera movement and can be reset after leaving and re-entering rotation mode.
- Arrow snapping can choose among relevant placed Arrows instead of forcing the most recent one. Gentle automatic floor alignment when looking down is retained.
- Gate placement supports authored height/tilt, undo/redo and deliberate hold-to-clear. The redundant start-flag object is absent from the gate-object cycle; the original native main starting flag remains the start anchor. Ground validation is retained for that main flag. Its RT height-boundary behavior still needs live confirmation.
- Intro and results work includes larger title/logo treatment, bounded centered layouts and theme colors. Native challenge identification, map/nearby-card work, custom-map HUD residency and independence from stock Compass/D-Pad visibility are carried forward. The two-PC checklist below specifically checks these historically troublesome paths; they are not described as newly live-verified.

## Race flow and recovery changes carried forward

- The Finish is ignored until every Required gate has been passed. Required gates remain ordered; decorative Arrows and guide objects do not advance that sequence.
- Passed gates hide and can reappear when deliberately rewinding with D-pad Up. The Finish has its own compass marker. Merely missing a gate does not immediately teleport the player; later-gate checks retain the intended missed-gate handling.
- Starts use an evenly spaced row based on the main flag's facing direction. Each matching client holds its validated local board physics stationary through countdown and releases at GO.
- The selected race duration is used, from 30 through 3,600 seconds, rather than always ending at two minutes.
- A genuine unrecovered bail has a one-second mod grace period measured from the first fall. Repeated signals cannot extend it. Native teleport/fade/streaming may add visible time beyond that grace.
- Ordinary bail recovery keeps the latest confirmed safe grounded location. Deliberate gate returns use a three-metre run-up behind the authored gate. A safe-location return faces the next Required gate, skipping guide/Arrow objects, instead of reusing a later wrong-way heading.
- Recovery momentum comes from trusted onboard movement before leaving the board. Offboard movement does not replace the saved speed. Linear/angular motion is held while the return is pending, and saved speed is delivered once in the route direction when the board receiver is available.
- Offboard/runout alone does not start an automatic reset timer. Recognized recovered control cancels pending bail recovery. Hippy-jump filtering and gate passage while offboard are retained. Exact hippy-jump, spread-eagle and rolling behavior must still be checked in game; automated rule tests are not proof of every native animation outcome.

## Smaller complete asset pack

The compact native archives retain all required artwork, audio, texture resolution and registrations. Repeated encoded CAS payloads are stored once per original archive, preserving install-chunk ownership; no speculative feature pruning or lossy texture resizing is used.

For Assets 1.0.15, compaction verifies all 15,833 references across 40 bundles, 12 TOCs and 309 chunks. It removes 11,223 duplicate stored copies, reducing CAS storage from 497,363,466 to 190,256,034 bytes, a saving of 307,107,432 bytes. Every referenced encoded payload is compared after repacking. The ZIP is approximately 119 MB rather than the earlier approximately 354 MB package. It still includes all 19 required native/audio payload files plus package metadata.

## Validation and practical limits

- Release build: all **92/92 checks pass**, including route JSON disk operations, migration, current-map selection rules, recovery timing/direction, menu release handling and map-merger scope checks.
- Actual map integration: isolated rebuild of the installed Skate2Improved and Skate3Improved inputs, followed by a separate serialized-data reader checking the complete original Chevron dependency chain in both outputs. The same reader rejects the earlier build at the missing texture.
- Asset preservation: two intended blueprint changes; the other 2,188 payloads and all 2,190 metadata records unchanged. Lossless archive compaction independently verifies every reference.
- Existing exact-code route-menu previews remain applicable because the drawing code is unchanged: 18 bounded layouts at six resolutions, 60 D-pad navigation cases and six first-row A-activation cases. No native game input was synthesized for these checks.
- Source is pinned to official ReSkate 2.1.1 commit `db99ed4d724dd77edd2abf62f3de0dbc5bb6717e`. The custom source reconstruction is reapplied and verified separately. Protocol 52, mode wire 16 and 1-UP wire 5 are preserved. Runtime and complete asset hashes are recorded with the release source.
- Production diagnostics remain disabled in the shipped lean build. No game executable, Steam DLL, personal routes, settings, crash dumps or credentials are included.

Please try the original Chevron on both custom maps after the launcher rebuild; load/save with A and Enter held briefly and confirm the gate count does not change; navigate pencil/folder with the D-pad; share/import a JSON; then test two-player start/countdown, selected duration, safe-ground bail direction/speed, hippy jumps/offboard passage and HUD/card identity after map travel. These live outcomes remain pending for this exact build.

Maintained by SmokeyDawg58. Upstream ReSkate authorship, GPL-3.0 and existing R.E.S.K.A.T.E. contribution credits are retained. Integration, diagnosis and validation received substantial OpenAI Codex AI assistance.
