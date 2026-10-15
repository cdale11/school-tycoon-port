"""Interrupted downloads exit and remove staging without touching cached/user data."""
from pathlib import Path
import os,signal,subprocess,tempfile,time
r=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='st-install-signal-') as d:
 p=Path(d);b=p/'bin';b.mkdir()
 (p/'prepare-wine.sh').write_bytes((r/'tools/prepare-wine.sh').read_bytes())
 def script(name,text):
  f=b/name;f.write_text(text);f.chmod(0o755)
 script('uname','#!/bin/sh\necho aarch64\n')
 script('df','#!/bin/sh\necho "disk 4000000 1 3000000 1% /"\n')
 script('curl',f'#!/bin/sh\nprintf "%s\\n" "$@" > "{p}/curl-args"\nexec sleep 60\n')
 env=dict(os.environ,PATH=str(b)+':'+os.environ['PATH'])
 proc=subprocess.Popen(['sh',str(p/'prepare-wine.sh')],env=env,start_new_session=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
 try:
  deadline=time.monotonic()+5
  while not (p/'curl-args').exists() and time.monotonic()<deadline:time.sleep(.02)
  assert (p/'curl-args').exists()
  args=(p/'curl-args').read_text().splitlines()
  assert args[args.index('--max-time')+1]=='600'
  assert args[args.index('--connect-timeout')+1]=='30'
  os.killpg(proc.pid,signal.SIGTERM)
  assert proc.wait(timeout=5)==143
  assert not list((p/'runtime').glob('.wine-stage.*'))
  assert not (p/'runtime/wine-10.0-amd64-wow64').exists()
 finally:
  if proc.poll() is None:os.killpg(proc.pid,signal.SIGKILL);proc.wait()
 print('PASS: download bounded; cancellation exits 143 and removes only owned staging')
