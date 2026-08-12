# Codexion evaluation report

Checked against `subject.pdf` (v1.5) and the scale pages `correction_page_1.pdf` (primary — matches current subject / FIFO→LIFO recode) and `correction_page_2.pdf` (older scale — C89 + EDF tie-breaker recode).

Verdict: several **hard fails** against the scale. Fix the items below before defence.

---

## Critical (would stop grading / Yes→No)

### 1. Missing `README.md` (scale: README.md file → No)

There is **no** `README.md` at the repo root. The scale and subject require all of:

- First line italicized exactly:  
  `*This project has been created as part of the 42 curriculum by <login>.*`
- Sections: **Description**, **Instructions**, **Resources** (refs + how AI was used)
- Project-specific: **Blocking cases handled**, **Thread synchronization mechanisms**
- Written in English

`Dongles.md` / `REPORT.md` / `Workflow.md` do **not** replace this.

### 2. Single coder must burn out — currently compiles forever (Easy #1 → No)

Scale / subject:

```text
./codexion 1 800 200 200 200 10 0 fifo
```

One coder, one dongle, needs **two** dongles → must **never** compile and **must** burn out around `800`.

Actual behaviour:

- Takes the **same** dongle once and compiles.
- Logs only one `has taken a dongle` before `is compiling`.
- Never burns out (runs indefinitely until killed).

Cause in `srcs/coder_compile.c`:

```c
if (c->left == c->right) // ...
    return (take_dongle(c->left, c));
```

With `n == 1`, `left == right` points at the only dongle; one successful take is enough to enter `do_compile`. Required behaviour: do **not** treat one dongle as two; wait / block until burnout (monitor prints `burned out` ~800).

Also remove the `//` comment (see Norm below).

### 3. Cooldown feasible runs burn out unreliably (Medium → at risk / No)

Scale:

```text
./codexion 5 3000 200 200 200 10 400 fifo   # must complete, no burnout
./codexion 5 3000 200 200 200 10 800 fifo   # then same with edf
```

Observed locally:

| Run | Result |
|-----|--------|
| `5 3000 … 400 fifo` ×3 | burned out in 2/3 runs (`3000` / `6606`) |
| `5 3000 … 800 fifo` | burned out at `3001` |

Correction expects no burnout and full completion. Investigate waiter wake-up on cooldown expiry (`pthread_cond_timedwait` / `grant_next`), hold-and-wait while owning one dongle, and starvation under FIFO+cooldown.

### 4. `state_mtx` never initialized or destroyed (UB / races → 0)

`t_coder.state_mtx` is locked in `do_compile` and `check_burnouts`, but:

- never `pthread_mutex_init`’d in `init_coders` / `init_sim`
- never `pthread_mutex_destroy`’d in `cleanup`

Using an uninitialized mutex is undefined behaviour. Helgrind/TSan may flag this; scale says data race / deadlock → 0.

### 5. Data race on `n_compiled` (helgrind/drd → 0 risk)

- Writers: `bump_compile` → `c->n_compiled++` with **no** lock
- Readers: `check_all_done` reads `n_compiled` with **no** lock

Protect with `state_mtx` (or another dedicated mutex) on both sides.

---

## High (Norm / preliminaries)

### 6. Forbidden `//` comment (Norme → 0)

`srcs/coder_compile.c:58` uses a C++-style `//` comment. 42 Norm requires `/* … */` only. Scale: Norm error → flag and stop.

### 7. Makefile `.PHONY` incomplete (minor, still fix)

Rules `all`, `clean`, `fclean`, `re`, `$(NAME)` exist and flags are correct (`-Wall -Wextra -Werror -pthread`). Only `.PHONY: clean` is declared — add `all`, `fclean`, `re` (and preferably `$(NAME)`).

---

## Behaviour checks that currently pass

| Case | Expected | Observed |
|------|----------|----------|
| `make` → binary `codexion` | compile clean | OK |
| No global mutable shared state | no globals for dongles/sched/log | OK |
| Args: exactly 8 + scheduler `fifo`/`edf` | reject invalid | OK |
| `5 2000 200 200 200 10 0 fifo` | no burnout, ≥10 compiles each | OK |
| `5 2000 200 200 200 7 0 edf` | no burnout | OK |
| `5 500 200 200 200 10 0 fifo` | burnout ~500, last line | OK (`500 5 burned out` last) |
| Log strings / mutex around `printf` | subject format | OK |
| Custom heap for FIFO/EDF | mandatory | OK |
| Monitor thread | mandatory | OK |
| Allowed libc/pthread APIs only | subject list | OK (no forbidden calls found) |

---

## Defence / recode readiness

### Primary scale (`correction_page_1.pdf`)

**Recode: FIFO → LIFO** — when several waiters share a dongle, grant the **latest** arrival first.

Change is in scheduler/heap compare (`req_better` for `CX_FIFO`: use `arrival >` instead of `<`, or a `CX_LIFO` mode). Must be observable under heavy contention, e.g.:

```text
./codexion 5 3000 200 200 200 10 800 fifo
```

before vs after. Rest must keep working (no duplication, serialized logs, no burnout on feasible params).

### Older scale (`correction_page_2.pdf`) — if that sheet is used

- Claims **C89** — this codebase is effectively C99 (`long long`, etc.); confirm which scale applies.
- Recode: EDF tie-breaker prefer **higher** `coder_id` (you currently prefer **lower** in `req_better`).

---

## Recommended fix order

1. Add compliant `README.md`.
2. Fix single-coder path so one dongle ≠ two hands (Easy #1).
3. Init/destroy `state_mtx`; serialize `n_compiled`.
4. Remove `//` comment; re-run Norm.
5. Fix cooldown / grant / wait logic until `5 3000 … 400/800` completes without burnout consistently.
6. Practice FIFO→LIFO heap change for the recode.

---

## Quick retest checklist (after fixes)

```bash
make re
./codexion 1 800 200 200 200 10 0 fifo          # burnout ~800, never "is compiling"
./codexion 5 2000 200 200 200 10 0 fifo         # no burnout
./codexion 5 2000 200 200 200 7 0 edf           # no burnout
./codexion 5 500 200 200 200 10 0 fifo          # burnout ~500, last line
./codexion 5 3000 200 200 200 10 400 fifo       # no burnout, completes
./codexion 5 3000 200 200 200 10 800 fifo       # no burnout; compare grant order vs edf
valgrind --tool=helgrind ./codexion 5 800 100 100 100 3 0 fifo
valgrind --leak-check=full ./codexion 5 800 100 100 100 3 0 fifo
```
