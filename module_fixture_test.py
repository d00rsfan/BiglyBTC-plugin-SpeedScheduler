"""Replay original Java tick commands through the independent native module ABI."""
import hashlib
import json
import pathlib
import shutil
import subprocess
import sys
import tempfile
local = pathlib.Path(__file__).resolve().parent / 'tick-fixtures'
root = local if local.is_dir() else pathlib.Path(__file__).resolve().parents[3] / 'migration/fixtures/scheduler-ticks'
manifest = json.loads((root / 'manifest.json').read_text())
work = pathlib.Path(tempfile.mkdtemp(prefix='biglybtc-scheduler-module-'))
module = work / 'plugin.so'
shutil.copyfile(sys.argv[2], module)
module.chmod(0o400)
for case in manifest['cases']:
    target = work / case['case']
    target.mkdir()
    for name, expected in case['sha256'].items():
        source = root / case['case'] / name
        assert hashlib.sha256(source.read_bytes()).hexdigest() == expected
        shutil.copyfile(source, target / name)
    actual = subprocess.run([sys.argv[1], str(module), str(target / 'SavedSchedules.xml'),
        str(target / 'values.properties')], check=True, capture_output=True, timeout=45)
    # Selected row indices are internal diagnostics; the ABI returns commands/rates.
    expected = b'\n'.join(line for line in (target / 'original.expected').read_bytes().splitlines()
        if not line.startswith(b'SELECT')) + b'\n'
    assert actual.stdout == expected, case['case']
print('PASS independently loaded scheduler ABI against original', manifest['total_torrent_states'], 'torrent states')
