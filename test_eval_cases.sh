#!/usr/bin/env bash
# Full Codexion scale suite (subject + correction page benchmarks).
# Usage (fresh clone):
#   ./test_eval_cases.sh
set -u

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT" || exit 1

BIN="./codexion"
PASS=0
FAIL=0

red() { printf '\033[31m%s\033[0m\n' "$*"; }
grn() { printf '\033[32m%s\033[0m\n' "$*"; }
ylw() { printf '\033[33m%s\033[0m\n' "$*"; }

build() {
	echo "== make re =="
	if ! make re; then
		red "BUILD FAILED"
		exit 1
	fi
	if [[ ! -x "$BIN" ]]; then
		red "Binary $BIN missing after make"
		exit 1
	fi
	echo
}

# $1=label $2=timeout_sec $3...=program args
run_cap() {
	local label="$1"
	local to="$2"
	shift 2
	local out ec=0

	out="$(mktemp)"
	echo "---- $label ----"
	echo "cmd: timeout ${to}s $BIN $*"
	set +e
	timeout "$to" "$BIN" "$@" >"$out" 2>/tmp/cx_err.txt
	ec=$?
	set -e
	if [[ $ec -eq 124 ]]; then
		ylw "TIMEOUT after ${to}s"
		echo "(last 15 lines)"
		tail -n 15 "$out" || true
		rm -f "$out"
		LAST_OUT=""
		return 124
	fi
	cat "$out"
	LAST_OUT="$out"
	return 0
}

mark_pass() { grn "PASS: $1"; PASS=$((PASS + 1)); }
mark_fail() { red "FAIL: $1"; FAIL=$((FAIL + 1)); }

count_msg() {
	grep -c "$1" "$LAST_OUT" 2>/dev/null || true
}

# Every "is compiling" for a coder must be immediately preceded by exactly
# two "has taken a dongle" lines for that same coder (ignoring other coders).
check_two_takes_before_compile() {
	awk '
	{
		ts=$1; id=$2; $1=""; $2=""; sub(/^ /,""); msg=$0
		if (msg == "is compiling") {
			n = pref[id] + 0
			if (n != 2) { bad++; printf "coder %s compile with %d preceding takes\n", id, n }
			pref[id] = 0
		} else if (msg == "has taken a dongle") {
			pref[id]++
		} else {
			# other state for this coder resets the take streak
			pref[id] = 0
		}
	}
	END { exit(bad > 0) }
	' "$LAST_OUT"
}

# Burnout must be the last log line; optional approx timestamp check.
check_burnout_last() {
	local approx="$1"
	local tol="${2:-50}"
	local burns last ts

	burns="$(count_msg 'burned out')"
	last="$(tail -n 1 "$LAST_OUT")"
	[[ "$burns" -eq 1 ]] || return 1
	[[ "$last" == *'burned out'* ]] || return 1
	ts="$(awk '{print $1; exit}' <<<"$last")"
	if [[ -n "$approx" && "$ts" =~ ^[0-9]+$ ]]; then
		if (( ts < approx - tol || ts > approx + tol )); then
			ylw "note: burnout ts=$ts (expected ~$approx ±$tol)"
		fi
	fi
	return 0
}

# Feasible run: no burnout, each coder compiled at least n_req times.
check_complete_no_burnout() {
	local n_coders="$1"
	local n_req="$2"
	local burns missing

	burns="$(count_msg 'burned out')"
	[[ "$burns" -eq 0 ]] || return 1
	missing="$(awk -v need="$n_req" -v n="$n_coders" '
		/is compiling/ { c[$2]++ }
		END {
			bad = 0
			for (i = 1; i <= n; i++)
				if (c[i] + 0 < need) { print i ":" (c[i] + 0); bad = 1 }
			exit bad
		}' "$LAST_OUT")"
	[[ -z "$missing" ]]
}

# ---------- Easy ----------
test_easy_single_burnout() {
	local label="Easy: 1 coder burns out (~800), never compiles"

	if ! run_cap "$label" 5 1 800 200 200 200 10 0 fifo; then
		mark_fail "$label (timeout)"
		return
	fi
	local burns compiles takes
	burns="$(count_msg 'burned out')"
	compiles="$(count_msg 'is compiling')"
	takes="$(count_msg 'has taken a dongle')"
	if check_burnout_last 800 30 \
		&& [[ "$burns" -eq 1 && "$compiles" -eq 0 && "$takes" -eq 0 ]]; then
		mark_pass "$label"
	else
		mark_fail "$label (burns=$burns compiles=$compiles takes=$takes last=$(tail -n1 "$LAST_OUT"))"
	fi
	rm -f "$LAST_OUT"
}

