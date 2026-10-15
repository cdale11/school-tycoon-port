"""Integrity checks remain enabled under python -O; failed builds preserve release."""
from pathlib import Path
import subprocess,tempfile,sys
repo=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='st-build-integrity-') as d:
 root=Path(d);box=root/'box64';box.write_bytes(b'untrusted runtime')
 output=root/'out';output.mkdir();release=output/'school-tycoon-handheld.tar.gz';release.write_bytes(b'prior release')
 command=[sys.executable,'-O',str(repo/'tools/build-handheld.py'),'--cab-runtime',str(root/'unused-cab.tar.gz'),'--box64',str(box),'--runtime-base',str(repo/'releases/school-tycoon-play.tar.gz'),'--cc','/bin/false','--strip','/bin/false','--sysroot',str(root),'--mingw','/bin/false','--output-dir',str(output)]
 result=subprocess.run(command,capture_output=True,text=True,timeout=10)
 assert result.returncode!=0 and 'Unexpected upstream Box64 build' in result.stderr
 assert release.read_bytes()==b'prior release'
 assert not list(output.glob('.handheld-*'))
 print('PASS: python -O cannot bypass runtime checksum, previous archive preserved on rejection')
