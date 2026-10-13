"""Exercise session orchestration with stand-in Wine/Weston on a private socket.
Checks asset cwd, graphics environment, game failure propagation and cleanup.
"""
from pathlib import Path
import json
import os
import subprocess
import tempfile

repo = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='st-session-') as tmp:
    root = Path(tmp)
    def script(path, text):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)
        path.chmod(0o755)
    source = (repo/'tools/test-window.sh').read_text()
    source = source.replace('/tmp/.X11-unix', str(root/'sockets'))
    source = source.replace('/tmp/.X0-lock', str(root/'x0-lock'))
    script(root/'run-game.sh', source)
    wine = root/'runtime/wine-10.0-amd64-wow64/bin'
    script(wine/'wine', '#!/bin/sh\nexit 99\n')
    (root/'runtime/graphics').mkdir()
    (root/'runtime/graphics/libst-egl-prime.so').touch()
    (root/'gamedata').mkdir()
    (root/'gamedata/SchoolTycoon.exe').touch()
    weston = root/'runtime/weston-0.2'
    script(weston/'wp_weston', '#!/usr/bin/python3\nimport socket,signal,time\ns=socket.socket(socket.AF_UNIX)\ns.bind('+repr(str(root/'sockets/X0'))+')\nsignal.signal(signal.SIGTERM,lambda *a:exit(0))\nwhile True: time.sleep(1)\n')
    script(root/'prepare-prefix.sh', '#!/bin/sh\n[ "$DISPLAY" = :0 ] && [ -z "${LD_PRELOAD:-}" ] || exit 98\n')
    script(root/'st-controls', '#!/bin/sh\ntrap \'echo stopped > "'+str(root/'controls-stopped')+'"; exit 1\' TERM\nwhile :; do sleep 0.1; done\n')
    script(root/'box64', '#!/usr/bin/python3\nimport os,sys,json\nfrom pathlib import Path\nr=Path('+repr(tmp)+')\nif sys.argv[1].endswith("wineserver"):\n (r/"wine-stopped").touch()\n sys.exit(0)\n(r/"observed.json").write_text(json.dumps({"cwd":os.getcwd(),"args":sys.argv[2:],"gles":os.environ.get("SDL_VIDEO_GL_DRIVER"),"cpu":os.environ.get("BOX64_DYNAREC"),"target":os.environ.get("ST_EGL_TARGET"),"preload":os.environ.get("LD_PRELOAD")}))\nsys.exit(7)\n')
    mocks = root/'mock-bin'
    script(mocks/'uname', '#!/bin/sh\necho aarch64\n')
    script(mocks/'sha256sum', '#!/bin/sh\ncat >/dev/null\nexit 0\n')
    script(mocks/'unsquashfs', '#!/bin/sh\nexit 99\n')
    env = dict(os.environ, PATH=str(mocks)+':'+os.environ['PATH'])
    env.pop('ST_CPU_MODE',None)
    result = subprocess.run(['sh',str(root/'run-game.sh'),'game'],env=env,timeout=15)
    assert result.returncode == 7, (result.returncode,(root/'logs/game.txt').read_text())
    data = json.loads((root/'observed.json').read_text())
    assert data == {'cwd':str(root/'gamedata'),'args':['./SchoolTycoon.exe'],
                    'gles':'/usr/lib/libGLESv2.so.2','cpu':'0','target':'SchoolTycoon.exe','preload':str(root/'runtime/graphics/libst-egl-prime.so')+':'+str(weston/'lib_aarch64/graphics/crusty_glx/libcrusty.so')}, data
    assert (root/'controls-stopped').exists()
    assert (root/'wine-stopped').exists()
    print('PASS: original-game command, asset cwd, GLES selection, CPU baseline, exit status, cleanup')
