#!/usr/bin/env python3
"""Boot a private ARM64 test VM; no game or firmware assets are published."""
import argparse
import json
import os
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--vm-dir', type=Path, required=True)
parser.add_argument('--analysis-dir', type=Path, required=True,
                    help='Read-only share containing private runtime/game files')
parser.add_argument('--qemu', type=Path, required=True)
parser.add_argument('--socket-console', action='store_true',
                    help='Use reconnectable private UNIX serial/monitor sockets')
parser.add_argument('--results-dir', type=Path,
                    help='Dedicated writable output share for captures and logs')
args = parser.parse_args()
profile = json.loads(Path(__file__).with_name('rg34xxsp.json').read_text())
vm = args.vm_dir.resolve()
config = (vm / 'linux-4.9.170/.config').read_text()
for setting in ('CONFIG_ARM64_4K_PAGES=y', 'CONFIG_ARM64_VA_BITS=39',
                'CONFIG_VIRTIO_BLK=y', 'CONFIG_NET_9P_VIRTIO=y', 'CONFIG_SQUASHFS=y',
                'CONFIG_HW_RANDOM=y', 'CONFIG_HW_RANDOM_VIRTIO=y'):
    if setting not in config.splitlines():
        parser.error(f'Kernel prerequisite missing: {setting}')
for name in ('linux-4.9.170/arch/arm64/boot/Image', 'initrd.gz',
             'rootfs.squashfs', 'data.ext4'):
    if not (vm / name).is_file():
        parser.error(f'Missing private VM input: {vm / name}')
# Guest has no network device. Original assets are read-only on the host;
# Wine, prefix, logs and test copies use the separate guest ext4 disk.
command = [str(args.qemu.resolve()), '-machine', 'virt,gic-version=2,highmem=off',
           '-cpu', f"{profile['cpu']},cntfrq={profile['reported_counter_hz']}", '-smp', str(profile['cores']),
           '-m', str(profile['vm_ram_mib']), '-accel', 'tcg,thread=multi',
           '-kernel', str(vm / 'linux-4.9.170/arch/arm64/boot/Image'),
           '-initrd', str(vm / 'initrd.gz'),
           '-append', 'console=ttyAMA0 rdinit=/init panic=0',
           '-drive', f'if=none,file={vm}/rootfs.squashfs,format=raw,readonly=on,id=root',
           '-device', 'virtio-blk-device,drive=root',
           '-drive', f'if=none,file={vm}/data.ext4,format=raw,id=data',
           '-device', 'virtio-blk-device,drive=data',
           '-fsdev', f'local,id=analysis,path={args.analysis_dir.resolve()},security_model=none,readonly=on',
           '-device', 'virtio-9p-device,fsdev=analysis,mount_tag=analysis',
           '-object', 'rng-random,id=hostrng,filename=/dev/urandom',
           '-device', 'virtio-rng-device,rng=hostrng',
           '-nic', 'none', '-nographic']
if args.results_dir:
    results = args.results_dir.resolve()
    analysis = args.analysis_dir.resolve()
    if results == analysis or results in analysis.parents:
        parser.error('The writable output share must not contain the private input share')
    results.mkdir(parents=True, exist_ok=True)
    command += ['-fsdev', f'local,id=results,path={results},security_model=none',
                '-device', 'virtio-9p-device,fsdev=results,mount_tag=results']
if args.socket_console:
    for name in ('serial.sock', 'monitor.sock'):
        if (vm / name).exists():
            parser.error(f'Socket path already exists; check its owner before reusing it: {vm / name}')
    command += ['-serial', f'unix:{vm}/serial.sock,server=on,wait=off',
                '-monitor', f'unix:{vm}/monitor.sock,server=on,wait=off']
raise SystemExit(subprocess.call(command, env=os.environ.copy()))
