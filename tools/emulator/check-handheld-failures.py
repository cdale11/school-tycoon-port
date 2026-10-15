#!/usr/bin/env python3
"""Required contract/fault checks alongside software-rendered VM gameplay.
These tests model observed failures, not proprietary Mali hardware behavior.
"""
import json,subprocess,sys
from pathlib import Path
repo=Path(__file__).resolve().parents[2]
profile=json.loads(Path(__file__).with_name('rg34xxsp.json').read_text())
for name in profile['required_fault_checks']:
 subprocess.run([sys.executable,str(repo/'tests'/name)],check=True,cwd=repo)
print('PASS: handheld failure contracts; Mali rendering and physical controls remain unverified')
