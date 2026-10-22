"""Export the existing native archive as a Thunderstore-compatible ReSkate mod.



No game or Studio process is started. Payload bytes are preserved exactly; the

Patch/ archive prefix becomes the root layout.toc/Win32 mod layout Studio uses.

"""

from pathlib import Path

import argparse, hashlib, json, shutil, struct



parser=argparse.ArgumentParser(description=__doc__)

parser.add_argument('--output',type=Path,required=True,help='New SmokeyDawg58-ReSkate_Throwdown_Assets mod folder')

args=parser.parse_args()

source=Path(__file__).resolve().parents[2]

archive=source/'assets/one_up/native_assets.bin'

expected=(source/'assets/one_up/native_assets.sha256').read_text().strip().split()[0].lower()

with archive.open('rb') as stream:

    if hashlib.file_digest(stream,'sha256').hexdigest()!=expected:

        raise RuntimeError('The native archive does not match its checksum.')

if args.output.exists():

    raise RuntimeError('Destination already exists; use a new folder to preserve previous packages.')

args.output.mkdir(parents=True)

records=[]

with archive.open('rb') as stream:

    if stream.read(8)!=b'RS1UP001': raise RuntimeError('Unsupported native archive.')

    count=struct.unpack('<I',stream.read(4))[0]

    if count!=17: raise RuntimeError('Expected the complete 17-file Throwdown asset pack.')

    for _ in range(count):

        length,size=struct.unpack('<IQ',stream.read(12))

        if not 0<length<=512: raise RuntimeError('Invalid archive path.')

        name=stream.read(length).decode('utf-8')

        if not name.startswith('Patch/'): raise RuntimeError('Expected a Patch-relative native file.')

        relative=name[6:]

        if not relative or Path(relative).is_absolute() or '..' in Path(relative).parts or ':' in relative or '\\' in relative:

            raise RuntimeError('Invalid asset destination.')

        target=args.output/relative

        target.parent.mkdir(parents=True,exist_ok=True)

        digest=hashlib.sha256();remaining=size

        with target.open('xb') as output:

            while remaining:

                block=stream.read(min(remaining,1024*1024))

                if not block: raise RuntimeError('Truncated native file.')

                output.write(block);digest.update(block);remaining-=len(block)

        records.append({'path':relative,'bytes':size,'sha256':digest.hexdigest()})

    if stream.read(1): raise RuntimeError('Trailing native archive data.')

# These two small user-selected WAVs are embedded in the lean runtime too.

# Mirroring the exact bytes here keeps the visible asset package complete.

for audio in sorted((source/'assets/throwdowns/audio').glob('*.wav')):

    relative='Audio/'+audio.name;target=args.output/relative

    target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(audio,target)

    records.append({'path':relative,'bytes':target.stat().st_size,'sha256':hashlib.sha256(target.read_bytes()).hexdigest()})

# The Block Party renderer uses the recovered game artwork and fonts verbatim.

for art in sorted((source/'assets/throwdowns/blockparty').glob('*')):

    if art.suffix not in ['.png','.ttf','.json']:continue

    relative='UI/Deathrace/'+art.name;target=args.output/relative

    target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(art,target)

    records.append({'path':relative,'bytes':target.stat().st_size,'sha256':hashlib.sha256(target.read_bytes()).hexdigest()})

manifest={'name':'ReSkate_Throwdown_Assets','version_number':'1.0.22',

    'website_url':'https://github.com/SmokeyDawg58/reskate-custom-mods',

    'description':'Required native logos, gates, flags, menus and HUD assets for the matching ReSkate custom Throwdowns lean playtest.',

    'dependencies':[],'author':'SmokeyDawg58'}

