"""Exercise interrupted-prefix recovery with a stand-in Wine bootstrap.
Checks restoration of missing files, preservation, clean bootstrap env, retries.
"""
from pathlib import Path
import json
import os
import shutil
import subprocess
import tempfile

repo=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='st-prefix-') as tmp:
    root=Path(tmp)
    shutil.copy2(repo/'tools/prepare-prefix.sh',root/'prepare-prefix.sh')
    builtin=root/'runtime/wine-10.0-amd64-wow64/lib/wine/x86_64-windows'
    builtin.mkdir(parents=True)
    for name in ['wow64.dll','wow64cpu.dll','wow64win.dll']:
        (builtin/name).write_text('runtime-builtin-'+name)
    system32=root/'prefix-wine-10.0/drive_c/windows/system32'
    system32.mkdir(parents=True)
    (system32/'wow64.dll').symlink_to(root/'removed-wow64.dll')
    (system32/'wow64cpu.dll').write_text('keep-existing-dll')
    save=root/'prefix-wine-10.0/drive_c/save.dat'
    save.write_text('preserve-save')
    wrapper=root/'box64'
    wrapper.write_text('''#!/usr/bin/python3
import sys,os,json
from pathlib import Path
root=Path(__file__).resolve().parent
if sys.argv[1].endswith('wineserver'): sys.exit(0)
(root/'bootstrap.json').write_text(json.dumps({'args':sys.argv[2:], 'preload':os.environ.get('LD_PRELOAD'), 'display':os.environ.get('DISPLAY'), 'cpu':os.environ.get('BOX64_DYNAREC')}))
if (root/'fail').exists(): sys.exit(7)
''')
    wrapper.chmod(0o755)
    (root/'fail').touch()
    def run():
        return subprocess.run(['sh',str(root/'prepare-prefix.sh')],env=dict(os.environ,DISPLAY=':123'),capture_output=True,text=True,timeout=20)
    failed=run()
    assert failed.returncode==7,(failed.stdout,failed.stderr)
    assert (root/'runtime/prefix-repair-pending').exists()
    assert len(list(system32.glob('wow64.dll.incomplete.*')))==1
    assert (system32/'wow64.dll').read_text()=='runtime-builtin-wow64.dll'
    assert (system32/'wow64cpu.dll').read_text()=='keep-existing-dll'
    assert save.read_text()=='preserve-save'
    data=json.loads((root/'bootstrap.json').read_text())
    assert data=={'args':[str(builtin/'wineboot.exe'),'-u'],'preload':None,'display':None,'cpu':'0'},data
    (root/'fail').unlink()
    completed=run()
    assert completed.returncode==0,(completed.stdout,completed.stderr)
    assert not (root/'runtime/prefix-repair-pending').exists()
    (root/'bootstrap.json').unlink()
    again=run()
    assert again.returncode==0 and not (root/'bootstrap.json').exists()
    assert save.read_text()=='preserve-save'
print('PASS: missing DLL restoration, existing DLL/save preservation, clean bootstrap, retry after failure, subsequent reuse')
