#!/usr/bin/env python3
"""Attach to this task's private QEMU serial socket without restarting the VM."""
import argparse
import os
from pathlib import Path
import select
import signal
import socket
import sys
import termios
import tty

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('socket', type=Path)
args = parser.parse_args()
previous = termios.tcgetattr(0) if os.isatty(0) else None
connection = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
connection.connect(str(args.socket.resolve()))
signal.signal(signal.SIGTERM, lambda *_: sys.exit(0))
try:
    if previous is not None:
        tty.setraw(0)
    while True:
        ready, _, _ = select.select([connection, 0], [], [])
        if connection in ready:
            data = connection.recv(65536)
            if not data:
                break
            os.write(1, data)
        if 0 in ready:
            data = os.read(0, 65536)
            if not data:
                break
            connection.sendall(data)
finally:
    connection.close()
    if previous is not None:
        termios.tcsetattr(0, termios.TCSADRAIN, previous)
