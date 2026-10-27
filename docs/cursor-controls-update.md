# Cursor and controller repair — 28 October 2026

The handheld now has sound and faster gameplay. The latest user report describes
an initially off-screen cursor, an opaque box following it, misplaced text and
slow school loading. A screenshot is not required for this investigation.

The supplied gameplay logs confirm 800×600 → 720×480 presentation, but a stale
256×1 GL viewport. They contain 249 texture-upload GL_INVALID_OPERATION reports.
The final memory sample reports 495,660 KiB game RSS, 151,368 KiB available RAM
and only 27,336 KiB free system swap. Prefix readiness took 445 seconds in that
run. These observations do not prove an out-of-memory kill or identify every
texture-rendering failure.

## Corrections

The native Crusty cursor used the inherited viewport and blend equation. It also
left texture-unit-zero binding, blend factors and VAO changed behind GL4ES's
state cache. Its first texture upload inherited pixel-unpack settings/buffers.
The port's own preload wrapper now sets a full-drawable viewport, additive alpha
blending, a neutral sampler and clean unpack state for the cursor. It restores
all affected state afterward, including first-use initialization. No vendor
binary is patched. The one-time `ST cursor: isolated native overlay` log confirms
that this path ran. This repairs concrete state defects; the handheld black-box
and text symptoms have not yet been confirmed resolved.

On the first pointer movement in verified 800×600 mode, controls place the cursor
inside the game area. Subsequent movement queries its actual position and clamps
to 0–799 / 0–599, preserving mouse dragging and edge scrolling. Recognized menu
selection remains available; L1 provides precise pointer control.

The menu observer previously kept one `/proc/PID/mem` descriptor. If Wine execs
after that descriptor opens, it refers to the obsolete address space permanently.
An unreadable game image is now reopened once per second. A successful image
read is logged once. Exact executable verification and read-only screen checks
remain in place; unknown screens still use the pointer.

## Validation and limits

- Cursor regression: stale viewport, inherited blend/sampler/unpack state, and
  complete restoration across repeated calls passed against native API fixtures.
- Real process exec test reproduced the stale memory descriptor and verified
  reopening accesses the new image.
- SDL virtual controller on a real X server: off-screen recovery, right/bottom
  bounds, clicks, held-input release and Guide/Select+Start exit passed.
- Presentation, menu geometry, launcher cleanup, firmware-settings audit and
  package-integrity regression checks passed.

These are targeted host regression tests, not new physical Mali or original-game
playthrough results. The prior VM audio/game results remain documented in
[audio-menu-update.md](audio-menu-update.md). Remaining texture failures and
school-loading memory pressure require separate validation; this release does
not claim that all text defects or loading delays are fixed.

## Update

Stop the game. Extract `releases/school-tycoon-handheld.tar.gz` into
`/userdata/roms/ports/`, replacing included files. Keep existing gamedata, saves,
Wine runtime and prefix. Launch School Tycoon from Ports. Keep 800×600 selected.
No new asset copy, prefix reset or firmware-setting changes are needed.
