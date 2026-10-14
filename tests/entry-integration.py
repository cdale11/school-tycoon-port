"""Check update-entry log preservation and damaged-package rejection."""
from pathlib import Path
import os
import subprocess
import tempfile
repo=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='st-entry-') as directory:
 root=Path(directory); game=root/'school-tycoon';logs=game/'logs';logs.mkdir(parents=True)
 entry=root/'School Tycoon.sh';entry.write_bytes((repo/'packaging/School Tycoon.sh').read_bytes())
 (game/'gamedata').mkdir();(game/'gamedata/SchoolTycoon.exe').write_bytes(b'private asset')
 saved=game/'gamedata/save.sts';saved.write_bytes(b'preserve saved school')
 def script(path,text):path.parent.mkdir(parents=True,exist_ok=True);path.write_text(text);path.chmod(0o755)
 mocks=root/'bin';script(mocks/'uname','#!/bin/sh\necho aarch64\n')
 script(game/'prepare-wine.sh','#!/bin/sh\nexit 77\n')
 (logs/'game-wine.txt').write_bytes(b'a'*400000+b'last crash')
 (logs/'launcher.txt').write_text('preceding launcher')
 (game/'debug.txt').write_text('1\n')
 env=dict(os.environ,PATH=str(mocks)+':'+os.environ['PATH'])
 result=subprocess.run(['sh',str(entry)],env=env,timeout=10)
 assert result.returncode==77
 prior=(logs/'game-wine.txt.previous').read_bytes()
 assert len(prior)==262144 and prior.endswith(b'last crash')
 assert (logs/'launcher.txt.previous').read_text()=='preceding launcher'
 assert 'Debug: 1' in (logs/'system.txt').read_text()
 assert saved.read_bytes()==b'preserve saved school'
 # A damaged runtime must stop before invoking the installer or game.
 (game/'SHA256SUMS').write_text('0'*64+'  school-tycoon/prepare-wine.sh\n')
 result=subprocess.run(['sh',str(entry)],env=env,timeout=10)
 assert result.returncode==1
 assert 'Package verification failed' in (logs/'launcher.txt').read_text()
 assert saved.read_bytes()==b'preserve saved school'
 print('PASS: bounded preceding logs, debug report, saved-file preservation, damaged-package gate')
