#!/usr/bin/env python3
"""Build the asset-free handheld archive from pinned runtime inputs and own sources."""
import argparse
import gzip
import hashlib
import io
import os
from pathlib import Path
import shutil
import subprocess
import tarfile
import tempfile
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--box64',type=Path,required=True)
p.add_argument('--experimental-directmusic',action='store_true',help='include the failed native-media developer experiment, never enabled by default')
p.add_argument('--cab-runtime',type=Path,help='verified Debian ARM64 CAB input, only for --experimental-directmusic')
p.add_argument('--runtime-base',type=Path,required=True,help='previous public play archive: licenses and libgcc only')
p.add_argument('--cc',required=True)
p.add_argument('--sysroot',required=True)
p.add_argument('--strip',required=True)
p.add_argument('--mingw',required=True,help='PE32 MinGW compiler')
p.add_argument('--output-dir',type=Path,default=Path('releases'))
a=p.parse_args(); repo=Path(__file__).resolve().parents[1]
sha=lambda b:hashlib.sha256(b).hexdigest()
expected='fdf6581debf39fcdea490f9a36e7b7578b2687531ed2dc2e910ddeed09f51de8'
if sha(a.box64.read_bytes())!=expected:raise SystemExit('Unexpected upstream Box64 build')
if a.experimental_directmusic:
 if not a.cab_runtime:raise SystemExit('--experimental-directmusic requires --cab-runtime')
 if sha(a.cab_runtime.read_bytes())!='5cf7f9a7f452df470c6e7f9aa917b416a21d4cee37c8a57b18d567c28d26e510':raise SystemExit('Unexpected cabinet runtime bundle')
