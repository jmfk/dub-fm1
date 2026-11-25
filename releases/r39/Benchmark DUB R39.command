#!/bin/sh
DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
PY="$DIR/../../.build-env/midi-venv/bin/python"
NAME=${1:-owner}
case "$NAME" in *[!A-Za-z0-9_-]*|'') echo 'Run-namn får bara innehålla bokstäver, siffror, _ eller -.'; exit 1;; esac
RUNS="$DIR/../../dub-synth/hardware-reports/benchmarks"
if [ ! -x "$PY" ]; then echo 'Pythonmiljön saknas.'; exit 1; fi
"$PY" "$DIR/stability_benchmark.py" --canary --run-dir "$RUNS/r39-$NAME-canary" || exit $?
"$PY" "$DIR/validate_run_state.py" "$RUNS/r39-$NAME-canary" --require-complete || exit $?
"$PY" "$DIR/stability_benchmark.py" --run-dir "$RUNS/r39-$NAME-full" --canary-run "$RUNS/r39-$NAME-canary"
result=$?
printf '\nBenchmark slutade med kod %s. Resultat: %s\nTryck Retur för att stänga.\n' "$result" "$RUNS/r39-$NAME-full"
read -r reply
exit "$result"