(args.output/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')

(args.output/'asset-manifest.json').write_text(json.dumps({'archive_sha256':expected,'files':records},indent=2)+'\n',encoding='utf-8')

shutil.copy2(source/'assets/launcher/icon_mods.png',args.output/'icon.png')

shutil.copy2(source/'LICENSE',args.output/'LICENSE')

(args.output/'README.md').write_text('''# ReSkate Throwdown Assets



Native logos, placement flags, Deathrace gates, menus, HUD materials and map registrations for the matching custom ReSkate 2.1.1 lean playtest.



Install as `Mods/SmokeyDawg58-ReSkate_Throwdown_Assets/`, with `layout.toc`, `Win32` and `initfs_win32` directly inside. The corrected runtime also accepts `Mods/SmokeyDawg58-ReSkate_Throwdown_Assets-1.0.22/`, the folder Windows creates when extracting this ZIP. Older lean runtimes require the unversioned name. The launcher also accepts the standalone package ZIP. Install into your active Mods directory; ModData installations use `ModData/Default/Mods`. The full playtest installer selects that directory automatically.



Everyone needs this asset mod and the matching custom DLL/launcher. The assets alone do not add custom modes to the stock runtime. No new DLL or game executable is included in this asset-only mod.



The custom runtime verifies all 24 required file hashes and mounts this required dependency once, ahead of optional mods. Use the plain folder name or its numeric version suffix; avoid extra nested folders. An incomplete or different pack stops preparation with an error instead of silently losing artwork. Version 1.0.19 fixes native root measurement for the new HUD, bounds device glyphs, and uses fixed native artwork widgets. Version 1.0.18 adds private native Deathrace builder, rotation, route dialogs, enrolled lobby, intro/countdown, live timing, standings, notices and results components. The matching runtime keeps the previous HUD available as a fallback and through `mode hud legacy`. It retains all 2,190 existing resource identities and bundle memberships; only the Throwdown UI resource receives imports for the added skins. Native Gilroy/Skate marker fonts and existing native brush, timer, player and rank artwork are reused. The runtime bounds the HUD and pointer hits to the game viewport, preserves millisecond clocks and dynamic names, and scopes modal input to the current race. It preserves the previous compass update, which replaces the Required and Finish compass horseshoes with downward triangles (yellow Required, red Finish). Guide gates have no compass pointer. It retains the original sideways Chevron and its custom-map texture closure. The native archives store repeated payloads once per archive, with every referenced byte and registration preserved. It retains the builder controls and grounded main-flag placement. Intro, results and challenge colours are retained. All existing resource identities, bundle memberships, gate/arrow geometry, map registrations, logos and shaders are preserved. It uses a fixed native blue/yellow chevron, with all Deathrace gates and markers registered in every supported map. Chevrons retain their placed pose and are decorative, silent and non-colliding. It preserves the supplied pink R.E.S.K.A.T.E. artwork, selected quiet gate/finish clips and existing HUD animation payloads. Install this pack with the matching updated runtime; earlier asset versions are not interchangeable. Replace the previous asset mod rather than keeping duplicate versions enabled.



Use the runtime pair supplied with this local playtest. Everyone joining must use this matching asset version. Use only the runtime supplied with this asset version.

''',encoding='utf-8')

(args.output/'CHANGELOG.md').write_text('# 1.0.22\n\nRender the recovered rectangular gate and right-facing seamless chevron as native depth-tested world meshes, using resident game FX materials. Replace bitmap HUD lettering with complete native scalable fonts, Unicode name symbols, consistent button glyphs and a continuous countdown/race transition. Preserve all previous asset identities, native registration payloads, map residency, route data and gameplay.\n\n# 1.0.21\n\nUse ReSkate’s existing renderer for the complete Deathrace interface with the recovered Block Party font and texture atlas. Keep compact viewport-safe dialogs, brush headers, dynamic names, clocks, standings and enrollment. Preserve all native compass markers and race mechanics.\n\n# 1.0.20\n\nRestore the native label text/font bindings using engine property hashes, correct native linear tint values, and retain the original textured notice fills and brush artwork. Preserve bounded panels, device glyphs, existing HUD backups and all gameplay.\n\n# 1.0.19\n\nFix invisible native HUD cells by giving direct Rime layer children fixed-size roots. Bound device glyphs to 56 native units and prevent automatic presenter sizing. Render logos, timer, player and rank art through immutable native widgets. Preserve all previous assets and Deathrace rules.\n\n# 1.0.18\n\nAdd the approved red native Deathrace presentation with private fixed-size skins, native fonts, existing game artwork, responsive viewport fitting, map-scoped route controls, exact race clocks, ordered standings and result rows. Retain the previous HUD for fallback. Preserve every existing asset payload except the owning UI resource import list; preserve original metadata and bundle memberships.\n\n# 1.0.17\n\nUse yellow Required and red Finish downward compass triangles with cut-paper edges, heavy ink and halftone print. Guide gates have no compass pointer. Retain every existing asset identity and change only two compass pixel chunks. Preserve placed Throwdown logos through the matching runtime.\n\nRemove duplicate CAS payload storage without changing artwork, texture resolution or registrations. Restore the original sideways Chevron geometry and artwork; retain its texture with both layers on custom maps. Retain the builder title, fixed rows and original grounded starting flag. Preserve the intro/results layout and independent challenge colours. Preserve all existing resource identities and bundle memberships. Use a fixed stunt-style blue/yellow chevron with native materials in every supported map. Preserve the working map residency. Keep the pink R.E.S.K.A.T.E. artwork, quiet gate/finish audio and existing HUD animation payloads. Install with the matching runtime; all playtest participants need the same version.\n',encoding='utf-8')

print(json.dumps({'folder':str(args.output),'native_files':len(records),'archive_sha256':expected}))

