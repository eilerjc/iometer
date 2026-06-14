#!/usr/bin/env bash
# run_smoke.sh - POSIX (Linux/macOS) smoke-test runner.
#
# Reads the SAME shared scenarios.json that run_smoke.ps1 uses, and executes the
# scenarios whose "platforms" include "linux". MFC scenarios are Windows-only and
# are skipped here. Keep behavior in sync with run_smoke.ps1.
#
# Requires: jq (JSON), and a built IometerQt + a Dynamo. Binary locations are
# resolved from env vars (overridable) so this works regardless of build layout:
#   IOMETER_QT          path to the IometerQt executable
#   IOMETER_DYNAMO      path to the production dynamo
#   IOMETER_DYNAMOTEST  path to a test-mode dynamo (built with IOMTR_TEST_MODE);
#                       defaults to IOMETER_DYNAMO if unset
#   CTEST               path to ctest (default: ctest on PATH)
#
# Usage:
#   ./run_smoke.sh                 # mode=test scenarios
#   ./run_smoke.sh --mode real     # real dynamo (real disk; run as root)
#   ./run_smoke.sh --only qt-dynamotest,unit-tests
#   ./run_smoke.sh --list
#   ./run_smoke.sh --include-pending
set -u

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TESTDIR="$(cd "$HERE/.." && pwd)"
REPO="$(cd "$HERE/../.." && pwd)"
MANIFEST="$HERE/scenarios.json"

MODE="test"
ONLY=""
INCLUDE_PENDING=0
LIST=0
while [ $# -gt 0 ]; do
  case "$1" in
    --mode) MODE="$2"; shift 2;;
    --only) ONLY="$2"; shift 2;;
    --include-pending) INCLUDE_PENDING=1; shift;;
    --list) LIST=1; shift;;
    *) echo "unknown arg: $1"; exit 2;;
  esac
done

command -v jq >/dev/null 2>&1 || { echo "ERROR: jq is required (apt install jq / brew install jq)"; exit 2; }

HOST4="$(hostname)"
RDELAY="$(jq -r '.config.rdelay' "$MANIFEST")"
LOGIN_TIMEOUT="$(jq -r '.config.loginTimeout' "$MANIFEST")"
QT_RUN_SECONDS="$(jq -r '.config.qtRunSeconds' "$MANIFEST")"
BATCH_MAX="$(jq -r '.config.batchMaxWaitSeconds' "$MANIFEST")"
CTEST="${CTEST:-ctest}"

# --- Resolve logical binary names -> real paths (override via env) -----------
resolve_bin() {
  case "$1" in
    IometerQt)  echo "${IOMETER_QT:-$REPO/src/qt/build/IometerQt}";;
    Dynamo)     echo "${IOMETER_DYNAMO:-$REPO/src/dynamo}";;
    dynamotest) echo "${IOMETER_DYNAMOTEST:-${IOMETER_DYNAMO:-$REPO/src/dynamo}}";;
    *) echo ""; return 1;;
  esac
}

port_listening() { # port
  if command -v ss >/dev/null 2>&1; then ss -ltn 2>/dev/null | grep -q ":$1 "
  else netstat -ltn 2>/dev/null | grep -q ":$1 "; fi
}
wait_port() { # port timeout
  local i=0; while [ "$i" -lt "$2" ]; do port_listening "$1" && return 0; sleep 1; i=$((i+1)); done; return 1
}
get_all_row() { # csv  -> echoes the ALL data row, or empty
  awk -F, 'f&&$1=="ALL"{print;exit} /^.?Results/{f=1}' "$1" 2>/dev/null
}
verify_csv() { # name csv iopsGt(optional,-) errorsEq(optional,-)
  local row iops mbps err
  row="$(get_all_row "$2")"
  [ -z "$row" ] && { echo "no ALL row in $2"; return 1; }
  iops="$(echo "$row" | cut -d, -f7  | tr -d ' ')"
  mbps="$(echo "$row" | cut -d, -f13 | tr -d ' ')"
  err="$( echo "$row" | cut -d, -f28 | tr -d ' ')"
  local detail="IOps=$iops MBps=$mbps Errors=$err"
  if [ "$3" != "-" ] && ! awk "BEGIN{exit !($iops > $3)}"; then echo "$detail (IOps not > $3)"; return 1; fi
  if [ "$4" != "-" ] && ! awk "BEGIN{exit !($err == $4)}";  then echo "$detail (Errors != $4)"; return 1; fi
  echo "$detail"; return 0
}
kill_stale() { pkill -f 'IometerQt|dynamo' 2>/dev/null; sleep 1; }

