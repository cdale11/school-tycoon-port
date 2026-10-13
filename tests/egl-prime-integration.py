"""Verify early EGL initialization plus real GLES pixel rendering.
Usage: python3 tests/egl-prime-integration.py primer.so native-egl-probe.host
Set CRUSTY_LIBEGL, LD_LIBRARY_PATH and EGL_PLATFORM for the host driver.
"""
from pathlib import Path
import os
import shutil
import subprocess
import sys
import tempfile

primer,probe=(str(Path(a).resolve()) for a in sys.argv[1:3])
with tempfile.TemporaryDirectory(prefix='st-egl-') as tmp:
    executable=Path(tmp)/'SchoolTycoon.exe'
    shutil.copy2(probe,executable)
    env=dict(os.environ,LD_PRELOAD=primer,ST_EGL_TARGET='SchoolTycoon.exe')
    rendered=subprocess.run([str(executable)],env=env,capture_output=True,text=True,timeout=20)
    assert rendered.returncode==0,(rendered.stdout,rendered.stderr)
    assert 'initialized before Windows mappings' in rendered.stderr,rendered.stderr
    assert 'PASS' in rendered.stdout,rendered.stdout
    invalid=dict(env,CRUSTY_LIBEGL='/nonexistent/native-EGL.so')
    failed=subprocess.run([str(executable)],env=invalid,capture_output=True,text=True,timeout=20)
    assert failed.returncode==1 and 'early EGL load failed' in failed.stderr
    env['ST_EGL_TARGET']='different.exe' 
    skipped=subprocess.run([str(executable)],env=env,capture_output=True,text=True,timeout=20)
    assert skipped.returncode==0
    assert 'initialized before Windows mappings' not in skipped.stderr
    # Exercise Crusty's exported loader API using an explicit full native path.
    test_loader='''import ctypes as c,os,sys
lib=c.CDLL(sys.argv[1]); f=lib.load_lib_local
f.argtypes=[c.c_char_p,c.c_char_p]; f.restype=c.c_void_p
assert f(b"nonexistent-fallback.so",b"CRUSTY_LIBEGL")
'''
    loaded=subprocess.run([sys.executable,'-c',test_loader,primer],env=env,capture_output=True,text=True,timeout=20)
    assert loaded.returncode==0,(loaded.stdout,loaded.stderr)
    assert 'native CRUSTY_LIBEGL loaded from '+env['CRUSTY_LIBEGL'] in loaded.stderr
    print('PASS: early native EGL, real GLES pixel readback, target filtering, direct native loader')
