> Current package and complete export instructions: [Assets 1.0.6](ASSET_PACK_1_0_6.md). The versioned recipe below is retained as authoring history.

# Custom Throwdown physical flags

Built and installed on 2026-10-11 for Skate executable SHA256
`fbce74d5e28ef525dbba2cb4adbebc13405bdbd88f31bc940bca45e4ae88b8f9`.

The native Spot Battle backend remains the queue adapter. The local client
rewrites **InstanceId** to `ReSkate.<mode-key>` only while our placement owns the
queue (Create, owned Update, owned HostSet). **MpEventSeriesId remains SpotBattle**.
The verified native registry expression (hash `0x743b15f5`, RVA `0x7fcc60`)
selects RegistrationPrefab by series and instance identifier.

Nine private registrations select the native banner mesh with a private
ObjectVariation, fabric texture and compiled material. Their colors use the
challenge registry's RGBA palette: oneup blue, jam pink, meat orange, race red,
domination violet, graffiti green, tag yellow, infection green and hide blue.
The badge region uses the existing approved challenge artwork. Native pole,
fabric normals, distress, UV layout and SVCC graphics are retained.

The fabric is now 1024x4096, with a sharp 928x928 approved badge region,
13 complete native-format mips and firstMip=0. Keeping the full chain resident
avoids falling back to the streamed tail at close range. The earlier 256x1024
package's logo/colors were confirmed in game; the quality upgrade needs visual
verification under motion and distance changes.

Placement uses a separate client validator prefab. Nine private copies keep
the stock validation/physics graph and internal entity/path GUIDs, changing
only the mesh-component import. They load in the original validator bundles,
with private cloth dependencies resident there. These client previews are not
appended to the network registries. Offline decompilation established that RVA
0x915090 queries entity components; it does not create a prefab. The incorrect
hook at that address was removed. The private preview copies remain authored,
but their creation bridge is unverified.

Native variations derive their compiled keys from mesh base key plus the
ObjectVariation NameHash. Opaque base: `0xac2054d49ce97317`; shadow base:
`0xfd8dbcc823ff91d5`. The stock variation hash is `2602416751`. Each private
material replaces exactly the Spot Battle fabric texture identity. Both new
shader depots contain only nine private rows. Root shader program and texture
lookup resources retain every stock row and append eighteen aliases. The
prefab registry retains its stock entry and appends nine instance selectors.
Six native level network registries append the private prefab roots. Private
prefabs and all flag dependencies join each registry's owning bundle; native
registries stay in their own original level bundles. Their eager loader cannot
accept unresolved prefab imports from a deferred UI bundle or another level.

Cancellation retains ownership of hidden custom notification slots until the
queue removes them. Retired source identities survive menu/HUD rebuilds. A
genuine native Create or Join advances a revision and releases suppression, so
native Spot Battle can present its own banner. Expired map-card storage is
checked before restoration.
Every mounted HUD presenter receives its own backup, including remounts that
reuse an existing notification source. Retiring one presenter does not discard
the shared private source or participant model. Map cards accept the two
display rows when the optional creator row is absent.

## Rebuild with ReSkate Studio

Use the companion `native_throwdown_flags.inl` in Studio's `src/cli`, included
by `one_up_badge.cpp` after `throwdown_editor_assets.inl`; call
`run_native_throwdown_flags(count,args)` at the beginning of
`run_one_up_badge_command`. Include `modding/materials/shader_lookup.h`.
The companion shader-depot files contain the exact-texture replacement helper
used by the author. These are source snapshots from the tested Studio build.

```text
reskate_cli native-throwdown-flags <game-root> <map-identity.fbproject> <approved-badge-directory> <output.fbproject>
reskate_cli compile-mod <game-root> <staging-directory> <output.fbmod>
python build_oneup_native_bundle.py <staging-directory>/Patch assets/one_up/native_assets.bin
```

The author verifies all serialized prefab selectors and registration GUIDs,
and independently compares the shader parameters before and after encoding.
Every pole/normal/mask/non-badge parameter must remain identical. Preview PNGs
are written next to the project for visual review.

Validation: all 73 runtime regression tests passed, including native menu
ownership and challenge registry contracts (22.46 seconds). The compiled
project contains 207 resources, 36 bundles and 12 TOCs; the embedded archive
has 17 files and 297,917,442 bytes. Its bytes were verified in the final DLL
without executing it. Embedded SHA256:
`a297e86b5b74692a9cdff2bb6b47abb0cfb636c2becd3223455fdcd2bf90e676`.

