# Fresh installation on RG34XXSP / KNULLI Scarab

The current build is a tested compatibility candidate, not a certified native
handheld port. Emulator testing cannot establish that native Mali rendering,
audio, physical controls or speed will work on your device.

## Start without old runtime or prefix state

Stop the game and reboot KNULLI. Keep your legally obtained original game files
available on your computer. Back up your saves in `gamedata/Data/SAVED_GAMES`
and any prefix data you need, following existing links before removing anything.

Move both old port directories out of Ports, so the launcher cannot reuse their
Wine, prefix or settings. This also preserves their actual contents as a backup:

```sh
ports=/userdata/roms/ports
backup=/userdata/system/school-tycoon-backup-$(date +%Y%m%d-%H%M%S)
mkdir -p "$backup"
for name in school-tycoon school-tycoon-runtime-probe; do
    if [ -e "$ports/$name" ] || [ -L "$ports/$name" ]; then
        mv "$ports/$name" "$backup/$name"
    fi
done
```

Moving directories does not rewrite absolute symbolic links inside them. Their
old targets may need to be located inside the backup if you later restore data.
Do not copy those links into the fresh installation. You can delete the backup
once the new installation is working and the saves you want are safely stored.

## Download and install

Download `releases/school-tycoon-handheld.tar.gz` and `releases/SHA256SUMS` from
this repository. Verify the handheld archive against its entry in SHA256SUMS,
then extract into `/userdata/roms/ports/`. For example, from your download folder:

```sh
sha256sum school-tycoon-handheld.tar.gz
tar -xzf school-tycoon-handheld.tar.gz -C /userdata/roms/ports/
```

Copy the contents of your original `ST` installation into
`/userdata/roms/ports/school-tycoon/gamedata/`. The layout must be:

```text
ports/School Tycoon.sh
ports/school-tycoon/gamedata/SchoolTycoon.exe
ports/school-tycoon/gamedata/Data/...
```

The original auxiliary files belong alongside the executable. The repository
does not include the original executable or assets. Do not copy the previously
modified handheld gamedata, Wine prefix, cpu-mode.txt or runtime directory.
For default settings, omit the old `settings.dat` and leave `Data/SAVED_GAMES`
empty. The uploaded PC profile had both music and effects muted; copying that
profile carries its mute and graphics settings into a fresh port installation.

PortMaster must be installed with its Weston runtime available. Keep at least
2 GiB free after copying game assets. Connect Wi-Fi for the initial verified
Wine and legacy DirectMusic downloads (the latter is about 105 MiB).
The legacy runtime is hash-verified and extracted locally; Microsoft DLLs are not
included in the repository. Keep the verified cache for repair without another
download. Refresh KNULLI's game list and launch School Tycoon under Ports,
or run `sh "/userdata/roms/ports/School Tycoon.sh"` over SSH.

First-run prefix initialization allows up to ten minutes; later 32-bit execution
verification allows two minutes, media registration/COM verification allows three
minutes, and desktop readiness allows three minutes. These
are bounded failure gates, not a promise about normal startup duration.
Prefix initialization, 32-bit execution/media verification and desktop readiness run
before the game. Inspect `logs/game.txt`, `logs/prefix-setup.txt` and
`logs/desktop.txt` to identify the phase. A black screen alone is not proof of
progress. Failed readiness prevents game launch; do not repeatedly start a
second session while the first is running.

D-pad moves the pointer; A/B are left/right mouse buttons. L1 gives precise
movement; Start is Enter, X is Space, Y is Escape. Fn/Guide + Start exits;
Select + Start is the fallback. Save before exiting. Physical Fn mapping is
still unverified. Use 800x600 and Low detail where available.

## Audio and troubleshooting

The port uses Wine's native Linux audio backend. It does not add rumble or ship
an audio server. Firmware volume/mute and selected output still apply. Native
audio inventory is included in `logs/system.txt`; audio initialization failures
are recorded in bounded Wine logs. Send the entire logs directory on failure.
The emulator audio sink described in the validation report is a testing fixture
and must not be installed on the handheld.

See [fresh emulator results](fresh-validation.md) for what was actually tested,
including audio and the original game's rumble evidence.
