"""Private original-game menu test, starting on Main at 800x600.
Set DISPLAY, ST_MENU_GAME_PID and ST_MENU_SETTINGS for the verified game.
Pass a shared build of tools/st-controls.c. Uses SDL's virtual controller;
never bundles or writes game assets, and does not certify physical buttons.
"""
import hashlib
from pathlib import Path
import ctypes as c
import os
import sys
import threading
import time

assert hashlib.sha256((Path(os.environ['ST_MENU_SETTINGS']).parent/'SchoolTycoon.exe').read_bytes()).hexdigest() == '4c4372be0372223864a8648aacfc488fc9ed93d45c9e364609efa11f471b698a'
s = c.CDLL('libSDL2-2.0.so.0')
x = c.CDLL('libX11.so.6')

def api(lib, name, restype, *args):
    f = getattr(lib, name)
    f.restype, f.argtypes = restype, args
    return f

init = api(s, 'SDL_Init', c.c_int, c.c_uint)
attach = api(s, 'SDL_JoystickAttachVirtual', c.c_int, c.c_int, c.c_int, c.c_int, c.c_int)
openjoy = api(s, 'SDL_JoystickOpen', c.c_void_p, c.c_int)
button = api(s, 'SDL_JoystickSetVirtualButton', c.c_int, c.c_void_p, c.c_int, c.c_ubyte)
update = api(s, 'SDL_JoystickUpdate', None)
assert init(0x2000) == 0
idx = attach(1, 6, 15, 0)
assert idx >= 0
joy = openjoy(idx)
assert joy
api(x, 'XInitThreads', c.c_int)()
d = api(x, 'XOpenDisplay', c.c_void_p, c.c_char_p)(os.environ['DISPLAY'].encode())
assert d
root = api(x, 'XDefaultRootWindow', c.c_ulong, c.c_void_p)(d)
query = api(x, 'XQueryPointer', c.c_int, c.c_void_p, c.c_ulong,
            c.POINTER(c.c_ulong), c.POINTER(c.c_ulong),
            c.POINTER(c.c_int), c.POINTER(c.c_int),
            c.POINTER(c.c_int), c.POINTER(c.c_int), c.POINTER(c.c_uint))
def position():
    a,b = c.c_ulong(),c.c_ulong()
    rx,ry,wx,wy = (c.c_int() for _ in range(4))
    mask = c.c_uint()
    assert query(d,root,c.byref(a),c.byref(b),c.byref(rx),c.byref(ry),c.byref(wx),c.byref(wy),c.byref(mask))
    return rx.value,ry.value,mask.value

def setbutton(n, val):
    assert button(joy,n,val) == 0
    update()
    time.sleep(.15)

helper = c.CDLL(os.path.abspath(sys.argv[1]))
result = []
thread = threading.Thread(target=lambda: result.append(helper.main()), daemon=True)
thread.start()
time.sleep(.2)
try:
    setbutton(12,1)
    setbutton(12,0)
    assert position()[:2]==(400,187), position()
    setbutton(12,1)
    setbutton(12,0)
    assert position()[:2]==(400,286), position()
    setbutton(0,1)
    time.sleep(5)
    setbutton(0,0)
    print('ST-MENU-NAVIGATION: selected Settings; pointer', position(), flush=True)
    fd=os.open('/proc/'+os.environ['ST_MENU_GAME_PID']+'/mem',os.O_RDONLY)
    def u32(address):
        return int.from_bytes(os.pread(fd,4,address),'little')
    deadline=time.monotonic()+90
    while time.monotonic()<deadline:
        manager=u32(0x6548c4)
        if u32(u32(u32(manager+0xa0)))==0x6003f4:
            break
        time.sleep(.25)
    else:
        raise AssertionError('Settings screen did not open')
    os.close(fd)
finally:
    for n in range(15):
        assert button(joy,n,0)==0
    update()
    setbutton(5,1)
    setbutton(6,1)
    thread.join(2)
assert result==[0], result
assert not position()[2] & (256|1024)
print('ST-MENU-CONTROLS-PASS',flush=True)
