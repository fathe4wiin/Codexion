# Codexion Workflow

End-to-end flow of the simulation: parse config → initialize world (dongles + coders) → spawn threads → run compile cycles → stop → cleanup.

CLI:

```text
./codexion n_coders t_burnout t_compile t_debug t_refactor n_compiles dongle_cd sched
# sched = fifo | edf
```

---

## 1. Top-level pipeline

```mermaid
flowchart TD
  A["main"] --> B{"parse_args?"}
  B -->|fail| Z1["return 1"]
  B -->|ok| C{"init_sim?"}
  C -->|fail| Z1
  C -->|ok| D["start_simulation"]
  D --> E["cleanup_sim"]
  E --> Z0["return 0"]
```

| Step | Function | File | Role |
|------|----------|------|------|
| Entry | `main` | `srcs/main.c` | Orchestrates the whole run |
| Parse | `parse_args` → `fill_config` / `parse_scheduler` | `srcs/parse_args.c` | Fill `t_config` from CLI |
| Init | `init_sim` | `srcs/init_sim.c` | Build shared world (arrays, mutexes, dongles, coders) |
| Run | `start_simulation` | `srcs/sim_run.c` | Stamp clock, spawn monitor + coders, join |
| Teardown | `cleanup_sim` | `srcs/cleanup.c` | Destroy sync objects and free memory |

### If conditions in `main`

| Condition | Role |
|-----------|------|
| `if (parse_args(...))` | Bad argc/values → exit before any allocation |
| `if (init_sim(...))` | Alloc or pthread init failure → exit (init already called `cleanup_sim`) |

---

## 2. Initialization (`init_sim`)

Structures and sync primitives are created here. **No threads yet.**

```mermaid
flowchart TD
  IS["init_sim"] --> G{"!sim \|\| !cfg?"}
  G -->|yes| F1["return 1"]
  G -->|no| Z["memset sim + copy cfg"]
  Z --> AS{"alloc_sim?"}
  AS -->|fail| CL["cleanup_sim → return 1"]
  AS -->|ok| SH{"init_shared?"}
  SH -->|fail| CL
  SH -->|ok| ID{"init_dongles?"}
  ID -->|fail| CL
  ID -->|ok| IC{"init_coders?"}
  IC -->|fail| CL
  IC -->|ok| OK["return 0"]
```

| Function | Role |
|----------|------|
| `alloc_sim` | `malloc` `coders[]` and `dongles[]`, zero them |
| `init_shared` | Init `stop_mtx` + `log_mtx`; `stopped = 0`; set `start_ms = 1` as “shared inited” marker |
| `init_dongles` | For each dongle: wait heap, mutex, condition variable |
| `init_coders` | Reset coder fields and link left/right dongles in a circle |

### If conditions during init

| Location | Condition | Role |
|----------|-----------|------|
| `init_sim` | `!sim \|\| !cfg` | Null guard |
| `init_sim` | any of alloc/shared/dongles/coders fails | Partial teardown via `cleanup_sim` |
| `alloc_sim` | `!sim->coders` / `!sim->dongles` | Out-of-memory; free what was allocated |
| `init_shared` | `pthread_mutex_init(&stop_mtx)` fails | Abort before log mtx exists |
| `init_shared` | `pthread_mutex_init(&log_mtx)` fails | Destroy stop mtx and abort |
| `init_one_dongle` | `heap_init` / mtx / cond fail | Destroy only what this dongle already created |
| `init_dongles` | `init_one_dongle` fails | `rollback_dongles(i - 1)` for prior successes |
| `cleanup_sim` | `if (sim->start_ms)` | Only destroy shared mutexes if `init_shared` ran |

---

## 3. Initializing coders

```mermaid
flowchart TD
  IC["init_coders"] --> G{"!sim \|\| !coders?"}
  G -->|yes| F["return 1"]
  G -->|no| R["for i: reset_coder id=i+1"]
  R --> L["link_coder_dongles"]
  L --> N{"n_coders == 1?"}
  N -->|yes| S["left = right = &dongles[0]"]
  N -->|no| C["for each i: left=&dongles[i], right=&dongles[(i+1)%n]"]
```

| Function | Role |
|----------|------|
| `reset_coder` | Set `id`, zero `n_compiled` / `last_compile`, clear dongle ptrs, store `sim*` |
| `link_coder_dongles` | Wire dining-circle topology over USB dongles |
| `init_coders` | Reset every coder, then link |

### Dongle topology

