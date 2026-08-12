# The cooldown starvation fix

How the `dongle_cooldown` burnouts were diagnosed, why they happened, and what
changed in the code. Written to be defence-ready: every rule below exists
because the version without it was measured and failed.

---

## 1. The symptom

```text
./codexion 5 3000 200 200 200 10 400 fifo
```

The scale requires this run to complete: 10 compiles per coder, no burnout.
Instead it ended with a burnout at ~3000 ms, and the burnout timestamp was
*exactly* `time_to_burnout`, which means the victim had **never compiled once**
in three seconds while other coders compiled two or three times. The same
happened under `edf`. It was not flaky timing on a slow VM: a clean machine
produced the same pass/fail ratio.

## 2. How it was diagnosed

Guessing from logs was not conclusive, so the monitor was temporarily
instrumented to dump the whole table state to `stderr` at the instant it
declared a burnout (dongle holder, remaining cooldown, queue contents with
arrival times, and each coder's compile count). One run said:

```text
=== BURNOUT victim=5 t=3001 ===
D0 holder=-1 ready_in=400 qsize=1 q=[1(a=2400) ]
D1 holder=2  ready_in=-601 qsize=0 q=[]
D2 holder=-1 ready_in=400 qsize=1 q=[2(a=2400) ]
D3 holder=-1 ready_in=-1  qsize=2 q=[3(a=2400) 4(a=3001) ]
D4 holder=-1 ready_in=-1  qsize=1 q=[5(a=2400) ]
C1 compiled=1  C2 compiled=1  C3 compiled=2  C4 compiled=3  C5 compiled=0
```

Read together with the log line `2400 5 has taken a dongle` (a single take), this
says it all: coder 5 had been **holding dongle 0 since t=2400** while waiting for
dongle 4's cooldown to expire, and it had never compiled. Coder 4, sharing the
other side of the ring, had compiled three times.

The dump was removed once the cause was understood; it is not in the submitted
code.

## 3. Root cause: hold-and-wait

The old `take_two_dongles` acquired sequentially:

```text
take_dongle(first)   /* logs "has taken a dongle" immediately */
take_dongle(second)  /* blocks here, still holding the first  */
```

Ordering by dongle id was enough to prevent *deadlock* (no circular wait), but it
kept Coffman's **hold-and-wait** condition, which with a cooldown is very
expensive:

- the dongle already held sits **idle** for the whole cooldown of the partner, so
  each compile occupied its dongles for `t_compile + wait + cooldown` instead of
  `t_compile + cooldown`. Measured throughput was ~6 compiles / 3 s against a
  capacity of ~15;
- worse, that idle-held dongle **blocks the neighbour** who needs it. Coder 5
  holding dongle 0 is exactly why coder 1 was queued on dongle 0 doing nothing.

Starvation was therefore structural, not a scheduling accident.

## 4. The new acquisition protocol

A coder never holds one dongle while waiting for the other. It registers on both
dongles and takes them **atomically under both mutexes**, or takes nothing.

`take_two_dongles` (`srcs/coder_take.c`) now loops:

```text
while not stopped:
    try_pair_once():
        lock_pair(a, b)                 /* always in dongle-id order */
        join_pair_queues(a, b, c)       /* one request per queue, pushed once */
        claim_or_mark(a, b, c)          /* take both, or record why not */
        unlock_pair(a, b)
    if claimed: log two takes, return
    wait_pair_tick(a, b)                /* cond_timedwait, 1 ms re-check */
```

Three properties make this correct and fast, and each was needed:

**(a) Queue entries persist across retries.** `ensure_queued` pushes a request
only if the coder is not already in that queue, so a waiter keeps its original
`arrival` (and `deadline`) for as long as it waits. An earlier attempt that
dequeued on every failed try kept resetting a starving coder to the back of the
line, which is why it did not help.

**(b) A waiter that cannot proceed marks itself, so others can pass.** Requiring
"head of both queues" alone is perfectly fair but serialized the whole ring:
coder 3 waited on coder 2, who was itself stuck waiting elsewhere, so only one
coder compiled at a time (measured: 5 compiles / 3 s, everyone burning out
together). Now, when `claim_or_mark` fails, it records on each queue entry
whether *the other* dongle is what blocks us:

```text
heap_set_blocked(&a->queue, c->id, !ub);   /* b unusable => I can't use a */
heap_set_blocked(&b->queue, c->id, !ua);
```

`priority_ok` then ignores an ahead-of-us request while that request is blocked,
so coders that do not compete for the same dongle run in parallel again. This is
safe with no extra locking: an entry is only ever read or written while that
dongle's mutex is held, and a coder only touches its own two dongles.

**(c) A long-waiting request stops yielding (aging).** Pure bypassing was too
permissive: a starving coder's two neighbours kept taking its dongles out of
phase, so it never saw both free at the same instant (measured: throughput
doubled to 10 compiles / 3 s, but one coder still got 0). `req_stale` fixes the
window:

```text
req_yields(r) = r->blocked && (now - r->arrival < t_compile + dongle_cooldown)
```

Once a request has waited longer than one full dongle cycle it is no longer
bypassable, so nobody may take those dongles ahead of it and it gets them within
about one more cycle. Bypass can only ever cost a waiter a bounded number of
turns.

### Why this cannot deadlock or starve

- **Mutual exclusion / no duplication:** `holder` is only assigned by
  `claim_pair` while *both* dongle mutexes are held, after both were verified
  free, ready, and permitted.
- **No hold-and-wait:** a coder holds dongles only between `claim_pair` and
  `release_two_dongles`, never while waiting.
- **No circular wait:** `lock_pair` / `unlock_pair` always order the two mutexes
  by dongle id, and no other code path holds two dongle mutexes at once.
- **No starvation:** a request that ages stops being bypassable; from that point
  it has strict priority on both its dongles, whose holders always release after
  `t_compile`, and whose cooldowns always expire. In the limit where every
  request is aged, the policy degrades to strict FIFO/EDF order — slower, but
  still progressing.
- **No lost wakeup:** releases broadcast on the released dongle's condition
  variable, and `wait_pair_tick` additionally bounds its `pthread_cond_timedwait`
  to 1 ms, so a change on the dongle it is *not* parked on costs at most a
  millisecond of latency instead of a permanent sleep.

## 5. EDF tie-break

`req_better` now orders EDF by **deadline, then request arrival, then coder id**
(FIFO is unchanged: arrival, then id).

Equal deadlines are not rare at startup, they are systematic: a coder that
compiled at t≈0 has `last_compile + t_burnout` equal to a coder that never
compiled at all. Always resolving that tie by lowest id starved coder 5 in
roughly 1 run in 7 at cooldown 400. Preferring the longest-waiting request
removes the bias and is still fully deterministic, as the subject requires.

The recode targets stay one-liners: FIFO → LIFO flips the `arrival` comparison,
and "EDF prefers the higher id" flips the final `coder_id` comparison, which is
still exercised whenever several coders request within the same millisecond.

## 6. What changed, file by file

| File | Change |
|------|--------|
| `srcs/coder_take.c` | **new** — `build_req`, `join_pair_queues`, `wait_alone`, `try_pair_once`, `take_two_dongles` (moved out of `coder_compile.c` and rewritten around pair acquisition) |
| `srcs/dongle_pair.c` | **new** — `lock_pair`, `unlock_pair`, `claim_pair`, `leave_pair` |
| `srcs/dongle_wait.c` | **new** — `req_stale`, `req_yields`, `claim_or_mark`, `wait_pair_tick` |
| `srcs/heap_queue.c` | **new** — `heap_find`, `heap_set_blocked`, `heap_remove_id` (removing an arbitrary element, then sifting both ways) |
| `srcs/dongle_take.c` | rewritten: `dongle_ready`, `priority_ok`, `usable_dongle`, `ensure_queued`, `dequeue_waiter`. `try_acquire`, `enqueue_waiter` and `wait_for_grant` are gone |
| `srcs/dongle_release.c` | `grant_next` removed. Handing a dongle to a queued coder is wrong now that dongles are only taken in pairs; `release_dongle` clears the holder, arms the cooldown and broadcasts |
| `srcs/coder_compile.c` | old sequential `take_two_dongles` removed; the rest (`do_compile`, `release_two_dongles`, `compile_cycle`) unchanged |
| `srcs/heap_sift.c` | `req_better`: arrival inserted as the EDF tie-break before coder id |
| `includes/codexion.h` | `t_req` gains `blocked`; prototypes updated for the added/removed functions |
| `Makefile` | four new sources added to `SRCS` |
| `test_eval_cases.sh` | runs the full scale suite; assertions corrected (see below) |

Norm limits hold everywhere: at most 5 functions per file, no function body over
25 lines, no line over 80 columns.

## 7. Results

| Case | Before | After |
|------|--------|-------|
| `5 3000 200 200 200 10 400 fifo` | burnout ~3000, one coder with 0 compiles | completes, 10–12 compiles per coder, 11/11 runs clean |
| `5 3000 200 200 200 10 400 edf` | burnout ~3000 | completes, 8/8 runs clean after the tie-break change |
| `5 2000 200 200 200 10 0 fifo` / `7 0 edf` | pass | pass |
| `1 800 …` single coder | burns out, no take | unchanged (correct) |
| `5 500 …` infeasible | burnout last line | unchanged (correct) |
| all 21 cases in `tester.sh` | — | exit 0, no malformed lines, no hangs, including 300 coders |

`test_eval_cases.sh` reports `PASS=8 FAIL=0`. It also checks, on every run, that
each line matches the subject's format (proving log serialization), that every
`is compiling` is preceded by exactly two takes by the same coder, and that
cooldown is respected — the last one via a capacity invariant: compiles starting
within `t_compile + dongle_cooldown` of each other must use disjoint dongle
pairs, so at most `n / 2` of them may start in any such window.

## 8. Why `… 10 800` is expected to burn out

The tester no longer requires that case to complete, because it is infeasible.

A dongle is reusable only every `t_compile + cooldown` = 1000 ms, and on a ring
of 5 coders at most 2 can compile simultaneously, so each 1000 ms window offers
2 compile slots. The first 3000 ms therefore holds 6 slots, while demand is 7:
five first compiles (every coder's deadline starts at the beginning of the
simulation) plus a second compile for each of the 2 coders that compiled at t≈0,
whose deadline falls at exactly 3000. One coder must miss it.

This matches the scale, which for the 800 case asks only to **compare the grant
order** between `fifo` and `edf`, and the subject, whose liveness rule is scoped
to feasible parameters. The tester now prints the grant order for both
schedulers and verifies the correctness invariants instead of demanding
completion.

## 9. Still open

- The four new source files are untracked in git — `git add` them before
  pushing, otherwise a fresh clone will not build.
- `./codexion 5 1000 200 200 200 0 10 fifo` exits 1, because
  `number_of_compiles_required` is parsed with `parse_pos_int`. The subject only
  says to reject negatives and non-integers, so `0` is arguably valid and should
  stop the simulation immediately; switching that one call to `parse_nn_int`
  would do it. Left as is because it is a spec judgment call, unrelated to this
  bug.
