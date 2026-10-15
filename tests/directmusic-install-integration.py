"""Exercise real pinned CAB extraction using a supplied verified host CAB runtime.
Usage: python3 tests/directmusic-install-integration.py SDK.exe CAB_ROOT
No network, Microsoft files, or game assets are added to the repository.
"""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

repo = Path(__file__).resolve().parents[1]
if len(sys.argv) != 3:
    raise SystemExit(__doc__)
sdk, cab = map(Path, sys.argv[1:])
with tempfile.TemporaryDirectory(prefix='st-directmusic-') as temporary:
    root = Path(temporary)
    shutil.copy2(repo/'tools/prepare-directmusic.sh', root/'prepare-directmusic.sh')
    runtime = root/'runtime'
    (runtime/'downloads').mkdir(parents=True)
    shutil.copytree(cab, runtime/'cabextract')
    archive = runtime/'downloads/directx_feb2010_redist.exe'
    shutil.copy2(sdk, archive)
    dest = root/'prefix-wine-10.0/drive_c/windows/syswow64'
    dest.mkdir(parents=True)
    original = root/'wine-builtin.dll'
    original.write_bytes(b'preserve original runtime')
    (dest/'dmusic.dll').symlink_to(original)
    save = root/'prefix-wine-10.0/save.dat'
    save.write_bytes(b'preserve save')
    def run(*args):
        return subprocess.run(['sh', str(root/'prepare-directmusic.sh'), *args],
                              capture_output=True, text=True, timeout=120)
    # Extraction failure leaves every prefix module and readiness marker untouched.
    extractor = runtime/'cabextract/cabextract'
    real = extractor.read_bytes()
    extractor.write_text('#!/bin/sh\nexit 9\n')
    extractor.chmod(0o755)
    failed = run()
    assert failed.returncode != 0 and (dest/'dmusic.dll').is_symlink()
    assert not (runtime/'directmusic-ready').exists()
    extractor.write_bytes(real)
    extractor.chmod(0o755)
    # A directory at a DLL pathname must be rejected, never treated as mv's target.
    (dest/'dmband.dll').mkdir()
    rejected = run()
    assert rejected.returncode != 0 and (dest/'dmusic.dll').is_symlink()
    assert list((dest/'dmband.dll').iterdir()) == []
    (dest/'dmband.dll').rmdir()
    result = run()
    assert result.returncode == 0, result.stdout + result.stderr
    assert not (dest/'dmusic.dll').is_symlink()
    assert original.read_bytes() == b'preserve original runtime'
    assert save.read_bytes() == b'preserve save'
    assert len(list((runtime/'directmusic-backups').iterdir())) == 1
    assert run('--check').returncode == 0
    # Verified prefix reuse does not require the download or an extractor.
    archive.unlink()
    extractor.unlink()
    assert run().returncode == 0
    assert not archive.exists()
    assert not list(runtime.glob('.directmusic-stage.*'))
    print('PASS: real nine-DLL hashes, extraction failure rollback, symlink-target/save preservation, backups and offline reuse')
