"""Guard the no-firmware-mutation contract across every shipped shell script.
Behavioral entry/session tests additionally exercise normal and failed shutdown.
This source guard intentionally requires review when firmware helpers are added.
"""
from pathlib import Path
import re, shlex
r=Path(__file__).resolve().parents[1]
# Keys may appear in docs/tests, but no executable launcher path may reference
# them: even a well-meant 'restore original settings' can overwrite user edits.
files=list((r/'packaging').glob('*.sh'))+list((r/'tools').glob('*.sh'))
for p in files:
 code='\n'.join(s for s in p.read_text().splitlines() if not s.lstrip().startswith('#'))
 tokens=list(shlex.shlex(code,posix=True,punctuation_chars=True))
 assert not set(tokens)&{'reboot','poweroff','shutdown','killall','pkill'},p
 for forbidden in [r'global\.(?:quickresume|bootgame\.(?:cmd|path))',
                   r'batocera-settings',r'batocera\.conf',
                   r'(?:source|\.)\s+[^\n]*control\.txt']:
  assert not re.search(forbidden,code,re.M),(p,forbidden)
print(f'PASS: {len(files)} shell entry/startup/shutdown/diagnostic scripts contain no firmware-setting, reboot or broad process-kill operations')
