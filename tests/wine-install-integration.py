"""Exercise recovery with the real, pinned Wine archive and no network access.
Usage: python3 tests/wine-install-integration.py /path/to/wine-10.0-amd64-wow64.tar.xz
"""
from pathlib import Path
import os
import shutil
import subprocess
import sys
import tempfile

repo=Path(__file__).resolve().parents[1]
archive=Path(sys.argv[1]).resolve()
name='wine-10.0-amd64-wow64'
with tempfile.TemporaryDirectory(prefix='st-installer-') as tmp:
    base=Path(tmp)
    root=base/'school-tycoon'
    runtime=root/'runtime'
    runtime.mkdir(parents=True)
    shutil.copy2(repo/'tools/prepare-wine.sh',root/'prepare-wine.sh')
    mocks=base/'bin'
    mocks.mkdir()
    def executable(path,text):
        path.write_text(text)
        path.chmod(0o755)
    executable(mocks/'uname','#!/bin/sh\necho aarch64\n')
    executable(mocks/'curl','#!/bin/sh\necho "Unexpected network download" >&2\nexit 97\n')
    env=dict(os.environ,PATH=str(mocks)+':'+os.environ['PATH'])
    def install():
        return subprocess.run(['sh',str(root/'prepare-wine.sh')],env=env,capture_output=True,text=True,timeout=120)
    # A real file at the destination must be preserved and rejected early.
    (runtime/name).write_text('preserve this user file')
    failed=install()
    assert failed.returncode==1 and 'not a directory' in failed.stderr,(failed.stdout,failed.stderr)
    assert (runtime/name).read_text()=='preserve this user file'
    (runtime/name).unlink()
    (runtime/name).symlink_to(base/'deleted-probe/runtime'/name)
    (runtime/'downloads').mkdir()
    shutil.copyfile(archive,runtime/'downloads'/f'{name}.tar.xz')
    # Extraction failure must keep the verified archive for retry.
    executable(mocks/'tar','#!/bin/sh\nexit 7\n')
    interrupted=install()
    assert interrupted.returncode==1 and 'retained' in interrupted.stderr,(interrupted.stdout,interrupted.stderr)
    assert not (runtime/name).is_symlink()
    assert (runtime/'downloads'/f'{name}.tar.xz').is_file()
    (mocks/'tar').unlink()
    recovered=install()
    assert recovered.returncode==0,(recovered.stdout,recovered.stderr)
    assert 'Reusing the cached' in recovered.stdout
    assert (runtime/name/'bin/wine').is_file()
    assert (runtime/name/'lib/wine/i386-windows/cmd.exe').is_file()
    again=install()
    assert again.returncode==0 and 'already exists' in again.stdout,(again.stdout,again.stderr)
    # The Ports entry must remove stale runtime links and preserve a stale prefix
    # link while reusing a valid probe installation. Stub only the game session.
    ports=base/'ports'
    ports.mkdir()
    game=ports/'school-tycoon'
    (game/'runtime').mkdir(parents=True)
    (game/'gamedata').mkdir()
    (game/'gamedata/SchoolTycoon.exe').touch()
    for n in (name,'weston-0.2'):
        (game/'runtime'/n).symlink_to(base/'removed'/n)
    missing=base/'old-prefix'
    (game/'prefix-wine-10.0').symlink_to(missing)
    probe=ports/'school-tycoon-runtime-probe/runtime'
    probe.mkdir(parents=True)
    (probe/name).symlink_to(runtime/name)
    (probe/'weston-0.2').mkdir()
    shutil.copy2(repo/'packaging/School Tycoon.sh',ports/'School Tycoon.sh')
    shutil.copy2(repo/'tools/prepare-wine.sh',game/'prepare-wine.sh')
    executable(game/'prepare-prefix.sh','#!/bin/sh\necho prefix-setup-reached\n')
    executable(game/'run-game.sh','#!/bin/sh\necho game-session-reached\n')
    result=subprocess.run(['sh',str(ports/'School Tycoon.sh')],env=env,timeout=20)
    assert result.returncode==0
    log=(game/'logs/launcher.txt').read_text()
    assert 'game-session-reached' in log and 'Downloading' not in log,log
    backups=list(game.glob('prefix-wine-10.0.missing-target.*'))
    assert len(backups)==1 and backups[0].is_symlink()
    assert os.readlink(backups[0])==str(missing)
print('PASS: stale links, file preservation, extraction failure cache, real Wine install, repeat launch, prefix-link preservation')
