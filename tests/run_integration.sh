#!/bin/sh
# Chạy kiểm thử tích hợp với một ibus-daemon riêng (không đụng tới phiên làm việc thật).
# Dùng: dbus-run-session -- tests/run_integration.sh build/ibus-engine-vilikey
set -eu
ENGINE="$(readlink -f "${1:?đường dẫn tới ibus-engine-vilikey}")"
HERE="$(dirname "$(readlink -f "$0")")"
PY="${PYTHON:-python3}"

TMP="$(mktemp -d)"
trap 'kill $DAEMON 2>/dev/null || true; rm -rf "$TMP"' EXIT
mkdir -p "$TMP/component" "$TMP/config"
export XDG_CONFIG_HOME="$TMP/config"
export IBUS_COMPONENT_PATH="$TMP/component"
XML_IN="$HERE/../data/vilikey.xml.in"
sed -e "s|@VN_LIBEXECDIR@/ibus-engine-vilikey|$ENGINE|" -e "s|@VN_VERSION@|test|" \
    -e "s|@VN_DATADIR@|/tmp|" "$XML_IN" > "$TMP/component/vilikey.xml"

rc=0
for mode in preedit direct; do
    mkdir -p "$XDG_CONFIG_HOME/ibus-vilikey"
    printf '[vilikey]\ninput-method=telex\nmode=%s\n' "$mode" \
        > "$XDG_CONFIG_HOME/ibus-vilikey/config.ini"
    ibus-daemon --panel=disable --emoji-extension=disable --xim=false \
                --cache=none --config=disable --replace >"$TMP/daemon.log" 2>&1 &
    DAEMON=$!
    sleep 1.5
    "$PY" "$HERE/ibus_integration.py" "$mode" || { rc=1; cat "$TMP/daemon.log"; }
    kill $DAEMON 2>/dev/null || true
    wait $DAEMON 2>/dev/null || true
done
exit $rc