The first package crashed in the native NetworkRegistry loader at game RVA
`0x35911a7` while dereferencing an unresolved Objects entry. The corrected
package fixes registry scope and dependency residency. The native dump and
offline unwind are preserved in the task's `work/branding-flags-crash` directory.
The corrected package loaded the world and both 1-UP/Deathrace placed flags;
the user confirmed the corresponding logos/colors. The subsequent HUD/card,
placement preview and resident high-resolution changes need live verification.
Native panel/celebration work is deferred at the user's request.

## ReSkate 2.0.6 and distant flag logos

Official tag v2.0.6 (314d31fe7bf73866b4ae9b6a8195de50ced26312) is merged
with the custom pack. The runtime and launcher are stamped 2.0.6; launcher
binary auto-updates remain disabled so they cannot replace the custom runtime.
Multiplayer uses the official protocol 49.

The registration prefab's HUDElementIcon and Icon overrides import native
RimeTextureAssets at import slots 1 and 4. Each private registration now binds
both to its approved custom Rime logo. The 45 dependent picture, texture,
resource and chunk assets are included in every owning registry bundle.
Serialization verifies both icon identities and the original prefab layout.
The stock registration and its selector remain unchanged.

Waiting HUD backups are captured before publishing a private presenter, so
remounts retain the original native callback identity. A temporarily missing
skater pose while viewing the map or respawning no longer unloads the custom
coordinator. Real level unloads and generation changes still end the match.
The coordinator fixture covers 120 frames without a pose, Leave during that
interruption, Start on return, and real world unloading.

All 77 CTest checks pass (38.64 seconds). The compiled project contains 207
resources, 36 bundles and 12 TOCs. The final DLL embeds the exact 17-file,
297,916,006-byte archive without executing it during verification. SHA256:
`bd359876f83796f13b091e7a06cc329d3464e5cf98d8f62d895ee8a92f25cbc7`.
Manual confirmation of the map-return controls and distant logos is pending.

## Persistent picker and repeated Leave fix, 2026-10-11

The second waiting notification could be remounted in a new presenter while
still pointing to our previous private notification. Resolve that private
handle back to its native source before discovering mounted notifications,
then refresh its current mode and Start/Leave callbacks. Presenter backups
always point to the native source. A 1-Up Leave also explicitly retires its
flag registration, including while the skater pose is temporarily absent.

After a level change, the picker had depended on another UI screen first
instantiating two model types. `throwdown_menu_residency.inl` authors
`LogoTexture` and `LogoContents` in the picker's existing resource container.
The runtime uses those authored types and the donor cards' types to rebuild
the grid for the new UI generation. This retains all 20 existing defaults.
Include this companion after `native_throwdown_flags.inl` in Studio's
`one_up_badge.cpp`, and dispatch `run_throwdown_menu_residency(count,args)`
from `run_one_up_badge_command` before its other subcommands.

```text
reskate_cli throwdown-menu-residency <game-root> <existing-hq-flags.fbproject> <output.fbproject>
reskate_cli compile-mod <game-root> <staging-directory> <output.fbmod>
python build_oneup_native_bundle.py <staging-directory>/Patch assets/one_up/native_assets.bin
```

The placement bridge checks seven exact native graph hashes and layouts.
For an owned custom placement, it resolves the private placement prefab,
copies only that interpreter invocation's constant page, and substitutes the
prefab reference for the native call. The original page is restored after
the call, including nested calls and exceptions. Shared asset imports are
unchanged. Tests cover this restoration, graph/layout rejection, waiting
notification remounts, and explicit registration retirement.

All 77 regression checks passed; six environment-blocked checks passed on
retry outside the sandbox. The verified DLL embeds 17 files, 297,919,878
bytes, SHA256 `5933f44abc957382d08a9498da8a96543a64c337c0ae7930fbc7180b3bb2e1b5`.
Manual checks are still required for repeated Leave, changing maps and the
branded placement preview. The native panel and celebration remain deferred.

## Owned distant bubbles and pre-start Deathrace cleanup, 2026-10-12

The user confirmed repeated 1-Up Leave, the custom grid after changing maps,
and the placement preview work. The remaining world bubble uses a different
ActivityDisplay from the map marker. Its exact owner is now resolved from the
owned MMID's POI EntityHandle, including the entity generation, to its
MapObjectComponent (`1798a1f6-414e-43b5-afde-00285bd7927d`, 704 bytes).
The model handle at 576 must match the POI DataContext; the embedded MMID at
656 must match the registration. The handle at 584 is the ActivityDisplay
referenced by the world bubble's context. The live capture verified this
connection and its stock Spot Battle texture.

