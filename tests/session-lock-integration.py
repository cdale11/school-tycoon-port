"""Actual kernel serialization and lock release after launcher crash/exec."""
from pathlib import Path
import subprocess,tempfile,time
repo=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='st-lock-') as d:
 root=Path(d);exe=root/'st-lock';lock=root/'lock';ready=root/'ready'
 subprocess.run(['cc','-Wall','-Wextra','-Werror','-O2',str(repo/'tools/st-lock.c'),'-o',str(exe)],check=True)
 first=subprocess.Popen([str(exe),str(lock),'sh','-c','touch "$1"; exec sleep 20','test',str(ready)])
 try:
  for _ in range(100):
   if ready.exists():break
   time.sleep(.01)
  assert ready.exists()
  blocked=subprocess.run([str(exe),str(lock),'true'],capture_output=True,timeout=5)
  assert blocked.returncode==1 and b'already' in blocked.stderr
 finally:first.kill();first.wait()
 subprocess.run([str(exe),str(lock),'true'],check=True,timeout=5)
 # A forked child must not retain its dead parent's process-owned lock.
 pidfile=root/'child'
 subprocess.run([str(exe),str(lock),'sh','-c','sleep 20 & echo $! > "$1"','test',str(pidfile)],check=True)
 try:subprocess.run([str(exe),str(lock),'true'],check=True,timeout=5)
 finally:
  import os,signal
  os.kill(int(pidfile.read_text()),signal.SIGTERM)
 print('PASS: concurrent launch rejected, exec preserves lock, crash releases lock, background child does not retain it')
