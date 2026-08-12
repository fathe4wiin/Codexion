#!/usr/bin/env bash
# Full Codexion scale suite (subject + correction page benchmarks).
# Usage on a fresh clone:  ./test_eval_cases.sh
set -u

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT" || exit 1

BIN="./codexion"
PASS=0
FAIL=0
LAST_OUT=""

red() { printf '\033[31m%s\033[0m\n' "$*"; }
grn() { printf '\033[32m%s\033[0m\n' "$*"; }
ylw() { printf '\033[33m%s\033[0m\n' "$*"; }

mark_pass() { grn "PASS: $1"; PASS=$((PASS + 1)); }
mark_fail() { red "FAIL: $1"; FAIL=$((FAIL + 1)); }

build() {
	echo "== make re =="
	if ! make re; then
		red "BUILD FAILED"
		exit 1
	fi
	if [[ ! -x "$BIN" ]]; then
		red "binary $BIN missing after make"
		exit 1
	fi
	echo
}

# run_cap <label> <timeout_sec> <args...>  -> log path in $LAST_OUT
run_cap() {
	local label="$1" to="$2" ec=0
	shift 2

	LAST_OUT="$(mktemp)"
	echo "---- $label ----"
	echo "cmd: timeout ${to}s $BIN $*"
	timeout "$to" "$BIN" "$@" >"$LAST_OUT" 2>/dev/null || ec=$?
	if [[ $ec -eq 124 ]]; then
		ylw "TIMEOUT after ${to}s (last lines)"
		tail -n 10 "$LAST_OUT" || true
		return 124
	fi
	return 0
}

count_msg() { grep -c "$1" "$LAST_OUT" 2>/dev/null || true; }

# Every line must be a clean "<ts> <id> <state>" line: proves log serialization.
check_line_format() {
	local bad
	bad="$(grep -cvE '^[0-9]+ [0-9]+ (has taken a dongle|is compiling|is debugging|is refactoring|burned out)$' "$LAST_OUT" || true)"
	if [[ "$bad" -ne 0 ]]; then
		red "  $bad malformed/interleaved line(s)"
		return 1
	fi
	return 0
}

# Each "is compiling" must follow exactly two takes by that same coder.
check_two_takes_before_compile() {
	awk '
	{
		id = $2; $1 = ""; $2 = ""; sub(/^  /, ""); msg = $0
		if (msg == "is compiling") {
			if (pref[id] + 0 != 2) {
				printf "  coder %s compiled after %d take(s)\n", id, pref[id] + 0
				bad++
			}
			pref[id] = 0
		} else if (msg == "has taken a dongle")
			pref[id]++
		else
			pref[id] = 0
	}
	END { exit(bad > 0) }
	' "$LAST_OUT"
}

# A dongle is busy or cooling for t_compile+cooldown after a compile starts, so
# compiles started inside such a window need pairwise distinct dongle pairs.
check_cooldown_capacity() {
	local n_coders="$1" span="$2"
	awk -v n="$n_coders" -v span="$span" '
	/is compiling/ { t[++k] = $1 }
	END {
		for (i = 1; i <= k; i++) {
			cnt = 0
			for (j = i; j <= k && t[j] - t[i] < span - 3; j++)
				cnt++
			if (2 * cnt > n) {
				printf "  %d compiles within %dms (only %d dongles)\n", cnt, span, n
				bad++
			}
		}
		exit(bad > 0)
	}
	' "$LAST_OUT"
}

# Exactly one burnout, and it is the last line.
check_burnout_last() {
	local approx="$1" tol="${2:-40}" burns last ts

	burns="$(count_msg 'burned out')"
	last="$(tail -n 1 "$LAST_OUT")"
	if [[ "$burns" -ne 1 || "$last" != *'burned out'* ]]; then
		red "  burnouts=$burns last='$last'"
		return 1
	fi
	ts="${last%% *}"
	if (( ts < approx - tol || ts > approx + tol )); then
		ylw "  note: burnout ts=$ts (expected ~$approx)"
	fi
	return 0
}

# No burnout and every coder reached the required number of compiles.
check_complete() {
	local n_coders="$1" n_req="$2" burns

	burns="$(count_msg 'burned out')"
	if [[ "$burns" -ne 0 ]]; then
		red "  burned out (last='$(tail -n 1 "$LAST_OUT")')"
		return 1
	fi
	awk -v need="$n_req" -v n="$n_coders" '
		/is compiling/ { c[$2]++ }
		END {
			for (i = 1; i <= n; i++)
				if (c[i] + 0 < need) {
					printf "  coder %d compiled %d/%d times\n", i, c[i] + 0, need
					bad = 1
				}
			exit bad
		}' "$LAST_OUT"
}