```mermaid
flowchart LR
  subgraph multi["n > 1"]
    C0["coder 1"] --> D0["dongle 0"]
    C0 --> D1["dongle 1"]
    C1["coder 2"] --> D1
    C1 --> D2["dongle 2"]
    Cn["coder n"] --> Dn["dongle n-1"]
    Cn --> D0
  end
```

### If conditions in coder init

| Condition | Role |
|-----------|------|
| `reset_coder`: `!c \|\| !sim` | Skip invalid pointer |
| `link_coder_dongles`: `!sim \|\| !coders \|\| !dongles` | Skip if world incomplete |
| `if (n == 1)` | Single-coder special case: both hands hold the **same** dongle (avoids taking twice / deadlock on one resource) |
| `else` multi path | Adjacent pair `(i, (i+1) % n)` — classic circular contention |
| `init_coders`: `!sim \|\| !coders` | Fail init if allocate never ran |

---

## 4. Starting the simulation (threads)

```mermaid
sequenceDiagram
  participant Main
  participant Mon as monitor_routine
  participant C as coder_routine

  Main->>Main: stamp_start (real start_ms)
  Main->>Mon: spawn_monitor
  Main->>C: spawn_coders × N
  Note over Mon,C: work until set_stopped
  Main->>C: join_all (coders then monitor)
```

| Function | Role |
|----------|------|
| `stamp_start` | Replace marker `start_ms=1` with wall-clock epoch (burnout / EDF base) |
| `spawn_monitor` | `pthread_create(..., monitor_routine, sim)` |
| `spawn_coders` | One thread per coder → `coder_routine` |
| `join_all` | Join all coder threads, then the monitor |
| `set_stopped` | Set global stop flag and broadcast every dongle CV so waiters wake |

### If conditions at spawn

| Condition | Role |
|-----------|------|
| `spawn_monitor`: create fails | Abort start; caller sets stopped |
| `spawn_coders`: create fails mid-loop | `set_stopped` so already-running peers exit; return error |
| `start_simulation`: `spawn_monitor \|\| spawn_coders` | Ensure stop flag + non-zero return; `main` still calls `cleanup_sim` |

---

## 5. Coder thread lifecycle

```mermaid
flowchart TD
  CR["coder_routine"] --> CK{"if (c)?"}
  CK -->|no| END["return NULL"]
  CK -->|yes| CL["coder_loop"]
  CL --> EX{"coder_should_exit?"}
  EX -->|yes| DONE["return 0"]
  EX -->|no| CC{"compile_cycle fails?"}
  CC -->|yes| DONE
  CC -->|ok| EX2{"exit?"}
  EX2 -->|yes| DONE
  EX2 -->|no| DBG["do_debug"]
  DBG --> EX3{"exit?"}
  EX3 -->|yes| DONE
  EX3 -->|no| RF["do_refactor"]
  RF --> EX
```

| Function | Role |
|----------|------|
| `coder_routine` | pthread entry; casts arg and runs loop |
| `coder_should_exit` | True if null coder/sim or `is_stopped` |
| `coder_loop` | Repeat: compile → debug → refactor until stop |
| `compile_cycle` | Take dongles → compile → release → bump count |
| `do_debug` / `do_refactor` | Log state and sleep for configured duration (`act_sleep`) |

### If conditions in the coder loop

| Condition | Role |
|-----------|------|
| `coder_should_exit`: `!c \|\| !c->sim` | Treat bad arg as “exit” (safe default) |
| `while (!coder_should_exit)` | Cooperative halt driven by monitor / spawn failure |
| `if (compile_cycle(c)) break` | Stop/take failure ends the loop (no stuck phases) |
| Exit checks before debug / after debug | Do not start the next phase after a stop was signaled |
| `coder_routine`: `if (c)` | Ignore null pthread arg |
| `do_debug` / `do_refactor`: stopped or null | Skip logging/sleep when shutting down |

---

## 6. Compile cycle and dongle ordering

```mermaid
flowchart TD
  CY["compile_cycle"] --> E{"should exit?"}
  E -->|yes| F["return 1"]
  E -->|no| T{"take_two_dongles?"}
  T -->|fail| F
  T -->|ok| DC{"do_compile interrupted?"}
  DC -->|yes| R1["release_two_dongles → return 1"]
  DC -->|ok| R2["release_two_dongles"]
  R2 --> B["bump_compile"]
  B --> OK["return 0"]
```

### Taking two dongles (deadlock avoidance)

Always acquire the **lower dongle id first** via `dongle_first`. If the second take fails, release the first.