# --- Scenario kinds ----------------------------------------------------------
run_ctest() {
  local build="$REPO/src/qt/build"
  [ -d "$build" ] || { echo "qt build dir missing: $build"; return 1; }
  ( cd "$build" && "$CTEST" --output-on-failure >/dev/null 2>&1 ); local c=$?
  [ "$c" -eq 0 ] && { echo "ctest exit 0"; return 0; } || { echo "ctest exit $c"; return 1; }
}
run_validate_icf() {
  local exe; exe="$(resolve_bin "$(jq -r ".scenarios[]|select(.name==\"$1\").controller" "$MANIFEST")")"
  [ -x "$exe" ] || { echo "missing $exe"; return 1; }
  local fail=0 n=0 icf full
  while IFS= read -r icf; do
    full="$TESTDIR/$icf"; n=$((n+1))
    if [ ! -f "$full" ]; then echo "      [missing] $icf"; fail=$((fail+1)); continue; fi
    if "$exe" --validate-icf "$full" >/dev/null 2>&1; then echo "      [ok]   $icf"
    else echo "      [fail] $icf"; fail=$((fail+1)); fi
  done < <(jq -r ".scenarios[]|select(.name==\"$1\").icfs[]" "$MANIFEST")
  [ "$fail" -eq 0 ] && { echo "$((n-fail))/$n fixtures validated"; return 0; } || { echo "$((n-fail))/$n fixtures validated"; return 1; }
}
run_demo() {
  local exe; exe="$(resolve_bin "$(jq -r ".scenarios[]|select(.name==\"$1\").controller" "$MANIFEST")")"
  [ -x "$exe" ] || { echo "missing $exe"; return 1; }
  kill_stale
  "$exe" --demo & local pid=$!
  sleep 5
  kill -0 "$pid" 2>/dev/null || { echo "exited early"; return 1; }
  sleep "$QT_RUN_SECONDS"
  if kill -0 "$pid" 2>/dev/null; then kill "$pid" 2>/dev/null; echo "ran ${QT_RUN_SECONDS}s without crashing"; return 0
  else echo "crashed during demo run"; return 1; fi
}
run_batch_worker() {
  local s ctrl wkr icf out wargs i
  ctrl="$(resolve_bin "$(jq -r ".scenarios[]|select(.name==\"$1\").controller" "$MANIFEST")")"
  wkr="$(resolve_bin  "$(jq -r ".scenarios[]|select(.name==\"$1\").worker"     "$MANIFEST")")"
  icf="$TESTDIR/$(jq -r ".scenarios[]|select(.name==\"$1\").icf" "$MANIFEST")"
  out="$(mktemp -t smoke_${1}_XXXX.csv)"
  [ -x "$ctrl" ] || { echo "missing controller $ctrl"; return 1; }
  [ -x "$wkr" ]  || { echo "missing worker $wkr"; return 1; }
  kill_stale
  "$ctrl" /c "$icf" /r "$out" /t "$LOGIN_TIMEOUT" & local cpid=$!
  if ! wait_port 1066 50; then kill "$cpid" 2>/dev/null; echo "controller never listened on 1066"; return 1; fi
  wargs="$(jq -r ".scenarios[]|select(.name==\"$1\").workerArgs" "$MANIFEST")"
  wargs="${wargs//\{host\}/$HOST4}"; wargs="${wargs//\{rdelay\}/$RDELAY}"
  # shellcheck disable=SC2086
  "$wkr" $wargs & local wpid=$!
  i=0; while kill -0 "$cpid" 2>/dev/null && [ "$i" -lt "$BATCH_MAX" ]; do sleep 2; i=$((i+2)); done
  local done=1; kill -0 "$cpid" 2>/dev/null && done=0
  kill "$wpid" "$cpid" 2>/dev/null
  [ "$done" -eq 1 ] || { echo "controller did not finish within ${BATCH_MAX}s"; return 1; }
  local gt eq
  gt="$(jq -r ".scenarios[]|select(.name==\"$1\").verify.iopsGt   // \"-\"" "$MANIFEST")"
  eq="$(jq -r ".scenarios[]|select(.name==\"$1\").verify.errorsEq // \"-\"" "$MANIFEST")"
  verify_csv "$1" "$out" "$gt" "$eq"
}

# --- Selection ---------------------------------------------------------------
modes="$MODE"; [ "$MODE" = "both" ] && modes="test real"

if [ "$LIST" -eq 1 ]; then
  echo; echo "Scenarios (platform=linux):"
  jq -r '.scenarios[]|select(.platforms|index("linux"))|"  \(.name)  modes=\(.modes|join(","))  \(.status // "")\n      \(.description)"' "$MANIFEST"
  exit 0
fi

if echo "$modes" | grep -qw real && [ "$(id -u)" -ne 0 ]; then
  echo "WARNING: real mode does raw-disk I/O and usually needs root (sudo)."
fi

echo; echo "=== Iometer Smoke Suite (mode=$MODE) ==="
pass=0; fail=0; skip=0
names="$(jq -r '.scenarios[]|select(.platforms|index("linux"))|.name' "$MANIFEST")"
for name in $names; do
  smode="$(jq -r ".scenarios[]|select(.name==\"$name\").modes|join(\" \")" "$MANIFEST")"
  match=0; for m in $modes; do echo "$smode" | grep -qw "$m" && match=1; done
  [ "$match" -eq 1 ] || continue
  if [ -n "$ONLY" ] && ! echo ",$ONLY," | grep -q ",$name,"; then continue; fi
  status="$(jq -r ".scenarios[]|select(.name==\"$name\").status // \"\"" "$MANIFEST")"
  if [ "$status" = "pending" ] && [ "$INCLUDE_PENDING" -eq 0 ]; then
    echo "[SKIP] $name (pending - use --include-pending)"; skip=$((skip+1)); continue
  fi
  kind="$(jq -r ".scenarios[]|select(.name==\"$name\").kind" "$MANIFEST")"
  echo; echo "[RUN ] $name ($kind)"
  case "$kind" in
    ctest)        detail="$(run_ctest "$name")";        ok=$?;;
    validate-icf) detail="$(run_validate_icf "$name")"; ok=$?;;
    demo)         detail="$(run_demo "$name")";         ok=$?;;
    batch-worker) detail="$(run_batch_worker "$name")"; ok=$?;;
    *)            detail="unknown/unsupported kind '$kind' on this platform"; ok=1;;
  esac
  if [ "$ok" -eq 0 ]; then echo "[PASS] $name: $detail"; pass=$((pass+1))
  else echo "[FAIL] $name: $detail"; fail=$((fail+1)); fi
done

echo; echo "==================== SUMMARY ===================="
echo "Result: $pass passed, $fail failed, $skip skipped"
[ "$fail" -eq 0 ]
