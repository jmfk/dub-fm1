#!/bin/sh
DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
PY="$DIR/../../.build-env/midi-venv/bin/python"
if [ ! -x "$PY" ]; then echo 'Pythonmiljön saknas; inget startades.'; exit 1; fi
if [ "${1:-}" = '--check' ]; then exec "$PY" "$DIR/manual_install.py" --check; fi
if [ "$#" -ne 0 ]; then echo 'Okänt argument'; exit 1; fi
"$PY" "$DIR/manual_install.py"
result=$?
printf '\nTryck Retur för att stänga.\n'
read -r reply
exit "$result"
