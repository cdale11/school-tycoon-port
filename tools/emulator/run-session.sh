#!/bin/sh
# Snapshot scripts so edits to the shared source cannot alter a running test.
set -eu
bb=/userdata/busybox
model=$("$bb" tr -d '\000' </sys/firmware/devicetree/base/model)
[ "$model" = linux,dummy-virt ] || exit 1
script_dir=$("$bb" dirname "$0")
session=$("$bb" mktemp -d /userdata/st-game-session.XXXXXX)
trap '"$bb" rm -rf "$session"' EXIT
"$bb" cp "$script_dir/run-game.sh" "$script_dir/graphics-env.sh" "$session/"
set +e
/bin/sh "$session/run-game.sh"
status=$?
exit "$status"