test_easy_fifo_complete() {
	local label="Easy: 5 2000 … 10 0 fifo completes, no burnout"

	if ! run_cap "$label" 45 5 2000 200 200 200 10 0 fifo; then
		mark_fail "$label (timeout)"
		return
	fi
	if check_complete_no_burnout 5 10 && check_two_takes_before_compile; then
		mark_pass "$label"
	else
		mark_fail "$label"
	fi
	rm -f "$LAST_OUT"
}

test_easy_edf_complete() {
	local label="Easy: 5 2000 … 7 0 edf completes, no burnout"

	if ! run_cap "$label" 45 5 2000 200 200 200 7 0 edf; then
		mark_fail "$label (timeout)"
		return
	fi
	if check_complete_no_burnout 5 7 && check_two_takes_before_compile; then
		mark_pass "$label"
	else
		mark_fail "$label"
	fi
	rm -f "$LAST_OUT"
}

# ---------- Less easy ----------
test_less_easy_infeasible() {
	local label="Less easy: 5 500 … fifo must burn out (~500), last line"

	if ! run_cap "$label" 10 5 500 200 200 200 10 0 fifo; then
		mark_fail "$label (timeout)"
		return
	fi
	if check_burnout_last 500 40 && check_two_takes_before_compile; then
		mark_pass "$label"
	else
		mark_fail "$label (last=$(tail -n1 "$LAST_OUT"))"
	fi
	rm -f "$LAST_OUT"
}

# ---------- Medium ----------
test_medium_cooldown_400() {
	local label="Medium: cooldown 400 fifo completes, no burnout"

	if ! run_cap "$label" 60 5 3000 200 200 200 10 400 fifo; then
		mark_fail "$label (timeout)"
		return
	fi
	if check_complete_no_burnout 5 10 && check_two_takes_before_compile; then
		mark_pass "$label"
	else
		mark_fail "$label (last=$(tail -n1 "$LAST_OUT"))"
	fi
	rm -f "$LAST_OUT"
}

test_medium_cooldown_800_fifo() {
	local label="Medium: cooldown 800 fifo completes, no burnout"

	if ! run_cap "$label" 120 5 3000 200 200 200 10 800 fifo; then
		mark_fail "$label (timeout)"
		return
	fi
	if check_complete_no_burnout 5 10 && check_two_takes_before_compile; then
		mark_pass "$label"
	else
		mark_fail "$label (last=$(tail -n1 "$LAST_OUT"))"
	fi
	rm -f "$LAST_OUT"
}

test_medium_cooldown_800_edf() {
	local label="Medium: cooldown 800 edf completes, no burnout"

	if ! run_cap "$label" 120 5 3000 200 200 200 10 800 edf; then
		mark_fail "$label (timeout)"
		return
	fi
	if check_complete_no_burnout 5 10 && check_two_takes_before_compile; then
		mark_pass "$label"
	else
		mark_fail "$label (last=$(tail -n1 "$LAST_OUT"))"
	fi
	rm -f "$LAST_OUT"
}

test_invalid_args() {
	local label="Prelim: invalid args rejected"

	echo "---- $label ----"
	if "$BIN" 5 800 200 200 200 10 0 bad >/dev/null 2>&1; then
		mark_fail "$label (accepted bad scheduler)"
		return
	fi
	if "$BIN" -1 800 200 200 200 10 0 fifo >/dev/null 2>&1; then
		mark_fail "$label (accepted negative coders)"
		return
	fi
	if "$BIN" 5 800 200 200 200 0 0 fifo >/dev/null 2>&1; then
		mark_fail "$label (accepted 0 compiles)"
		return
	fi
	mark_pass "$label"
}

# ---------- main ----------
build

echo "========== SCALE SUITE =========="
test_invalid_args
test_easy_single_burnout
test_easy_fifo_complete
test_easy_edf_complete
test_less_easy_infeasible
test_medium_cooldown_400
test_medium_cooldown_800_fifo
test_medium_cooldown_800_edf

echo
echo "========== SUMMARY =========="
echo "PASS=$PASS FAIL=$FAIL"
if [[ "$FAIL" -ne 0 ]]; then
	exit 1
fi
exit 0
