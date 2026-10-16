"""Verify actual bounded capture and failure draining, independent of game assets."""
from pathlib import Path
import subprocess
import tempfile
repo=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='st-log-test-') as d:
    root=Path(d); exe=root/'st-log'
    subprocess.run(['cc','-Wall','-Wextra','-Werror','-O2',str(repo/'tools/st-log.c'),'-o',str(exe)],check=True)
    output=root/'output.txt'; data=bytes(range(256))*5000
    subprocess.run([str(exe),str(output)],input=data,check=True,timeout=10)
    previous=Path(str(output)+'.previous')
    assert output.stat().st_size<=262144 and previous.stat().st_size<=262144
    assert data.endswith(previous.read_bytes()+output.read_bytes())
    beginning=root/'startup.txt'
    subprocess.run([str(exe),str(output),str(beginning)],input=data,check=True,timeout=10)
    assert beginning.read_bytes()==data[:65536]
    assert data.endswith(previous.read_bytes()+output.read_bytes())
    failed_head=subprocess.run([str(exe),str(output),str(root/'missing/head')],input=data,capture_output=True,timeout=10)
    assert failed_head.returncode==1 and data.endswith(previous.read_bytes()+output.read_bytes())
    failed=subprocess.run([str(exe),str(root/'missing/output')],input=data,capture_output=True,timeout=10)
    assert failed.returncode==1 and b'drained' in failed.stderr
    print('PASS: bounded log rotation, exact tail, storage-failure draining')
