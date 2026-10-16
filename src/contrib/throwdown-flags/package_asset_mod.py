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
manifest={'name':'ReSkate_Throwdown_Assets','version_number':'1.0.2',
    'website_url':'https://github.com/SmokeyDawg58/reskate-custom-mods',
    'description':'Required native logos, gates, flags, menus and HUD assets for the matching ReSkate custom Throwdowns lean playtest.',
    'dependencies':[],'author':'SmokeyDawg58'}
(args.output/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
(args.output/'asset-manifest.json').write_text(json.dumps({'archive_sha256':expected,'files':records},indent=2)+'\n',encoding='utf-8')
shutil.copy2(source/'assets/launcher/icon_mods.png',args.output/'icon.png')
shutil.copy2(source/'LICENSE',args.output/'LICENSE')
(args.output/'README.md').write_text('''# ReSkate Throwdown Assets

Native logos, placement flags, Deathrace gates, menus, HUD materials and map registrations for the matching custom ReSkate 2.0.7 lean playtest.

Install as `Mods/SmokeyDawg58-ReSkate_Throwdown_Assets/`, with `layout.toc`, `Win32` and `initfs_win32` directly inside. The corrected runtime also accepts `Mods/SmokeyDawg58-ReSkate_Throwdown_Assets-1.0.2/`, the folder Windows creates when extracting this ZIP. Older lean runtimes require the unversioned name. The launcher also accepts the standalone package ZIP. Install into your active Mods directory; ModData installations use `ModData/Default/Mods`. The full playtest installer selects that directory automatically.

Everyone needs this asset mod and the matching custom DLL/launcher. The assets alone do not add custom modes to the stock runtime. No new DLL or game executable is included in this asset-only mod.

The custom runtime verifies all 19 required file hashes and mounts this required dependency once, ahead of optional mods. Use the plain folder name or its numeric version suffix; avoid extra nested folders. An incomplete or different pack stops preparation with an error instead of silently losing artwork. Version 1.0.2 replaces R.E.S.K.A.T.E. artwork with the supplied pink skateboard logo on the picker, pages, HUD, map markers and flags. It retains the selected quiet gate/finish clips. Existing HUD animation curves, gates and old flag payloads are preserved. Install this pack with the matching updated runtime; earlier asset versions are not interchangeable. Replace the previous asset mod rather than keeping duplicate versions enabled.

Download the matching runtime and full playtest instructions from https://github.com/SmokeyDawg58/reskate-custom-mods/releases . This is a local/shareable Thunderstore-format package, not evidence of a published Thunderstore listing.
''',encoding='utf-8')
(args.output/'CHANGELOG.md').write_text('# 1.0.2\n\nUse the supplied pink skateboard logo for R.E.S.K.A.T.E. everywhere, including physical flags and map markers. Preserve native identities, HUD curves and other challenge artwork. Keep quiet Required/Finish audio. Install with the matching runtime.\n',encoding='utf-8')
print(json.dumps({'folder':str(args.output),'native_files':len(records),'archive_sha256':expected}))
