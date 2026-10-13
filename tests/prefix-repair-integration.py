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
    for name in ['wow64.dll','wow64cpu.dll','wow64win.dll','kernel32.dll','kernelbase.dll','ntdll.dll','wineboot.exe']:
        (builtin/name).write_text('runtime-builtin-'+name)
    builtin32=root/'runtime/wine-10.0-amd64-wow64/lib/wine/i386-windows'
    builtin32.mkdir()
    for name in ['kernel32.dll','kernelbase.dll','ntdll.dll','cmd.exe']:
        (builtin32/name).write_text('32-bit-builtin-'+name)
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
if sys.argv[2].endswith('cmd.exe'):
 (root/'command-display.txt').write_text(str(os.environ.get('DISPLAY')))
 if (root/'bad32').exists(): sys.exit(53)
 if (root/'no-marker').exists(): sys.exit(0)
 print('ST-PREFIX32-PASS')
 sys.exit(0)
if (root/'must-not-bootstrap').exists(): sys.exit(97)
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
    assert data=={'args':[str(builtin/'wineboot.exe'),'-u'],'preload':None,'display':':123','cpu':'0'},data
    (root/'fail').unlink()
    (root/'bad32').touch()
    rejected=run()
    assert rejected.returncode==1 and not (root/'runtime/prefix-32bit-verified').exists()
    assert (root/'runtime/prefix-repair-pending').exists()
    (root/'bad32').unlink()
    (root/'no-marker').touch()
    empty_success=run()
    assert empty_success.returncode==1 and not (root/'runtime/prefix-32bit-verified').exists()
    (root/'no-marker').unlink()
    completed=run()
    assert completed.returncode==0,(completed.stdout,completed.stderr)
    assert not (root/'runtime/prefix-repair-pending').exists()
    assert (root/'runtime/prefix-32bit-verified').exists()
    assert (root/'command-display.txt').read_text()=='None'
    assert (root/'prefix-wine-10.0/drive_c/windows/syswow64/kernel32.dll').read_text()=='32-bit-builtin-kernel32.dll'
    (root/'bootstrap.json').unlink()
    again=run()
    assert again.returncode==0 and not (root/'bootstrap.json').exists()
    assert save.read_text()=='preserve-save'
    (root/'runtime/prefix-32bit-verified').unlink()
    (root/'prefix-wine-10.0/system.reg').write_text('existing registry')
    (root/'prefix-wine-10.0/user.reg').write_text('existing registry')
    (root/'must-not-bootstrap').touch()
    existing=run()
    assert existing.returncode==0,(existing.stdout,existing.stderr)
    assert not (root/'bootstrap.json').exists()
    assert (root/'runtime/prefix-32bit-verified').exists()
print('PASS: missing DLL restoration, existing DLL/save preservation, clean bootstrap, retry after failure, subsequent reuse')