`native_flag_bubble.h` performs a bounded read-only ECS store lookup. The UI
publishes the registered challenge logo through the native typed model API,
retaining the original artwork and restoring only an unchanged owned field.
It does not modify shared texture assets, native providers, or official flags.
Fixtures cover expired generations, foreign MMIDs/contexts, missing/retired
components, changed field layouts, malformed leaves and cyclic bucket lists.

A reused 1-Up Leave delegate now requests Quit for its currently registered
game-mode owner before retiring the flag. Deathrace releases its tracked gate
entities before checking the entity creation gate, which can close after
Leave. Saved route settings remain available for the next placement. Runtime
tests ensure that ordinary 1-Up Leave and a mismatched old game cannot quit a
replacement mode. Visual bubble and pre-start gate cleanup checks are pending
for this build; the native panel and celebration remain deferred.

All 77 CTest checks passed in 25.46 seconds. A separate read-only process probe
using the production C++ resolver returned the exact live display handle
verified by the bubble context capture. The installed DLL SHA256 is
`d7ac284f733d4d2378d59f99b9a2aa86c90e5e5c4fc0fff49147d1aac1c26819`;
its embedded 17-file native archive is unchanged at 297,919,878 bytes and
SHA256 `5933f44abc957382d08a9498da8a96543a64c337c0ae7930fbc7180b3bb2e1b5`.

## Waiting card reuse, 2026-10-12

The user confirmed the distant bubble and pre-start gate cleanup. A later
Deathrace -> Leave -> 1-Up reproduction captured a private notification with
the 1-Up title and logo but `mode-wait-start` / `mode-wait-leave` delegates
still carrying the previous Deathrace game ID. Two live presenters pointed
to that same private card. Those commands were rejected for the new 1-Up
lobby, leaving the native queue and radial menu lock active.

Waiting cards now publish stable `flag-wait-start` / `flag-wait-leave`
delegates keyed by the notification source, rather than the previous game ID.
Their lifetime belongs to the model manager instead of the picker page. At
dispatch, the exact private card must still be visibly mounted, and its flag
must be confirmed, handed off, not retiring, and in a custom lobby. The
current registration selects either 1-Up or the native game-mode coordinator.
Hidden cards, a stock remount, another notification, and running matches
cannot use these controls. The mounted notification graph supplies the
source directly, so rebinding does not depend on a second root scan.

Regression fixtures cover retained delegates after a picker generation
change, a closed picker, repeated card reuse, and each rejected ownership
state. Other picker and match callbacks keep their original generation
checks. Manual repeated Leave/Start and mode-switch checks remain required.

## White guides and the complete picker asset contract, 2026-10-12

The initial white-guide pack was authored from an older project that lacked
the resident `LogoTexture` and `LogoContents` model exports. The runtime
registered the custom mode cards, then failed while creating the first grid
logo and fell back to the three stock cards. The repaired pack starts from
the full persistent-grid project instead. An independent binary audit
confirms all 207 original resource payloads, metadata and bundle memberships
are unchanged; it appends only the three private white-guide roots.

`deathrace_guide_assets.inl` now validates the typed defaults and resource
exports for `LogoGrid`, `LogoTexture` and `LogoContents` before authoring. It
reloads the saved project and verifies all picker default target identities
remain unchanged. The older project is rejected before any output is made.
Gate-only extensions must use a project containing these resident templates;
successful asset compilation alone does not validate the native grid.

The repaired 17-file archive is 298,022,035 bytes, SHA256
`ad111da5ed579c631eaf7d92ac3d8f1a85ba65b3956561c5bb94e109ffb331a2`.
The new Deathrace routing rules and existing map, flag, bubble, controls,
spawn and HUD fixes are retained. No GitHub push was made for this repair.

All 77 CTest checks passed in 23.47 seconds. The asset contract rejection
check and the 207-resource preservation audit passed, and the embedded DLL
archive matches the compiled pack exactly. The repair was installed with
verified backups while the game and launcher were already closed; neither
process was closed, restarted or controlled. Visual grid confirmation on
the next launch remains pending.

## Editable Deathrace deadline, snapping and race HUD, 2026-10-12

Deathrace now uses the selected `Settings::duration_s` for the shared finish
cutoff, checkpoint messages, late countdown and DNF. The native setup slider
allows 30 through 3600 seconds in 30-second steps. The alternate setup panel
also exposes deadline choices. Only the final 30 seconds are visible during a
race. Modes wire version 12 rejects older fixed-deadline race clients; all
participants need this build.

