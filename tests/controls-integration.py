"""Run against an existing X display: python3 tests/controls-integration.py helper.so.
Build helper.so with cc -shared -fPIC tools/st-controls.c -ldl -o helper.so.
Uses a real SDL virtual controller and queries the real X pointer/button state.
"""
import ctypes as c
import os
import sys
import threading
import time

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
p = position()
setbutton(14,1)
setbutton(14,0)
q = position()
assert q[0] > p[0], (p,q)
setbutton(0,1)
assert position()[2] & 256, position()
setbutton(0,0)
assert not position()[2] & 256
setbutton(1,1)
assert position()[2] & 1024
setbutton(1,0)
setbutton(6,1)
assert thread.is_alive(), 'Start alone exited the helper'
setbutton(6,0)
setbutton(1,1)  # Exit must release an already-held mouse button.
exit_button = int(sys.argv[2]) if len(sys.argv) > 2 else 5
setbutton(exit_button,1)
setbutton(6,1)
thread.join(2)
assert result == [0], result
assert not position()[2] & (256|1024)
print(f'PASS: movement, clicks, Start alone, held-button release, exit button {exit_button}+Start')
