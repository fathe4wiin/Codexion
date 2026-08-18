*This project has been created as part of the 42 curriculum by bfathi.*

# Codexion

## Description

Codexion is a concurrency simulation: several coder threads sit in a circle and
share as many USB dongles as there are coders. Compiling quantum code requires
two dongles at once (left and right). After compiling, a coder debugs, then
refactors, then tries to compile again. A dongle that has just been released
stays unavailable for `dongle_cooldown` milliseconds. If a coder does not start
a compile within `time_to_burnout` milliseconds of their last compile (or of
the start of the simulation), they burn out and the run ends. The run also ends
when every coder has compiled at least `number_of_compiles_required` times.

Dongles are granted with a mandatory scheduler:

- **fifo** — the waiter whose request arrived first is served first
- **edf** — the waiter with the earliest burnout deadline
  (`last_compile_start + time_to_burnout`) is served first

The program is written in C with POSIX threads. Each coder is a thread, a
separate monitor thread watches deadlines, and a custom heap (no
standard-library priority queue) implements the scheduler on each dongle.

## Instructions

### Requirements

- A C compiler (`cc`) and POSIX threads (`-pthread`)
- Linux (uses `gettimeofday`, `usleep`, and pthreads)

### Compilation

```bash
make
```

This builds the `codexion` binary at the repository root with
`-Wall -Wextra -Werror -pthread`. Other Makefile rules: `all`, `clean`,
`fclean`, `re`.

```bash
make clean    # remove object files
make fclean   # remove objects and the binary
make re       # rebuild from scratch
```

### Execution

```text
./codexion number_of_coders time_to_burnout time_to_compile \
           time_to_debug time_to_refactor number_of_compiles_required \
           dongle_cooldown scheduler
```

All eight arguments are mandatory. Integers must be non-negative (coder count
and compiles required must be greater than zero). `scheduler` must be exactly
`fifo` or `edf`.

| Argument | Meaning |
|----------|---------|
| `number_of_coders` | Coders and dongles (one dongle between each pair) |
| `time_to_burnout` | Milliseconds allowed between compile starts |
| `time_to_compile` | How long a coder holds both dongles |
| `time_to_debug` | Debugging phase after a compile |
| `time_to_refactor` | Refactoring phase; then the coder tries to compile again |
| `number_of_compiles_required` | Stop when every coder has compiled this many times |
| `dongle_cooldown` | Extra delay after a dongle is released |
| `scheduler` | `fifo` or `edf` |

### Examples

```bash
# One coder, one dongle: cannot compile, burns out around 800 ms
./codexion 1 800 200 200 200 10 0 fifo

# Feasible run: no burnout, at least 10 compiles each
./codexion 5 2000 200 200 200 10 0 fifo

# Same idea with earliest-deadline-first
./codexion 5 2000 200 200 200 7 0 edf

# Infeasible cycle (600 ms > 500 ms burnout): must burn out around 500 ms
./codexion 5 500 200 200 200 10 0 fifo

# Cooldown: a released dongle cannot be taken again for 400 ms
./codexion 5 3000 200 200 200 10 400 fifo
```

### Log format

Each state change is one serialized line:

```text
timestamp_in_ms X has taken a dongle
timestamp_in_ms X is compiling
timestamp_in_ms X is debugging
timestamp_in_ms X is refactoring
timestamp_in_ms X burned out
```

A compile is always preceded by two `has taken a dongle` lines for that coder.
A `burned out` line is the last line of the program when the run ends by
deadline.

## Blocking cases handled

### Deadlock (hold-and-wait)

Coffman's hold-and-wait condition is refused: a coder never keeps one dongle
while waiting for the other. Both hands are claimed together under the table
lock (`claim_or_mark`). If the pair is not free, the coder stays queued on both
dongles and retries. That also avoids the cooldown starvation case where a
coder parked on a cooling dongle while blocking a neighbour with the dongle it
already held.

### Single coder / circular wait

With one coder there is only one dongle on the table, but compiling still
needs two. The coder does not treat that dongle as two hands; it waits until
the monitor reports burnout.

### Dongle duplication

Each dongle has a single `holder` id. A take sets both holders in one critical
section; a release clears them and arms cooldown. Two coders cannot observe
the same dongle as free at the same time.

### Starvation and unfair bypass

Each dongle keeps a two-slot heap (only the two adjacent coders can wait on
it). FIFO orders by arrival, EDF by deadline; equal keys break ties with the
lower coder id so the policy stays deterministic.

