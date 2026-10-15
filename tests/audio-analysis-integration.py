"""Reject silent/dither/clipped fixtures while accepting a quiet PCM tone."""
from pathlib import Path
import array
import subprocess
import sys
import tempfile
import wave
repo=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='st-pcm-') as temporary:
    path=Path(temporary)/'fixture.wav'
    for label, samples, expected in [('silence',[0]*1000,1),('dither',[-1,1]*500,1),
                                      ('tone',[-3000,3000]*500,0),('clipped',[-32768,32767]*500,1)]:
        values=array.array('h',samples)
        if sys.byteorder!='little': values.byteswap()
        with wave.open(str(path),'wb') as output:
            output.setnchannels(2); output.setsampwidth(2); output.setframerate(22050)
            output.writeframes(values.tobytes())
        result=subprocess.run([sys.executable,str(repo/'tools/emulator/analyze-audio.py'),
            str(path),'--maximum-clipped-fraction','0'],capture_output=True,text=True)
        assert result.returncode==expected,(label,result.stdout,result.stderr)
print('PASS: silence/dither/clipping rejected, generated PCM tone accepted')
