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
manifest={'name':'ReSkate_Throwdown_Assets','version_number':'1.0.0',
    'website_url':'https://github.com/SmokeyDawg58/reskate-custom-mods',
    'description':'Required native logos, gates, flags, menus and HUD assets for the matching ReSkate custom Throwdowns lean playtest.',
    'dependencies':[],'author':'SmokeyDawg58'}
(args.output/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
(args.output/'asset-manifest.json').write_text(json.dumps({'archive_sha256':expected,'files':records},indent=2)+'\n',encoding='utf-8')
shutil.copy2(source/'assets/launcher/icon_mods.png',args.output/'icon.png')
shutil.copy2(source/'LICENSE',args.output/'LICENSE')
(args.output/'README.md').write_text('''# ReSkate Throwdown Assets

Native logos, placement flags, Deathrace gates, menus, HUD materials and map registrations for the matching custom ReSkate 2.0.7 lean playtest.

Install as `Mods/SmokeyDawg58-ReSkate_Throwdown_Assets/`, with `layout.toc`, `Win32` and `initfs_win32` directly inside. The launcher also accepts the standalone package ZIP. Install into your active Mods directory; ModData installations use `ModData/Default/Mods`. The full playtest installer selects that directory automatically.

Everyone needs this asset mod and the matching custom DLL/launcher. The assets alone do not add custom modes to the stock runtime. No new DLL or game executable is included in this asset-only mod.

The custom runtime verifies all 17 native file hashes and mounts this required dependency once, ahead of optional mods. Keep its folder name unchanged. An incomplete or different pack stops preparation with an error instead of silently losing artwork. Asset contents are byte-identical to the previous working embedded pack; no route, HUD, font or logo artwork was rebuilt.

Download the matching runtime and full playtest instructions from https://github.com/SmokeyDawg58/reskate-custom-mods/releases . This is a local/shareable Thunderstore-format package, not evidence of a published Thunderstore listing.
''',encoding='utf-8')
(args.output/'CHANGELOG.md').write_text('# 1.0.0\n\nExtract the complete, unchanged native asset pack into a visible mod folder for the lean custom Throwdowns runtime.\n',encoding='utf-8')
print(json.dumps({'folder':str(args.output),'native_files':len(records),'archive_sha256':expected}))
