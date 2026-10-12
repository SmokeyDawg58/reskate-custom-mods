"""Extract an existing custom DLL resource as data, without executing the DLL."""
from pathlib import Path
import sys, hashlib, json
import pefile
root = Path(__file__).resolve().parents[1]
expected = json.loads((root / 'SNAPSHOT.json').read_text())['native_assets_sha256']
pe = pefile.PE(sys.argv[1])
for kind in pe.DIRECTORY_ENTRY_RESOURCE.entries:
    if kind.id != 10:
        continue
    for entry in kind.directory.entries:
        if str(entry.name) != 'ONE_UP_NATIVE_ASSETS':
            continue
        leaf = entry.directory.entries[0].data.struct
        data = pe.get_data(leaf.OffsetToData, leaf.Size)
        if hashlib.sha256(data).hexdigest() != expected:
            raise RuntimeError('The DLL contains a different native asset version.')
        output = Path(sys.argv[2])
        if output.exists():
            raise RuntimeError('Destination already exists; refusing to replace it.')
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(data)
        print('Verified native asset archive extracted:', output)
        sys.exit(0)
raise RuntimeError('Custom native asset resource not found.')
