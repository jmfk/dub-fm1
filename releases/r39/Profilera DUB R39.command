#!/bin/sh
DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
PY="$DIR/../../.build-env/midi-venv/bin/python"
NAME=${1:-owner}
case "$NAME" in *[!A-Za-z0-9_-]*|'') echo 'Ogiltigt run-namn'; exit 1;; esac
RUNS="$DIR/../../dub-synth/hardware-reports/benchmarks"
if [ ! -x "$PY" ]; then echo 'Pythonmiljön saknas.'; exit 1; fi
"$PY" "$DIR/profile_benchmark.py" --canary --run-dir "$RUNS/r39-profile-$NAME-canary" || exit $?
"$PY" "$DIR/validate_run_state.py" "$RUNS/r39-profile-$NAME-canary" --require-complete || exit $?
"$PY" "$DIR/profile_benchmark.py" --run-dir "$RUNS/r39-profile-$NAME-full" --canary-run "$RUNS/r39-profile-$NAME-canary"
result=$?
printf '\nResultat: %s\nTryck Retur för att stänga.\n' "$RUNS/r39-profile-$NAME-full"
read -r reply
exit "$result"