```mermaid
flowchart TD
  TT["take_two_dongles"] --> N{"left == right?"}
  N -->|yes| One["take_dongle left only"]
  N -->|no| O{"dongle_first == 0?"}
  O -->|yes left first| L1{"take left?"}
  L1 -->|fail| F["return 1"]
  L1 -->|ok| L2{"take right?"}
  L2 -->|fail| RL["release left → return 1"]
  L2 -->|ok| OK["return 0"]
  O -->|no right first| R1{"take right?"}
  R1 -->|fail| F
  R1 -->|ok| R2{"take left?"}
  R2 -->|fail| RR["release right → return 1"]
  R2 -->|ok| OK
```

Release uses the **inverse** order of acquire (higher id released first when left was first).

| Function | Role |
|----------|------|
| `dongle_first` | `0` if `left->id <= right->id` (take left first), else `1` |
| `take_two_dongles` | Acquire one or two dongles with rollback on partial failure |
| `release_two_dongles` | Symmetric release / single-dongle shortcut |
| `do_compile` | Stamp `last_compile`, log `ST_COMPILE`, sleep `t_compile` |
| `bump_compile` | Increment `n_compiled` after a successful full cycle |

### If conditions in compile / ordering

| Condition | Role |
|-----------|------|
| `compile_cycle`: already exiting | Skip work |
| `take` fails | Abort without compiling |
| `do_compile` fails (interrupt/stop) | **Must** release held dongles before returning |
| `left == right` (take/release) | N=1 path: one take, one release |
| `dongle_first == 0` vs else | Canonical lock ordering → no circular wait between neighbors |
| Second take fails | Release first dongle so it isn’t held forever |
| `dongle_first`: null pointers | Default to `0` (safe/conservative) |
| `do_compile`: stopped | Refuse to start a compile past halt |

---

## 7. Single-dongle protocol (`take_dongle` / `release_dongle`)

Sync: per-dongle `mtx` + `cv` + wait heap (FIFO or EDF).

```mermaid
flowchart TD
  TD["take_dongle"] --> Lock["lock mtx"]
  Lock --> S{"is_stopped?"}
  S -->|yes| U1["unlock → fail"]
  S -->|no| TA{"try_acquire?"}
  TA -->|yes| U2["unlock → log_take"]
  TA -->|no| EQ{"enqueue \|\| wait_for_grant fails?"}
  EQ -->|yes| U1
  EQ -->|no| U2
```

```mermaid
flowchart TD
  RD["release_dongle"] --> Lock["lock mtx"]
  Lock --> H{"holder == c->id?"}
  H -->|no| U["unlock"]
  H -->|yes| Free["holder = -1"]
  Free --> CD["arm_cooldown"]
  CD --> GN["grant_next"]
  GN --> U
```

| Function | Role |
|----------|------|
| `dongle_ready` | True when `now >= ready_at` (cooldown elapsed) |
| `try_acquire` | Instant grant if free, cooled down, and queue empty-or-self-on-top |
| `enqueue_waiter` | Push `{coder_id, arrival, deadline}` onto heap |
| `wait_for_grant` | Cond wait / timedwait until holder or stop |
| `arm_cooldown` | Set `ready_at = now + dongle_cd` |
| `grant_next` | Pop best waiter and set `holder`, or just broadcast if still cooling |
| `signal_waiter` | `pthread_cond_broadcast` on the dongle |
| `req_better` | Heap priority: FIFO by arrival, or EDF by deadline; ties by lower `coder_id` |

### If conditions in take / release / heap

| Condition | Role |
|-----------|------|
| `take_dongle`: null args | Fail fast |
| Locked + `is_stopped` | Don’t acquire during shutdown |
| `!try_acquire` | Contended: enqueue then wait |
| `enqueue \|\| wait` fails | Unlock and return error (stop or heap/wait fail) |
| `try_acquire`: held / not ready | Cannot steal a busy or cooling dongle |
| Queue nonempty and top ≠ self | Fairness: only the scheduled next waiter may take |
| Queue nonempty and top == self | Pop self and become holder |
| `wait_for_grant` loop: while not stopped and not holder | Stay until granted or halt |
| `try_acquire` inside wait loop | Win race if free after a spurious wake |
| Not ready and `ready_at > 0` → timedwait | Wake exactly when cooldown ends |
| Else → `cond_wait` | Wait for grant/broadcast |
| Return `holder != c->id` | Fail if left the loop without ownership (typically stop) |
| `release`: `holder == c->id` | Only the owner may clear, arm cooldown, and grant |
| `grant_next`: empty queue | Nothing to schedule |
| Not ready yet | Broadcast so waiters switch to timedwait |
| `req_better` FIFO vs EDF | Selects who sits at the top of the wait heap |
| Arrival/deadline ties | Lower `coder_id` wins for determinism |

---

## 8. Monitor thread