`race_gate_missed` uses a swept crossing of the next required gate's plane.
A wide detour or flying over it raises a persistent red `MISSED GATE / GO BACK`
notice until the required gate is crossed. Optional guides are exempt. This
removes the old 20-metre warning cutoff without advancing required progress.
Race status also no longer gets overwritten by the general mode description.

Placement snaps by default to a 0.5-metre X/Z grid and 15-degree yaw steps.
Near the preceding placed gate's centre line (within 1.5 metres laterally,
with at least 3 metres longitudinal separation), it aligns centres and snaps
spacing to a metre. Ground-hit height is preserved. Controller Y or keyboard G
toggles free placement for curves and forks. Preview and placement consume the
same snapped pose; the reserved finish does not become the alignment reference.

The new `deathrace-visual-assets` Studio CLI command starts from the repaired
210-resource project and requires the complete typed picker contract. It dims
only the three private guide roots to neutral RGB 0.22 and scales the three
mandatory roots' existing colour vectors by 0.18. The native additive shader
therefore gives gray guides and a stronger fade on mandatory gates without
inventing an unsupported alpha parameter. Finish roots stay red. The private
HUD logo presenter's root and both texture elements now measure 96 by 96.

The Deathrace HUD uses this measured logo with its brush title and live required
gate progress, a large red ordinal, gold/silver rank panels, highlighted local
player names and 800-by-76 standings rows. A separate red missed-gate notice and
late countdown keep critical race information distinct. 1-UP has a wider
508-by-76 name panel and aligned penalty stamps in a 760-wide row.
Layout references: the official [Mario Kart 8 manual](https://csassets.nintendo.com/noaext/image/private/t_KA_PDF/manual-WiiU-Mario_Kart_8?_a=BATCtdAA0)
and [Gran Turismo 7 race-screen manual](https://www.gran-turismo.com/au/gt7/manual/race/04).
The implementation uses existing native ReSkate widgets and styles.

The independent asset audit preserves all 210 resource identities and bundle
memberships and 203 payloads byte for byte; only the six private gate roots and
private HUD logo widget change. The complete custom picker payload is unchanged.
The embedded 17-file bundle is 298,022,175 bytes, SHA256
`970de5f969bbeed205ce4e7d608da3391ac748d9e55d128dcc9097c2c2456ed2`.

All 77 CTest checks passed in 13.77 seconds, including custom deadlines from
30 seconds to 60 minutes, cutoff/late-clock behavior, serialized settings,
wide/vertical detours, normal crossings and alignment/free-placement cases.
The release DLL and exact embedded asset bundle passed verification. The build
was staged in `outputs/ReSkate-2.0.6-custom` and installed with verified backups
while the game and launcher were already closed. Neither process was closed,
started or controlled. No GitHub push was made. In-game appearance, input and
multiplayer confirmation remain pending on the user's next manual launch.

## Saved courses, confirmed editor exit and native HUD assets, 2026-10-12

`RaceCourseMemory` retains a deep copy of each map's Deathrace settings in the
current process. The course is saved after edits and before finish/cancellation
cleanup; reopening Deathrace restores its checkpoints, guiding gates, widths,
yaws and single finish. The native starting flag can be repositioned for the
next placement. Gate entities are still removed when leaving the event. This
is session memory, not disk persistence across game restarts.

In the editor, controller B or Escape opens a native UI confirmation. A/Enter
leaves the editor and keeps the course; B/Escape dismisses the confirmation.
Other editor actions are suspended while the confirmation is visible.
Controller Back/View or keyboard Backspace must be held for 1.5 seconds to
Clear All. Its native device prompt shows progress; partial holds, focus loss
and a held button after clearing cannot cause repeated resets. Clearing retains
the starting anchor and removes every checkpoint, guide and finish. X still
undoes the last gate. Y/G toggles snapping; left/right stick clicks select the
previous/next angle in 5, 15, 30, 45 and 90 degrees. LT/RT is labelled Gate types.

Gate width now scales the crossing height too, in both local swept geometry
and referee acceptance. A missed mandatory gate queues native teleport recovery
to the last cleared mandatory gate, skipping guides, or the start if none is
cleared. Crossing reports are suspended during recovery so teleport movement
cannot count as progress. After the native teleport reaches the recovery pose,
the verified local physics body receives a bounded 4 m/s rolling speed. The
request expires, validates local body ownership and requires competition stock
physics; speed cheats, no-bail and noclip stay disabled.

The prior HUD presenters inherited notice icons and model visibility defaults.
`deathrace_hud_assets.inl` authors private native rank, name, progress, position
and warning presenters with explicit geometry, dark fill and red accents, plus
editor and confirmation backgrounds. Static native Deathrace and 1-UP artwork
removes the generated texture model's visibility and size overrides. Existing
native text/font helpers and device-aware button models are retained. 1-UP
names are now 620 by 76 pixels inside an 872-wide standings row.

Copy the companion `deathrace_hud_assets.inl` and updated
`deathrace_guide_assets.inl` into Studio's `src/cli`. The guide command includes
and dispatches the HUD command, which starts from the complete polish project:

```text
reskate_cli deathrace-hud-assets <game-root> <polish.fbproject> <output.fbproject>
reskate_cli compile-mod <game-root> <staging-directory> <output.fbmod>
python build_oneup_native_bundle.py <staging-directory>/Patch assets/one_up/native_assets.bin
```

The preservation audit verifies all 210 existing resource identities and bundle
memberships. Of these, 209 payloads are byte-identical and only the private
HUDLogo_Widget changes. Eight new private widgets bring the total to 218
resources; the complete picker remains unchanged. The verified embedded bundle
contains 17 files and 298,654,275 bytes, SHA256
`1807b53cb64ba61c8cc69aea035afce53b437c5966b455701d119180afd9f035`.

All 77 CTest checks pass in 23.18 seconds, including course deep-copy/map
isolation, last-mandatory recovery, scaled trigger height, adjustable angle
snapping, partial/focus-lost holds and Clear All. The release DLL and symbols
were installed with verified backups while the game and launcher were already
closed; neither process was stopped, started or controlled. No GitHub push was
made. Native appearance, device input, recovery speed and multiplayer behavior
still require manual in-game verification.

## Black/red HUD, discrete controls and world changes, 2026-10-12

The native renderer uses `ColorValue.RawColor`; changing only the older `Color`
field left the editor and race panels white. The authoring helper now writes
both fields, clears palette overrides, uses translucent near-black fills and
four red outline edges, and explicitly sets white label colors. The editor
background covers the complete control block, with separate compact action
rows. Race headers, rank/name/progress cells and results use the same palette.
Existing file/root GUIDs and bundle memberships are retained when replacing
private presenters. Three private backdrop widgets are added.

Gate sizing uses rising-edge inputs and changes five percentage points of the
default width per press. Holding the D-pad no longer repeatedly resizes. Each
rotation press advances the full selected angle, including 45/90-degree steps.
The snap angle also controls positional attraction to the previous gate's
centreline: reach increases from 0.5 to 6 metres while permitting fork routes.
Y toggles snapping; left/right stick clicks select its angle. LT/RT selects Gate
types; Back/View remains a hold-to-clear action.

Mandatory miss detection tests the scaled swept crossing before recovery and
waits until a forward outside crossing has travelled beyond a scaled grace
region. Backwards approaches do not trigger recovery. The later-gate proximity
sphere is removed: only a valid later crossing after passing the current plane
can count as a skip. Recovery applies a bounded 6 m/s local rolling speed after
the native teleport completes. Width and height remain shared by visuals and
the local/referee crossing tests.

Reopening a mode now retires the old game, flag/input ownership and recovery
before restoring its course. The native HUD detaches when the game ID changes;
results reuse their anchor graph instead of allocating more roots every tick.
Placing the start flag always opens the gate editor, including for a complete
restored course. The owner can change gates, hold Back/View to clear and rebuild,
or press Start to save the course unchanged before starting the race. The
console's Place Route path also preserves existing gates until explicitly cleared.
These changes address lifecycle hazards found during the reopening crash
investigation; no new dump established its exact cause. Repeat in-game reopening
still requires manual verification.

The grid disappearance after a map switch logged a missing private picker
widget. Asset lookup now searches all `0xbbf` native domain-table slots instead
of only the first 32, and releases cached assets/anchors when the world changes.
The complete custom picker resource remains byte-identical in the asset pack.

The independent audit verifies 211 payloads unchanged, seven private HUD roots
updated, three added, and all 218 prior identities/bundle memberships preserved.
The 221-resource compiled pack embeds 17 files totalling 298,898,203 bytes:
SHA256 `2c891bb17c1f6bebfc51feb2b8e3467bb371d1d06325e40ee5d8332b91b960af`.
The DLL contains that exact archive; all 77 CTest checks passed. The update is
staged while the user's game remains running. New native appearance, enlarged
gate recovery, map changes, reopening and multiplayer checks remain pending.

Public releases at [reskate-custom-mods](https://github.com/SmokeyDawg58/reskate-custom-mods)
include the matching DLL, launcher, native asset archive and SHA-256 manifest.
Those release files must accompany every future owner-authorized push. Future
updates are pushed only when the owner explicitly requests them.
