"""Embed a compiled native UI patch with deterministic paths and verification."""
from pathlib import Path
import hashlib, struct, sys

patch=Path(sys.argv[1]).resolve()
destination=Path(sys.argv[2]).resolve()
if not (patch/'layout.toc').is_file():
    raise RuntimeError('Expected the compiled Patch directory containing layout.toc')
files=sorted((p for p in patch.rglob('*') if p.is_file()),key=lambda p:p.relative_to(patch).as_posix())
if not 0<len(files)<=64:
    raise RuntimeError('Native bundle must contain 1-64 files')
blob=bytearray(b'RS1UP001'+struct.pack('<I',len(files)))
for path in files:
    relative='Patch/'+path.relative_to(patch).as_posix()
    name=relative.encode('utf-8');data=path.read_bytes()
    blob.extend(struct.pack('<IQ',len(name),len(data)))
    blob.extend(name);blob.extend(data)
destination.parent.mkdir(parents=True,exist_ok=True)
destination.write_bytes(blob)
# Verify every emitted path and payload with an independent parser.
read=destination.read_bytes();offset=12
for path in files:
    n,size=struct.unpack_from('<IQ',read,offset);offset+=12
    name=read[offset:offset+n].decode('utf-8');offset+=n
    if name!='Patch/'+path.relative_to(patch).as_posix() or read[offset:offset+size]!=path.read_bytes():
        raise RuntimeError('Native bundle verification failed')
    offset+=size
if offset!=len(read):raise RuntimeError('Native bundle trailing bytes')
digest=hashlib.sha256(read).hexdigest()
destination.with_suffix('.sha256').write_text(digest+'\n',encoding='ascii')
print(f'Native embedded bundle verified: {len(files)} files, {len(read)} bytes, SHA256 {digest}')