```mermaid
flowchart TD
  MR["monitor_routine"] --> L{"sim && !is_stopped?"}
  L -->|no| END["return"]
  L -->|yes| B{"check_burnouts?"}
  B -->|yes| END
  B -->|no| D{"check_all_done?"}
  D -->|yes| ST["set_stopped → break"]
  D -->|no| SL["usleep 500"]
  SL --> L
```

| Function | Role |
|----------|------|
| `check_burnouts` | Any coder with no compile start within `t_burnout` of last compile (or `start_ms`) → stop + log burnout |
| `check_all_done` | Every coder has `n_compiled >= n_compiles` |
| `monitor_routine` | Poll ~every 500µs until a terminal condition |

### If conditions in the monitor

| Condition | Role |
|-----------|------|
| Loop: `sim && !is_stopped` | Exit if null or already stopped |
| `n_compiled < n_compiles` | Not everyone finished → keep running |
| `!last_compile` → use `start_ms` | Burnout clock starts at sim epoch until first compile |
| `now - base >= t_burnout` | Terminal failure: coder starved of compiles |
| `check_all_done` true | Success path: set stop so coder loops unwind |

---

## 9. Stop signal and logging

| Function | Role |
|----------|------|
| `is_stopped` | Read `stopped` under `stop_mtx` |
| `set_stopped` | Write `stopped=1`, broadcast **all** dongle CVs |
| `log_msg` | Serialize prints under `log_mtx` |
| `act_sleep` | Interruptible sleep; returns early if stop is set |

### If conditions

| Condition | Role |
|-----------|------|
| `log_msg`: `!is_stopped \|\| st == ST_BURNOUT` | Suppress post-stop noise, but always print the burnout line |
| `get_deadline`: `!last_compile` → `start_ms` | EDF deadline anchored like burnout base |
| `destroy_dongles`: `if (d->sim)` | Only destroy dongles that finished `init_one_dongle` (`sim` pointer set last) |

---

## 10. Cleanup

```mermaid
flowchart TD
  CS["cleanup_sim"] --> DD["destroy_dongles"]
  DD --> SH{"start_ms nonzero?"}
  SH -->|yes| DS["destroy_shared + clear start_ms"]
  SH -->|no| FS["free_sim"]
  DS --> FS
```

| Function | Role |
|----------|------|
| `destroy_dongles` | Per dongle: `heap_destroy`, mutex/cond destroy |
| `destroy_shared` | Destroy `stop_mtx` and `log_mtx` |
| `free_sim` | Free coder and dongle arrays |

---

## 11. Full call graph (from coder init onward)

```mermaid
flowchart TB
  subgraph init["Init phase"]
    main --> parse_args
    main --> init_sim
    init_sim --> alloc_sim
    init_sim --> init_shared
    init_sim --> init_dongles
    init_dongles --> init_one_dongle
    init_one_dongle --> heap_init
    init_sim --> init_coders
    init_coders --> reset_coder
    init_coders --> link_coder_dongles
  end

  subgraph run["Run phase"]
    main --> start_simulation
    start_simulation --> stamp_start
    start_simulation --> spawn_monitor
    spawn_monitor --> monitor_routine
    monitor_routine --> check_burnouts
    monitor_routine --> check_all_done
    start_simulation --> spawn_coders
    spawn_coders --> coder_routine
    coder_routine --> coder_loop
    coder_loop --> compile_cycle
    compile_cycle --> take_two_dongles
    take_two_dongles --> take_dongle
    take_dongle --> try_acquire
    take_dongle --> enqueue_waiter
    take_dongle --> wait_for_grant
    compile_cycle --> do_compile
    compile_cycle --> release_two_dongles
    release_two_dongles --> release_dongle
    release_dongle --> grant_next
    compile_cycle --> bump_compile
    coder_loop --> do_debug
    coder_loop --> do_refactor
    start_simulation --> join_all
  end

  subgraph end["Teardown"]
    main --> cleanup_sim
    cleanup_sim --> destroy_dongles
    cleanup_sim --> destroy_shared
    cleanup_sim --> free_sim
  end
```

---

## 12. Shared state (no IPC)

Everything is in-process pthread sharing of `t_sim`:

| Object | Purpose |
|--------|---------|
| `stop_mtx` + `stopped` | Global cooperative halt |
| `log_mtx` | Serialize stdout |
| Per-dongle `mtx` / `cv` / wait heap | Ownership, cooldown, FIFO/EDF queuing |
| `coder.left` / `coder.right` | Which dongles a coder must hold to compile |

Coders write `n_compiled` / `last_compile`; the monitor reads them while polling. Stop is always taken under `stop_mtx`.
