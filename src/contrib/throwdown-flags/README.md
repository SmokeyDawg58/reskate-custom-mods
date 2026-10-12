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
