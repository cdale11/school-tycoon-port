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
    shutil.copy2(repo/'tools/media-env.sh',root/'media-env.sh')
    (root/'prepare-directmusic.sh').write_text('#!/bin/sh\nif [ "${1:-}" != --check ]; then touch "$(dirname "$0")/runtime/directmusic-ready"; fi\nexit 0\n')
    builtin=root/'runtime/wine-10.0-amd64-wow64/lib/wine/x86_64-windows'
    builtin.mkdir(parents=True)
    for name in ['wow64.dll','wow64cpu.dll','wow64win.dll','kernel32.dll','kernelbase.dll','ntdll.dll','wineboot.exe']:
        (builtin/name).write_text('runtime-builtin-'+name)
    builtin32=root/'runtime/wine-10.0-amd64-wow64/lib/wine/i386-windows'
    builtin32.mkdir()
    (root/'st-media-ready.exe').touch()
    for name in ['kernel32.dll','kernelbase.dll','ntdll.dll','cmd.exe','dmloader.dll','dmime.dll','dmusic.dll','dmsynth.dll','dsound.dll','mmdevapi.dll','dmband.dll','dmcompos.dll','dmstyle.dll','dmscript.dll','dswave.dll','vbscript.dll','msadp32.acm']:
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
if sys.argv[2].endswith('st-media-ready.exe'):
 if (root/'bad-media').exists(): sys.exit(1)
 if not (root/'no-media-marker').exists(): print('ST-PREFIX-MEDIA-PASS')
 sys.exit(0)
if sys.argv[2].endswith('cmd.exe'):
 (root/'command-display.txt').write_text(str(os.environ.get('DISPLAY')))
 if (root/'bad32').exists(): sys.exit(53)
 if (root/'no-marker').exists(): sys.exit(0)
 print('ST-PREFIX32-PASS')
 sys.exit(0)
if (root/'must-not-bootstrap').exists(): sys.exit(97)
(root/'bootstrap.json').write_text(json.dumps({'args':sys.argv[2:], 'preload':os.environ.get('LD_PRELOAD'), 'display':os.environ.get('DISPLAY'), 'cpu':os.environ.get('BOX64_DYNAREC'),'overrides':os.environ.get('WINEDLLOVERRIDES')}))
if (root/'fail').exists(): sys.exit(7)
(root/'prefix-wine-10.0/system.reg').write_text('created registry')
(root/'prefix-wine-10.0/user.reg').write_text('created registry')
''')
    wrapper.chmod(0o755)
    (root/'fail').touch()
    def run(**overrides):
        return subprocess.run(['sh',str(root/'prepare-prefix.sh')],env=dict(os.environ,DISPLAY=':123',**overrides),capture_output=True,text=True,timeout=20)
    failed=run()
    assert failed.returncode==7,(failed.stdout,failed.stderr)
    assert (root/'runtime/prefix-repair-pending').exists()
    assert len(list(system32.glob('wow64.dll.incomplete.*')))==1
    assert (system32/'wow64.dll').read_text()=='runtime-builtin-wow64.dll'
    assert (system32/'wow64cpu.dll').read_text()=='keep-existing-dll'
    assert save.read_text()=='preserve-save'
    data=json.loads((root/'bootstrap.json').read_text())
    assert data=={'args':[str(builtin/'wineboot.exe'),'--init'],'preload':None,'display':':123','cpu':'0','overrides':'dmband,dmcompos,dmime,dmloader,dmscript,dmstyle,dmsynth,dmusic,dsound,dswave=b;winewayland.drv=;iexplore.exe=;winemenubuilder.exe,mscoree,mshtml='},data
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
    (root/'bad-media').touch()
    assert run().returncode==1 and not (root/'runtime/prefix-media-verified').exists()
    (root/'bad-media').unlink()
    (root/'no-media-marker').touch()
    assert run().returncode==1 and not (root/'runtime/prefix-media-verified').exists()
    (root/'no-media-marker').unlink()
    completed=run()
    assert completed.returncode==0,(completed.stdout,completed.stderr)
    assert not (root/'runtime/prefix-repair-pending').exists()
    assert (root/'runtime/prefix-32bit-verified').exists()
    assert (root/'command-display.txt').read_text()=='None'
    assert (root/'prefix-wine-10.0/drive_c/windows/syswow64/kernel32.dll').read_text()=='32-bit-builtin-kernel32.dll'
    (root/'bootstrap.json').unlink()
    (root/'bad-media').touch()  # Verified unchanged helper need not run again.
    again=run()
    (root/'bad-media').unlink()
    assert again.returncode==0 and not (root/'bootstrap.json').exists()
    assert save.read_text()=='preserve-save'
    # Updating the owned helper invalidates the media marker automatically.
    (root/'st-media-ready.exe').write_text('new-helper-build')
    (root/'bad-media').touch()
    assert run().returncode==1, 'Media helper change incorrectly reused old readiness'
    (root/'bad-media').unlink()
    assert run().returncode==0
    # A missing codec must invalidate a previously verified prefix too.
    codec=root/'prefix-wine-10.0/drive_c/windows/syswow64/msadp32.acm'
    assert codec.read_text()=='32-bit-builtin-msadp32.acm'
    codec.unlink()
    result=run()
    assert result.returncode==0 and codec.read_text()=='32-bit-builtin-msadp32.acm'
    # Missing PE32 audio modules trigger restoration and re-registration.
    (root/'prefix-wine-10.0/drive_c/windows/syswow64/mmdevapi.dll').unlink()
    assert run().returncode==0
    assert (root/'prefix-wine-10.0/drive_c/windows/syswow64/mmdevapi.dll').is_file()
    (root/'runtime/prefix-32bit-verified').unlink()
    (root/'prefix-wine-10.0/system.reg').write_text('existing registry')
    (root/'prefix-wine-10.0/user.reg').write_text('existing registry')
    (root/'must-not-bootstrap').touch()
    existing=run()
    assert existing.returncode==0,(existing.stdout,existing.stderr)
    assert not (root/'bootstrap.json').exists()
    assert (root/'runtime/prefix-32bit-verified').exists()
    assert not (root/'runtime/directmusic-ready').exists(), 'Default run installed experimental media'
    (root/'experimental-directmusic.txt').write_text('1')
    assert run().returncode==0
    assert (root/'runtime/directmusic-ready').exists()
    assert (root/'runtime/prefix-media-verified').read_text().strip().endswith(':native')
    (root/'experimental-directmusic.txt').unlink()
    assert run().returncode==0
    assert (root/'runtime/prefix-media-verified').read_text().strip().endswith(':builtin')
    (root/'runtime/directmusic-enabled').write_text('1')
    assert run().returncode==0
    assert (root/'runtime/prefix-media-verified').read_text().strip().endswith(':native')
    assert run(ST_CPU_MODE='interpreter').returncode==0
    assert (root/'runtime/prefix-media-verified').read_text().strip().endswith(':builtin')
    (root/'audio-mode.txt').write_text('builtin')
    assert run().returncode==0
    assert (root/'runtime/prefix-media-verified').read_text().strip().endswith(':builtin')
    (root/'runtime/directmusic-enabled').unlink()
    (root/'audio-mode.txt').unlink()
    (root/'prepare-directmusic.sh').unlink()
    assert run().returncode==0, 'Default package requires experimental installer'
    (root/'experimental-directmusic.txt').write_text('1')
    assert run().returncode!=0, 'Developer experiment started without its installer'
print('PASS: missing DLL restoration, existing DLL/save preservation, clean bootstrap, CPU/media failure gates, retry and verified reuse')
