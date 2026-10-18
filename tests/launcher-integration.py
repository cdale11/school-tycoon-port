"""Exercise session orchestration with stand-in Wine/Weston on a private socket.
Checks asset cwd, graphics environment, game failure propagation and cleanup.
"""
from pathlib import Path
import json
import os
import subprocess
import socket
import tempfile

repo = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='st-session-') as tmp:
    root = Path(tmp)
    def script(path, text):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)
        path.chmod(0o755)
    source = (repo/'tools/run-game.sh').read_text()
    source = source.replace('/tmp/.X11-unix', str(root/'sockets'))
    source = source.replace('/tmp/.X0-lock', str(root/'x0-lock'))
    script(root/'run-game.sh', source)
    script(root/'media-env.sh', (repo/'tools/media-env.sh').read_text())
    wine = root/'runtime/wine-10.0-amd64-wow64/bin'
    script(wine/'wine', '#!/bin/sh\nexit 99\n')
    (root/'runtime/graphics').mkdir()
    (root/'runtime/graphics/libst-egl-prime.so').touch()
    (root/'runtime/graphics/libst-glx-config.so').touch()
    (root/'runtime/graphics/libst-gl-present.so').touch()
    (root/'gamedata').mkdir()
    (root/'gamedata/SchoolTycoon.exe').touch()
    (root/'st-desktop-ready.exe').touch()
    weston = root/'runtime/weston-0.2'
    script(weston/'wp_weston', '#!/usr/bin/python3\nimport socket,signal,time,os\np='+repr(str(root/'sockets/X0'))+'\ns=socket.socket(socket.AF_UNIX)\ns.bind(p)\ndef stop(*args):\n s.close()\n os.unlink(p)\n exit(0)\nsignal.signal(signal.SIGTERM,stop)\nwhile True: time.sleep(1)\n')
    subprocess.run(['cc','-O2',str(repo/'tools/st-log.c'),'-o',str(root/'st-log')],check=True)
    script(root/'sample-session.sh', '#!/bin/sh\nexit 0\n')
    script(root/'prepare-prefix.sh', '#!/bin/sh\n[ "$DISPLAY" = :0 ] && [ -z "${LD_PRELOAD:-}" ] || exit 98\n')
    script(root/'st-controls', '#!/bin/sh\ntrap \'echo stopped > "'+str(root/'controls-stopped')+'"; exit 1\' TERM\nwhile :; do sleep 0.1; done\n')
    script(root/'box64', '#!/usr/bin/python3\nimport os,sys,json\nfrom pathlib import Path\nr=Path('+repr(tmp)+')\nif sys.argv[-1].endswith("st-desktop-ready.exe"):\n import signal,time\n (r/"logs/desktop-ready.txt").write_text("ST-DESKTOP-READY\\n")\n signal.signal(signal.SIGTERM,lambda *a:sys.exit(0))\n while True: time.sleep(0.1)\nif sys.argv[1].endswith("wineserver"):\n (r/"wine-stopped").touch()\n sys.exit(0)\n(r/"observed.json").write_text(json.dumps({"cwd":os.getcwd(),"args":sys.argv[2:],"gles":os.environ.get("SDL_VIDEO_GL_DRIVER"),"desktop_gl":os.environ.get("BOX64_LIBGL"),"context":os.environ.get("CRUSTY_GLES"),"noarb":os.environ.get("LIBGL_NOARBPROGRAM"),"d3d_config":os.environ.get("WINE_D3D_CONFIG"),"discrete":os.environ.get("LIBGL_NOTEXARRAY"),"cpu":os.environ.get("BOX64_DYNAREC"),"mmap32":os.environ.get("BOX64_MMAP32"),"target":os.environ.get("ST_EGL_TARGET"),"preload":os.environ.get("LD_PRELOAD"),"pulse":os.environ.get("PULSE_SERVER"),"wine_debug":os.environ.get("WINEDEBUG")}))\nsys.exit(7)\n')
    mocks = root/'mock-bin'
    script(mocks/'uname', '#!/bin/sh\necho aarch64\n')
    script(mocks/'sha256sum', '#!/bin/sh\ncat >/dev/null\nexit 0\n')
    script(mocks/'unsquashfs', '#!/bin/sh\nexit 99\n')
    env = dict(os.environ, PATH=str(mocks)+':'+os.environ['PATH'])
    env['BOX64_MMAP32']='1'  # Launcher must override inherited forced native mappings.
    env.pop('ST_CPU_MODE',None)
    env.pop('PULSE_SERVER',None)
    audio_dir=root/'original-runtime/pulse';audio_dir.mkdir(parents=True)
    audio_socket=socket.socket(socket.AF_UNIX);audio_socket.bind(str(audio_dir/'native'))
    env['XDG_RUNTIME_DIR']=str(audio_dir.parent)
    result = subprocess.run(['sh',str(root/'run-game.sh'),'game'],env=env,timeout=15)
    assert result.returncode == 7, (result.returncode,(root/'logs/game.txt').read_text())
    data = json.loads((root/'observed.json').read_text())
    assert data == {'cwd':str(root/'gamedata'),'args':['./SchoolTycoon.exe'],
                    'wine_debug':'-all,err+all','gles':'/usr/lib/libGLESv2.so.2','desktop_gl':str(weston/'lib_aarch64/graphics/gl4es_glxpass/libGL.so.1'),'context':'30','noarb':'1','d3d_config':'csmt=0,shader_backend=glsl','discrete':'1','cpu':'0','mmap32':'0','target':'SchoolTycoon.exe','pulse':'unix:'+str(audio_dir/'native'),'preload':str(root/'runtime/graphics/libst-gl-present.so')+':'+str(root/'runtime/graphics/libst-glx-config.so')+':'+str(root/'runtime/graphics/libst-egl-prime.so')+':'+str(weston/'lib_aarch64/graphics/crusty_glx/libcrusty.so')}, data
    assert (root/'controls-stopped').exists()
    assert (root/'wine-stopped').exists()
    assert not list((root/'logs').glob('.pipes.*')), 'Logger FIFO directory leaked'
    # Controller shortcut while the game is live: only owned processes are stopped.
    script(root/'st-controls', '#!/bin/sh\nsleep 0.3\nexit 0\n')
    box_source=(root/'box64').read_text().replace('sys.exit(7)',
        'import signal,time\ndef stop(*args):\n (r/"game-stopped").touch()\n sys.exit(143)\nsignal.signal(signal.SIGTERM,stop)\nwhile True: time.sleep(0.1)')
    script(root/'box64',box_source)
    env['PULSE_SERVER']='unix:/explicit/audio.sock'
    result=subprocess.run(['sh',str(root/'run-game.sh'),'game'],env=env,timeout=15)
    assert result.returncode==0,(result.returncode,(root/'logs/game.txt').read_text())
    assert 'Controller exit shortcut requested exit.' in (root/'logs/game.txt').read_text()
    assert (root/'game-stopped').exists(), 'Owned game was not stopped'
    assert json.loads((root/'observed.json').read_text())['pulse']=='unix:/explicit/audio.sock'
    assert not list((root/'logs').glob('.pipes.*')), 'Shortcut leaked logger FIFOs'
    # A frozen/backwards wall clock must not disable shutdown deadlines.
    script(mocks/'date', '#!/bin/sh\nif [ "$1" = +%s ]; then echo 0; else exec /bin/date "$@"; fi\n')
    # A child ignoring TERM cannot hold cleanup forever; escalation is owned.
    script(root/'st-controls', '#!/bin/sh\ntrap "" TERM\nwhile :; do sleep 0.1; done\n')
    script(root/'box64',box_source.replace('while True: time.sleep(0.1)','sys.exit(7)'))
    result=subprocess.run(['sh',str(root/'run-game.sh'),'game'],env=env,timeout=10)
    assert result.returncode==7, 'Stubborn controller blocked failure cleanup'
    assert not list((root/'logs').glob('.pipes.*'))
    # Broken controls stop an otherwise live game instead of waiting forever.
    script(root/'st-controls', '#!/bin/sh\nsleep 0.2\nexit 1\n')
    script(root/'box64',box_source)
    result=subprocess.run(['sh',str(root/'run-game.sh'),'game'],env=env,timeout=10)
    assert result.returncode==1
    assert 'Controller helper unavailable' in (root/'logs/game.txt').read_text()
    # Reproduce the real handheld log while Wine remains alive. SDL failure
    # must stop the owned process and let the frontend run its game-end hook.
    script(root/'st-controls', '#!/bin/sh\ntrap "exit 1" TERM\nwhile :; do sleep 0.1; done\n')
    (root/'game-stopped').unlink()
    failure=box_source.replace('import signal,time\ndef stop',
        'print("Could not create SDL Window: mali-fbdev: Can\\\'t create EGL window surface",flush=True)\nimport signal,time\ndef stop')
    script(root/'box64',failure)
    result=subprocess.run(['sh',str(root/'run-game.sh'),'game'],env=env,timeout=15)
    assert result.returncode==1
    assert 'phase=graphics-failed' in (root/'logs/game.txt').read_text()
    assert (root/'game-stopped').exists()
    assert not list((root/'logs').glob('.pipes.*'))
    # Repeated runtime GPU errors must end the game, while preserving their
    # initial diagnostics even if the rotating tail later fills up.
    repeated=box_source.replace('import signal,time\ndef stop',
        'print(("GL_OUT_OF_MEMORY GL_INVALID_FRAMEBUFFER_OPERATION\\n"*16),file=sys.stderr,flush=True)\nimport signal,time\ndef stop')
    script(root/'box64',repeated)
    result=subprocess.run(['sh',str(root/'run-game.sh'),'game'],env=env,timeout=15)
    assert result.returncode==1
    assert 'phase=rendering-failed' in (root/'logs/game.txt').read_text()
    assert 'GL_OUT_OF_MEMORY' in (root/'logs/game-wine.startup.txt').read_text()
    # A desktop helper failure must stop before starting the original game.
    (root/'observed.json').unlink()
    broken=(root/'box64').read_text()
    start=broken.index('if sys.argv[-1].endswith("st-desktop-ready.exe"):')
    end=broken.index('if sys.argv[1].endswith("wineserver"):',start)
    broken=broken[:start]+'if sys.argv[-1].endswith("st-desktop-ready.exe"):\n sys.exit(3)\n'+broken[end:]
    script(root/'box64',broken)
    result=subprocess.run(['sh',str(root/'run-game.sh'),'game'],env=env,timeout=15)
    assert result.returncode==1
    assert not (root/'observed.json').exists(), 'Game started despite failed desktop readiness'
    assert 'Desktop helper exited before readiness: 3' in (root/'logs/game.txt').read_text()
    print('PASS: command/cwd/GLES/CPU, desktop readiness/failure gate, logger cleanup, controller shortcut and owned cleanup')
