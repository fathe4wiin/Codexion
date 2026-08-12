#!/usr/bin/env bash
# Scale / subject cases that failed or were flaky on a constrained VM.
# Run on a normal machine: ./test_eval_cases.sh
set -u

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT" || exit 1

BIN="./codexion"
PASS=0
FAIL=0
SKIP=0

red() { printf '\033[31m%s\033[0m\n' "$*"; }
grn() { printf '\033[32m%s\033[0m\n' "$*"; }
ylw() { printf '\033[33m%s\033[0m\n' "$*"; }

build() {
	echo "== make re =="
	if ! make re; then
		red "BUILD FAILED"
		exit 1
	fi
	echo
}

# $1=label $2=timeout_sec $3...=args
run_cap() {
	local label="$1"
	local to="$2"
	shift 2
	local out
	out="$(mktemp)"
	echo "---- $label ----"
	echo "cmd: timeout ${to}s $BIN $*"
	if timeout "$to" "$BIN" "$@" >"$out" 2>/tmp/cx_err.txt; then
		:
	else
		local ec=$?
		if [[ $ec -eq 124 ]]; then
			ylw "TIMEOUT after ${to}s (treated as fail for this case)"
			echo "(last 15 lines)"
			tail -n 15 "$out" || true
			rm -f "$out"
			return 124
		fi
	fi
	cat "$out"
	# leave path in global for checkers
	LAST_OUT="$out"
	return 0
}

mark_pass() { grn "PASS: $1"; PASS=$((PASS + 1)); }
mark_fail() { red "FAIL: $1"; FAIL=$((FAIL + 1)); }

# --- Easy #1: single coder must burn out ~800, never compile ---
test_single_coder_burnout() {
	local label="Easy: 1 coder must burn out (~800), never compile"
	if ! run_cap "$label" 5 1 800 200 200 200 10 0 fifo; then
		mark_fail "$label (timeout/hang — coder should stop ~800ms)"
		return
	fi
	local burns compiles takes
	burns="$(grep -c 'burned out' "$LAST_OUT" || true)"
	compiles="$(grep -c 'is compiling' "$LAST_OUT" || true)"
	takes="$(grep -c 'has taken a dongle' "$LAST_OUT" || true)"
	local last ts
	last="$(tail -n 1 "$LAST_OUT")"
	ts="$(awk '{print $1; exit}' <<<"$last")"

	local ok=1
	[[ "$burns" -eq 1 ]] || ok=0
	[[ "$compiles" -eq 0 ]] || ok=0
	[[ "$takes" -eq 0 ]] || ok=0
	[[ "$last" == *'burned out'* ]] || ok=0
	# allow timing tolerance around 800
	if [[ -n "$ts" && "$ts" =~ ^[0-9]+$ ]]; then
		if (( ts < 790 || ts > 820 )); then
			ylw "note: burnout timestamp=$ts (expected ~800 ±10+)"
		fi
	fi

	if [[ $ok -eq 1 ]]; then
		mark_pass "$label (ts=$ts)"
	else
		mark_fail "$label (burns=$burns compiles=$compiles takes=$takes last='$last')"
	fi
	rm -f "$LAST_OUT"
}

# --- Medium: cooldown must complete without burnout ---
# Expect: no "burned out", every coder compiles >= N times, process exits.
test_cooldown_complete() {
	local label="$1"
	local to="$2"
	shift 2
	local n_req="$6"

	if ! run_cap "$label" "$to" "$@"; then
		mark_fail "$label (timeout)"
		return
	fi
	local burns
	burns="$(grep -c 'burned out' "$LAST_OUT" || true)"
	if [[ "$burns" -ne 0 ]]; then
		mark_fail "$label (burned out=$burns, last=$(tail -n 1 "$LAST_OUT"))"
		rm -f "$LAST_OUT"
		return
	fi

	# each coder id 1..n_coders must have >= n_req compiles
	local n_coders="$1"
	local missing
	missing="$(awk -v need="$n_req" -v n="$n_coders" '
		/is compiling/ { c[$2]++ }
		END {
			bad=0
			for (i=1; i<=n; i++)
				if (c[i]+0 < need) { print i":"c[i]+0; bad=1 }
			exit bad
		}' "$LAST_OUT")"
	if [[ -n "$missing" ]]; then
		mark_fail "$label (compiles short: $missing)"
	else
		mark_pass "$label"
	fi
	rm -f "$LAST_OUT"
}

# Optional: compare fifo vs edf grant pressure (informational)
test_sched_compare() {
	local label="Medium: fifo vs edf under cooldown 800 (informational)"
	echo "---- $label ----"
	echo "Run both and eyeball grant order under contention:"
	echo "  $BIN 5 3000 200 200 200 10 800 fifo"
	echo "  $BIN 5 3000 200 200 200 10 800 edf"
	ylw "SKIP auto-assert (manual compare of waiting grant order)"
	SKIP=$((SKIP + 1))
}

build

echo "========== FAILING / FLAKY CASES =========="
test_single_coder_burnout
test_cooldown_complete \
	"Medium: cooldown 400 fifo must complete" 45 \
	5 3000 200 200 200 10 400 fifo
test_cooldown_complete \
	"Medium: cooldown 800 fifo must complete" 90 \
	5 3000 200 200 200 10 800 fifo
test_cooldown_complete \
	"Medium: cooldown 800 edf must complete" 90 \
	5 3000 200 200 200 10 800 edf
test_sched_compare

echo
echo "========== SUMMARY =========="
echo "PASS=$PASS FAIL=$FAIL SKIP=$SKIP"
if [[ "$FAIL" -ne 0 ]]; then
	exit 1
fi
exit 0