show_compiles() {
	awk '/is compiling/ { c[$2]++ }
		END { printf "  compiles per coder:"
			for (i = 1; i <= 5; i++) printf " C%d=%d", i, c[i] + 0
			print "" }' "$LAST_OUT"
}

# ---------------------------------------------------------------- preliminaries
test_invalid_args() {
	local label="Preliminaries: invalid arguments are rejected"

	echo "---- $label ----"
	if "$BIN" 5 800 200 200 200 10 0 bad >/dev/null 2>&1 \
		|| "$BIN" -1 800 200 200 200 10 0 fifo >/dev/null 2>&1 \
		|| "$BIN" 5 800 200 200 200 0 0 fifo >/dev/null 2>&1 \
		|| "$BIN" 5 800 200 200 200 10 0 >/dev/null 2>&1 \
		|| "$BIN" 5 800 200 abc 200 10 0 fifo >/dev/null 2>&1; then
		mark_fail "$label"
		return
	fi
	mark_pass "$label"
}

# ------------------------------------------------------------------------- easy
test_easy_single_burnout() {
	local label="Easy: 1 coder cannot compile and burns out (~800)"

	if ! run_cap "$label" 5 1 800 200 200 200 10 0 fifo; then
		mark_fail "$label (timeout)"
		return
	fi
	if check_line_format && check_burnout_last 800 30 \
		&& [[ "$(count_msg 'is compiling')" -eq 0 ]] \
		&& [[ "$(count_msg 'has taken a dongle')" -eq 0 ]]; then
		mark_pass "$label"
	else
		mark_fail "$label"
	fi
	rm -f "$LAST_OUT"
}

test_easy_complete() {
	local label="$1" sched="$2" need="$3"

	if ! run_cap "$label" 60 5 2000 200 200 200 "$need" 0 "$sched"; then
		mark_fail "$label (timeout)"
		return
	fi
	show_compiles
	if check_line_format && check_two_takes_before_compile \
		&& check_complete 5 "$need"; then
		mark_pass "$label"
	else
		mark_fail "$label"
	fi
	rm -f "$LAST_OUT"
}

# -------------------------------------------------------------------- less easy
test_infeasible_burnout() {
	local label="Less easy: cycle longer than burnout must burn out (~500)"

	if ! run_cap "$label" 10 5 500 200 200 200 10 0 fifo; then
		mark_fail "$label (timeout)"
		return
	fi
	if check_line_format && check_two_takes_before_compile \
		&& check_burnout_last 500 40; then
		mark_pass "$label"
	else
		mark_fail "$label"
	fi
	rm -f "$LAST_OUT"
}

# ----------------------------------------------------------------------- medium
test_cooldown_complete() {
	local label="Medium: cooldown 400 fifo completes without burnout"

	if ! run_cap "$label" 90 5 3000 200 200 200 10 400 fifo; then
		mark_fail "$label (timeout)"
		return
	fi
	show_compiles
	if check_line_format && check_two_takes_before_compile \
		&& check_cooldown_capacity 5 600 && check_complete 5 10; then
		mark_pass "$label"
	else
		mark_fail "$label"
	fi
	rm -f "$LAST_OUT"
}

# The scale only asks to compare the grant order here, not to finish the run:
# heavy contention (cooldown 800) may legitimately end in a burnout.
test_grant_order() {
	local sched="$1"
	local label="Medium: cooldown 800 $sched stays correct under contention"

	if ! run_cap "$label" 120 5 3000 200 200 200 10 800 "$sched"; then
		mark_fail "$label (timeout)"
		return
	fi
	echo "  grant order (first 12 takes): $(awk '/has taken a dongle/ {printf "%s ", $2} NR>60 {exit}' "$LAST_OUT" | cut -d" " -f1-12)"
	if [[ "$(count_msg 'burned out')" -gt 1 ]]; then
		mark_fail "$label (more than one burnout)"
		rm -f "$LAST_OUT"
		return
	fi
	if check_line_format && check_two_takes_before_compile \
		&& check_cooldown_capacity 5 1000; then
		mark_pass "$label"
	else
		mark_fail "$label"
	fi
	rm -f "$LAST_OUT"
}

build

echo "========== SCALE SUITE =========="
test_invalid_args
test_easy_single_burnout
test_easy_complete "Easy: 5 coders, fifo, 10 compiles each" fifo 10
test_easy_complete "Easy: 5 coders, edf, 7 compiles each" edf 7
test_infeasible_burnout
test_cooldown_complete
test_grant_order fifo
test_grant_order edf

echo
echo "========== SUMMARY =========="
echo "PASS=$PASS FAIL=$FAIL"
[[ "$FAIL" -eq 0 ]]
