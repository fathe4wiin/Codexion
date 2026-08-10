# Codexion Implementation Report

Date: 2026-08-08

## Goal

Finish the runtime path after init, following `main`’s workflow:

`parse → init → start_simulation → cleanup`

## Main workflow (now)

```
main
 ├─ parse_args
 ├─ init_sim
 │   ├─ alloc_sim
 │   ├─ init_shared
 │   ├─ init_dongles (+ heap_init per dongle)
 │   └─ init_coders
 ├─ start_simulation          ← NEW
 │   ├─ stamp_start
 │   ├─ spawn_monitor
 │   ├─ spawn_coders
 │   └─ join_all
 └─ cleanup_sim (+ heap_destroy)
```

## What was implemented

### 1. Heap (FIFO / EDF) — required by subject
| File | Role |
|------|------|
| `srcs/heap.c` | `heap_init`, `push`, `pop`, `peek`, `destroy` |
| `srcs/heap_sift.c` | sift up/down, swap, `req_better` |

- **FIFO:** earlier `arrival` wins; tie → lower `coder_id`
- **EDF:** earlier `deadline` wins; tie → lower `coder_id`
- Each dongle owns one wait heap (`cap = n_coders`)

### 2. Logger
| File | Role |
|------|------|
| `srcs/logger.c` | mutex-serialized state lines |

Messages: `has taken a dongle`, `is compiling`, `is debugging`, `is refactoring`, `burned out`

### 3. Dongle take / release + cooldown
| File | Role |
|------|------|
| `srcs/dongle_take.c` | acquire, enqueue, `cond_wait` / `timedwait` |
| `srcs/dongle_release.c` | release → arm cooldown → `grant_next` |

### 4. Coder threads
| File | Role |
|------|------|
| `srcs/coder_routine.c` | pthread entry + loop |
| `srcs/coder_compile.c` | take 2 dongles → compile → release |
| `srcs/coder_rest.c` | debug + refactor |

- Deadlock avoidance: lower dongle id first (`dongle_first`)
- 1 coder: single dongle (left == right)

### 5. Monitor
| File | Role |
|------|------|
| `srcs/monitor.c` | burnout scan (~500µs) + “all compiled enough” |

Burnout base = `last_compile` or `start_ms` if never compiled.

### 6. Simulation runner + main wiring
| File | Role |
|------|------|
| `srcs/sim_run.c` | spawn / join |
| `srcs/main.c` | calls `start_simulation` between init and cleanup |

### Supporting updates
- `init_dongles.c` — `heap_init` on each dongle
- `cleanup.c` — `heap_destroy` before mutex/cond destroy
- `util.c` — `set_stopped` broadcasts all dongle condvars
- `time_utils.c` — `act_sleep` (interruptible on stop)
- `includes/codexion.h` — prototypes for new modules
- `Makefile` — all new sources

## Smoke tests run

| Command | Result |
|---------|--------|
| `./codexion 1 800 100 50 50 2 0 fifo` | exit 0, lifecycle logs |
| `./codexion 3 400 50 50 50 2 10 edf` | exit 0, multi-coder + cooldown |
| `./codexion 4 200 150 150 150 10 0 fifo` | exit 0, ends with `burned out` |

Build: `make re` with `-Wall -Wextra -Werror -pthread` — OK

## Files added this session

```
srcs/heap.c
srcs/heap_sift.c
srcs/logger.c
srcs/dongle_take.c
srcs/dongle_release.c
srcs/coder_routine.c
srcs/coder_compile.c
srcs/coder_rest.c
srcs/monitor.c
srcs/sim_run.c
REPORT.md  (this file)
```

## Still optional / polish (not blockers for a first run)

- [ ] Subject README (mandatory for submission)
- [ ] Harden edge cases (remove aborted waiters from heap on stop)
- [ ] More thorough Norminette pass / peer eval edge cases
- [ ] Tuning sleep/monitor intervals for stricter timing

## How to run

```bash
make
./codexion number_of_coders time_to_burnout time_to_compile \
  time_to_debug time_to_refactor number_of_compiles_required \
  dongle_cooldown scheduler

# scheduler: fifo | edf
./codexion 4 410 200 200 200 5 0 fifo
```