elif a.cab_runtime:raise SystemExit('--cab-runtime is only used by --experimental-directmusic')
with tempfile.TemporaryDirectory(prefix='st-build-') as temporary:
 root=Path(temporary); game=root/'school-tycoon'; graphics=game/'runtime/graphics';graphics.mkdir(parents=True)
 needed=['BOX64-LICENSE','runtime/x86_64/GCC-COPYRIGHT','runtime/x86_64/PROVENANCE.txt','runtime/x86_64/GPL-3','runtime/x86_64/libgcc_s.so.1']
 with tarfile.open(a.runtime_base) as base:
  for name in needed:
   member=base.getmember('school-tycoon/'+name)
   if not member.isfile() or member.size>2097152:raise SystemExit('Invalid runtime input: '+name)
   dest=game/name;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(base.extractfile(member).read())
 if sha((game/'runtime/x86_64/libgcc_s.so.1').read_bytes())!='30c61ab012a4241bed033725a09b61f5fdd3bb7df95ee852d0b096520524c7af':raise SystemExit('Unexpected libgcc support library')
 if a.experimental_directmusic:
  cabinet=game/'runtime/cabextract';cabinet.mkdir()
  cab_files=['cabextract','libmspack.so.0','CABEXTRACT-COPYRIGHT','LIBMSPACK-COPYRIGHT','GPL-3','LGPL-2.1','PROVENANCE.txt','sources/cabextract_1.9-3.dsc','sources/cabextract_1.9.orig.tar.gz','sources/cabextract_1.9-3.debian.tar.xz','sources/libmspack_0.11-1.dsc','sources/libmspack_0.11.orig.tar.gz','sources/libmspack_0.11-1.debian.tar.xz']
  with tarfile.open(a.cab_runtime) as source:
   for name in cab_files:
    member=source.getmember(name)
    if not member.isfile() or member.size>2097152:raise SystemExit('Invalid cabinet runtime member: '+name)
    dest=cabinet/name;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(source.extractfile(member).read())
 shutil.copyfile(a.box64,game/'box64')
 flags=[a.cc,'--sysroot='+a.sysroot,'-Wall','-Wextra','-Werror','-O2','-mtune=cortex-a53']
 def compile(output,sources,shared=False):
  command=flags+(['-fPIC','-shared'] if shared else [])+[str(repo/'tools'/s) for s in sources]+['-ldl','-pthread','-o',str(output)]
  subprocess.run(command,check=True)
 compile(graphics/'libst-gl-present.so',['st-gl-present.c','st-gl-shader.c','st-gl-uniform.c','st-gl-source.c'],True)
 compile(graphics/'libst-glx-config.so',['st-glx-config.c'],True)
 compile(graphics/'libst-egl-prime.so',['st-egl-prime.c','st-gpu-warmup.c'],True)
 compile(game/'st-controls',['st-controls.c'])
 compile(game/'st-log',['st-log.c'])
 compile(game/'st-lock',['st-lock.c'])
 subprocess.run([a.mingw,'-Wall','-Wextra','-Werror','-Os','-nostdlib','-mwindows','-Wl,-e,_desktop_start','-Wl,--no-insert-timestamp',str(repo/'tools/st-desktop-ready.c'),'-lkernel32','-luser32','-o',str(game/'st-desktop-ready.exe')],check=True)
 subprocess.run([a.mingw,'-Wall','-Wextra','-Werror','-Os','-fdata-sections','-ffunction-sections','-DINITGUID','-nostdlib','-mwindows','-Wl,--gc-sections','-Wl,--strip-all','-Wl,-e,_media_start','-Wl,--no-insert-timestamp',str(repo/'tools/st-media-ready.c'),'-lkernel32','-lole32','-ladvapi32','-o',str(game/'st-media-ready.exe')],check=True)
 for binary in [game/'box64',game/'st-controls',game/'st-log',game/'st-lock',*graphics.glob('*.so')]:
  subprocess.run([a.strip,'--strip-unneeded',str(binary)],check=True);binary.chmod(0o755)
 for source,target in [('tools/media-env.sh','school-tycoon/media-env.sh'),('tools/run-game.sh','school-tycoon/run-game.sh'),('tools/sample-session.sh','school-tycoon/sample-session.sh'),('tools/prepare-wine.sh','school-tycoon/prepare-wine.sh'),('tools/prepare-prefix.sh','school-tycoon/prepare-prefix.sh'),('packaging/School Tycoon.sh','School Tycoon.sh'),('packaging/PLAY-README.txt','PLAY-README.txt'),('packaging/LICENSE-launcher.txt','school-tycoon/LICENSE-launcher.txt')]:
  shutil.copyfile(repo/source,root/target)
 if a.experimental_directmusic:shutil.copyfile(repo/'tools/prepare-directmusic.sh',game/'prepare-directmusic.sh')
 source_files=sorted((repo/'tools').glob('st-*.c'))+[repo/'tools/run-game.sh',repo/'tools/prepare-directmusic.sh',repo/'tools/media-env.sh',repo/'tools/sample-session.sh',repo/'tools/prepare-prefix.sh',repo/'tools/prepare-wine.sh',repo/'packaging/School Tycoon.sh']
 source_id=sha(b''.join(str(f.relative_to(repo)).encode()+b'\0'+f.read_bytes()+b'\0' for f in source_files))
 (game/'BUILD-ID.txt').write_text('handheld-20261015 media='+('native-experiment' if a.experimental_directmusic else 'builtin')+' sources-sha256 '+source_id+'\n')
 (game/'BUILD-NOTICES.txt').write_text('Box64 MIT: unchanged upstream revision 45f85efc52d7bd3a3c8642bfc5093ce7932f6c23, -O2 -DNDEBUG, symbols stripped.\nhttps://github.com/ptitSeb/box64/tree/45f85efc52d7bd3a3c8642bfc5093ce7932f6c23\nOwn helper sources and MIT license:\nhttps://github.com/cdale11/school-tycoon-port/tree/main/tools\nDefault media: Wine builtins; original-game audio remains unverified.\nExperimental builds only: unchanged Debian CAB tools with licenses/sources under runtime/cabextract; native Microsoft media downloaded and hash-verified on device, never bundled.\nWine 10.0 LGPL: downloaded unchanged and checksum verified; not bundled.\nhttps://github.com/Kron4ek/Wine-Builds/releases/tag/10.0\nhttps://gitlab.winehq.org/wine/wine/-/tree/wine-10.0\nWeston/Crusty/GL4ES: reused from the installed PortMaster runtime; not bundled.\nlibgcc: license and source provenance under runtime/x86_64.\n')
 files=sorted(f for f in root.rglob('*') if f.is_file())
 (game/'SHA256SUMS').write_text(''.join(sha(f.read_bytes())+'  '+str(f.relative_to(root))+'\n' for f in files))
 (game/'gamedata').mkdir()
 a.output_dir.mkdir(parents=True,exist_ok=True); archive=a.output_dir/'school-tycoon-handheld.tar.gz'
 # Publish only a complete archive; preserve the preceding release on failure.
 temporary_archive=None
 try:
  with tempfile.NamedTemporaryFile(dir=a.output_dir,prefix='.handheld-',delete=False) as out:
   temporary_archive=Path(out.name)
   with gzip.GzipFile(filename='',fileobj=out,mode='wb',mtime=0) as gz,tarfile.open(fileobj=gz,mode='w') as tar:
    for f in sorted(root.rglob('*')):
     info=tar.gettarinfo(str(f),arcname=str(f.relative_to(root)))
     info.uid=info.gid=0;info.uname=info.gname='';info.mtime=0
     info.mode=0o755 if f.is_dir() or f.suffix in ('.sh','.so') or f.name in ('box64','st-controls','st-log','st-lock','cabextract') else 0o644
     tar.addfile(info,io.BytesIO(f.read_bytes()) if f.is_file() else None)
  temporary_archive.chmod(0o644)
  os.replace(temporary_archive,archive)
 finally:
  if temporary_archive and temporary_archive.exists():temporary_archive.unlink()
 print(str(archive),archive.stat().st_size,'bytes',sha(archive.read_bytes()))