A waiter that cannot use a dongle yet (other hand busy or cooling) is marked
`blocked` so a neighbour that *can* compile is not stuck behind it. That bypass
is bounded: once a request is older than `time_to_compile + dongle_cooldown`,
it stops yielding (`req_yields`), so a coder cannot be skipped forever.

### Cooldown

On release, `ready_at` is set to `now + dongle_cooldown`. `usable_dongle`
refuses a dongle until that time. Waiters sleep until the pair's latest
`ready_at` so they do not busy-spin, then re-check under the table lock.

### Precise burnout detection

A dedicated monitor thread compares `now` to each coder's last compile start
(or simulation start if they have never compiled). On timeout it sets the
global stop flag and prints `burned out`. The poll interval is 200 µs so the
line can appear within 10 ms of the deadline.

### Log serialization and stop races

`printf` is wrapped by `log_mtx`, so two messages never share a line. After
stop, only the burnout line is still printed. `set_stopped` broadcasts
`table_cv` so any coder blocked in `pthread_cond_wait` wakes and exits instead
of hanging.

## Thread synchronization mechanisms

Shared state lives in `t_sim`. There are no global mutable variables for
dongles, scheduling, or logging.

| Primitive | Protects |
|-----------|----------|
| `table_mtx` + `table_cv` | Every dongle (`holder`, `ready_at`, wait heap) and `stopped` |
| `log_mtx` | Stdout; one complete log line at a time |
| `state_mtx` (per coder) | `n_compiled` and `last_compile` between the coder and the monitor |

Lock order is **`log_mtx` then `table_mtx`**. A thread never logs while it
already holds `table_mtx` (takes are logged after the table lock is dropped).
That avoids lock-order deadlocks between logging and the table.

How the pieces talk:

- **Coders ↔ dongles.** `take_two_dongles` locks the table, enqueues on both
  dongles, and either claims the pair or waits on `table_cv` /
  cooldown sleep. Release writes `ready_at`, clears `holder`, and broadcasts
  so neighbours retry.
- **Coders ↔ monitor.** The monitor reads compile counts and last-compile
  timestamps under `state_mtx`. When it decides the run is over it calls
  `set_stopped`, which takes `table_mtx`, sets `stopped`, and broadcasts
  `table_cv`. Coders sample `stopped` through `is_stopped` (same mutex) at
  phase boundaries and inside interruptible sleeps (`act_sleep`).
- **Race on `n_compiled`.** `bump_compile` increments under `state_mtx`;
  `check_all_done` reads under the same mutex, so the “everyone compiled
  enough” stop is not a data race.
- **Spurious wakeups.** Wait loops re-check `stopped` and pair availability
  after every wake rather than assuming the broadcast means “your dongles are
  ready”.

## Resources

### References

- POSIX threads: `pthread_create(3)`, `pthread_mutex_lock(3)`,
  `pthread_cond_wait(3)`, `pthread_cond_broadcast(3)`
  ([Linux man-pages](https://man7.org/linux/man-pages/dir_section_3.html))
- `gettimeofday(2)` for millisecond timestamps
  ([gettimeofday(2)](https://man7.org/linux/man-pages/man2/gettimeofday.2.html))
- Dining philosophers — same circular two-resource pattern as the dongles
  ([Wikipedia](https://en.wikipedia.org/wiki/Dining_philosophers_problem))
- Coffman conditions for deadlock
  ([Wikipedia](https://en.wikipedia.org/wiki/Deadlock#Necessary_conditions))
- Earliest Deadline First
  ([Wikipedia](https://en.wikipedia.org/wiki/Earliest_deadline_first_scheduling))

### How AI was used

AI was used as an assistant, not as a substitute for understanding the code:

- **Project skeleton.** Empty files, headers, and a first Makefile layout so
  the modules (parse, init, heap, dongles, coder loop, monitor, logger) were
  in place before the logic was filled in.
- **Parsing.** Help designing the argument parser (strict integers, scheduler
  token `fifo` / `edf`, rejection of invalid input).
- **Problem solving.** Help thinking through deadlock, cooldown starvation
  (hold-and-wait on one dongle), and how FIFO/EDF should pick the next waiter.
- **Debugging.** Later sessions used AI to read logs, compare behaviour to the
  subject, and track down races or missed wakeups.

Generated suggestions were read, tested, and rewritten as needed. The
scheduling rules, the atomic pair-take, and the monitor behaviour were
validated against the subject cases rather than accepted on trust.
