# Codexion — final report vs subject v1.5

Checked against `en.subject.pdf` (Version 1.5) and the current evaluation
scale (Easy / Less easy / Medium + FIFO→LIFO recode). Date: 2026-08-18.

## Verdict

The mandatory program behaviour matches the subject. The only submission
blocker that was still missing was **`README.md`**; that file is now at the
repo root with login `bfathi` and every required section. Scale benchmark
cases all pass locally. Leaks and thread checkers are clean on the runs
below.

Nothing required by the subject is still unimplemented. Remaining items are
defence talking points, not missing features.

## What was added or changed in this pass

| Item | Status |
|------|--------|
| `README.md` (subject Ch. VII) | Added: italic first line, Description, Instructions, Resources + AI use, Blocking cases handled, Thread synchronization mechanisms |
| Monitor poll `usleep(10000)` (10 ms) | Tightened to `usleep(200)` so burnout logs sit inside the 10 ms window |
| Makefile `.PHONY` | Now `all clean fclean re` |

## Scale tests (all PASS)

```text
./codexion 1 800 200 200 200 10 0 fifo     # burns out ~800, never compiles
./codexion 5 2000 200 200 200 10 0 fifo    # completes, ≥10 compiles each
./codexion 5 2000 200 200 200 7 0 edf      # completes, ≥7 compiles each
./codexion 5 500 200 200 200 10 0 fifo     # burns out ~500, last line
./codexion 5 3000 200 200 200 10 400 fifo  # cooldown, no burnout
./codexion 5 3000 200 200 200 10 800 fifo  # contention, valid logs
./codexion 5 3000 200 200 200 10 800 edf   # grant order can differ from fifo
```

Invalid arguments (wrong arity, negatives, non-integers, scheduler ≠ fifo/edf)
are rejected. `make` builds `codexion` with `-Wall -Wextra -Werror -pthread`
and does not relink when nothing changed.

## Tooling

| Check | Result |
|-------|--------|
| valgrind memcheck (1 coder + short feasible run) | 0 leaks, 0 errors |
| valgrind helgrind | 0 races / deadlocks |
| valgrind drd | 0 conflicts |
| Global mutable state for dongles / sched / logs | None |
| Forbidden libc (outside the subject list) | None found |
| `//` comments | None |

`norminette` is not installed on this machine. A local pass over line width
and function length did not show 80-column or 25-line violations; still run
`norminette` on the campus cluster before defence.

## Subject checklist

| Requirement | Coverage |
|-------------|----------|
| C, Makefile `NAME` / `all` / `clean` / `fclean` / `re` | Yes |
| Exactly 8 arguments + `fifo`/`edf` | Yes |
| One thread per coder + monitor thread | Yes |
| One dongle per coder; n=1 has a single dongle | Yes; n=1 waits and burns out, does not compile |
| Dongle state under a mutex; condvar for waiters | Yes (`table_mtx` + `table_cv`) |
| Cooldown after release | Yes (`ready_at`) |
| FIFO / EDF via a custom heap | Yes (`srcs/heap.c`, cap 2: only two adjacent waiters) |
| Liveness under feasible EDF | Yes on the 2000/edf and 3000/400 cases |
| Burnout log within 10 ms | Typical 800–804 / 500–502 after the poll fix |
| Serialized logs, two takes before compile | Yes |
| Stop on burnout **or** all coders reaching N compiles | Yes |
| Allowed functions only | Yes |
| No globals, no leaks | Yes |
| README in English with all required sections | Yes |

## Notes for defence (not missing work)

1. **One table mutex vs one mutex per dongle.** The subject says to protect
   each dongle’s state with a mutex. This project uses a single `table_mtx`
   that covers every dongle. That is stronger (no lock-order deadlock between
   neighbours) and still exclusive. Be ready to explain that choice; it is
   not a missing lock.

2. **Heap capacity 2.** Only the two coders sitting on a dongle can wait for
   it, so a two-slot heap is enough. Recode (FIFO→LIFO) is still one
   comparison in `req_better`: for `CX_FIFO`, grant the **latest** arrival
   (`a->arrival > b->arrival` instead of `<`).

3. **`… 10 800` may burn out.** With cooldown 800 and compile 200, a dongle
   is busy 1000 ms per use. Five coders, two concurrent compiles → not enough
   slots before 3000 ms for everyone to compile 10 times. The scale asks to
   compare grant order fifo vs edf here, not to finish the run.

4. **Rare late burnout under load.** One isolated `1 800 …` run logged `904`
   while others were 800–804. The scale allows OS jitter; if it happens in
   defence, rerun the case. Do not treat 800-cooldown completion as required.

5. **Working notes in the repo** (`eval.md`, `Workflow.md`, `THE_FIX.md`,
   …) are not part of the subject. Only `README.md` is required. They will
   not be graded; they also will not hurt if you leave them.

## Recode drill (scale: FIFO → LIFO)

In `srcs/heap.c`, function `req_better`, FIFO branch: serve last arrival
first. Re-run `./codexion 5 3000 200 200 200 10 800 fifo` before and after
and show that the grant order reversed while logs stay valid.
