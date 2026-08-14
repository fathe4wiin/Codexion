# `take_two_dongles`

Acquires **both** of a coder's dongles or none of them, then returns. It is the
only place a coder blocks waiting for a dongle, and the only place a `holder` is
assigned to a coder. On entry the coder holds no lock; on every path it leaves
holding none.

Files: `srcs/coder_take.c`, `srcs/dongle_wait.c`, `srcs/dongle_take.c`,
`srcs/dongle_pair.c`.

---

## 1. Full control flow

```mermaid
flowchart TD
  S["take_two_dongles(c)"] --> One{"c->left == c->right?"}
  One -->|"yes, n == 1"| WFS["wait_for_stop<br/>cond_wait until stopped"]
  WFS --> R1["return 1<br/>this coder never compiles"]
  One -->|no| L["st = 1<br/>lock table_mtx"]
  L --> W{"st != 0 and not stopped?"}
  W -->|yes| T["st = try_pair_once(c, left, right)"]
  T --> C2{"st == 2?"}
  C2 -->|"yes, our flags flipped"| BC["cond_broadcast table_cv<br/>wake the peer we may have freed"]
  C2 -->|no| C1{"st != 0?"}
  BC --> C1
  C1 -->|"yes, still empty handed"| TW["table_wait(sim, pair_wake_at(left, right))"]
  C1 -->|"no, claimed"| W
  TW --> W
  W -->|no| Fin{"st != 0?"}
  Fin -->|"yes, we were stopped"| LV["leave_pair<br/>drop both queue entries"]
  LV --> UL1["unlock table_mtx"]
  UL1 --> R2["return 1"]
  Fin -->|"no, pair is ours"| UL2["unlock table_mtx"]
  UL2 --> LOG["log_take x2<br/>printed outside the lock"]
  LOG --> R0["return 0 → do_compile"]
```

Two details the diagram encodes on purpose:

- `st == 1` sends **no** broadcast. A retry that changed nothing must not wake
  anyone, otherwise two neighbours wake each other forever.
- the two log lines happen **after** the unlock. Lock order is
  `log_mtx -> table_mtx`, so logging while holding the table lock would invert it.

---

## 2. `try_pair_once` — register, then try to claim

```mermaid
flowchart TD
  T["try_pair_once(c, a, b)"] --> J["join_pair_queues(a, b, c)"]
  J --> BR["build_req<br/>id, blocked = 0, arrival = now, deadline"]
  BR --> QA{"already in a->queue?"}
  QA -->|"yes, a retry"| K1["keep the old entry<br/>the fresh req is discarded"]
  QA -->|"no, first attempt"| PA{"heap_push into a->queue"}
  PA -->|full| RF["return 1<br/>cannot happen on a ring"]
  PA -->|ok| QB
  K1 --> QB{"already in b->queue?"}
  QB -->|yes| K2["keep the old entry"]
  QB -->|no| PB{"heap_push into b->queue"}
  PB -->|full| RB["dequeue from a<br/>return 1"]
  PB -->|ok| CM
  K2 --> CM["claim_or_mark(a, b, c)"]
```

`ensure_queued` returning early on a retry is what makes a waiter keep its
place: `arrival` and `blocked` from the first attempt survive every later pass,
so FIFO and EDF order stay stable while the coder waits.

---

## 3. `claim_or_mark` — the three outcomes

```mermaid
flowchart TD
  CM["claim_or_mark(a, b, c)"] --> UA["ua = usable_dongle(a, c)"]
  UA --> UB["ub = usable_dongle(b, c)"]
  UB --> BOTH{"ua and ub?"}
  BOTH -->|yes| CP["claim_pair<br/>dequeue both, holder = c->id on both"]
  CP --> Z["return 0"]
  BOTH -->|no| M1["blocked on a = not ub"]
  M1 --> M2["blocked on b = not ua"]
  M2 --> MV{"did either value really flip?"}
  MV -->|yes| TWO["return 2"]
  MV -->|no| ONE["return 1"]
```

`claim_pair` writes both `holder` fields inside one critical section, so no
thread can ever observe a half-taken pair. A `blocked` flag means *my other
dongle is unusable*, which is the signal that lets a peer overtake us.

---

## 4. `usable_dongle` — why a dongle is refused

```mermaid
flowchart TD
  U["usable_dongle(d, c)"] --> H{"d->holder >= 0?"}
  H -->|"yes, a neighbour owns it"| NO["not usable"]
  H -->|no| RD{"now >= d->ready_at?"}
  RD -->|"no, still cooling down"| NO
  RD -->|yes| P["priority_ok(d, c)"]
  P --> SF{"are we in this queue?"}
  SF -->|no| NO
  SF -->|yes| SZ{"queue size < 2?"}
  SZ -->|"yes, we are alone"| OK["usable"]
  SZ -->|no| Y{"req_yields(peer)?<br/>peer is blocked and younger<br/>than t_compile + cooldown"}
  Y -->|"yes, it stands aside"| OK
  Y -->|no| B{"req_better(peer, self)?"}
  B -->|"yes, peer outranks us"| NO
  B -->|no| OK
```

`req_better` is FIFO by `arrival` or EDF by `deadline`, ties broken by the lower
`coder_id`. The staleness bound in `req_yields` is what stops a bypassed waiter
from being skipped forever.

---

## 5. `table_wait` — how the coder sleeps

```mermaid
flowchart TD
  PW["pair_wake_at(a, b)"] --> MX["until = later of a->ready_at, b->ready_at"]
  MX --> F{"until > now?"}
  F -->|"no → returns 0"| CW["pthread_cond_wait<br/>suspended, mutex released,<br/>only a broadcast resumes us"]
  F -->|"yes → returns the timestamp"| SL["unlock table_mtx<br/>act_sleep until expiry<br/>relock table_mtx"]
  CW --> RE["back to the loop<br/>re-derive everything"]
  SL --> RE
```

A cooldown is a time event nobody can signal, so it is waited out unlocked. No
opportunity is lost by doing so: until that instant the coder still needs a
dongle that is not ready, so nothing arriving earlier could have helped it.
Spurious wakeups are harmless because the loop re-reads all state rather than
inferring anything from having been woken.

---

## 6. Two neighbours contending

```mermaid
sequenceDiagram
  participant C1 as Coder 1
  participant TB as Table lock and condvar
  participant C2 as Coder 2

  C1->>TB: lock, queue on d0 and d1
  TB->>C1: both usable → holder = 1
  C1->>TB: unlock, then log two takes

  C2->>TB: lock, queue on d1 and d2
  TB->>C2: d1 held by coder 1 → mark blocked on d2
  C2->>TB: broadcast (flags flipped), then cond_wait

  Note over C1: compile finishes
  C1->>TB: release_two_dongles<br/>holder = -1, arm cooldown, broadcast
  TB->>C2: woken, re-evaluates
  alt cooldown still running
    C2->>TB: unlock, act_sleep to ready_at, relock
  else ready
    TB->>C2: both usable → holder = 2
  end
```

---

## 7. Return value

| Value | Meaning | State on return |
|-------|---------|-----------------|
| `0` | pair claimed | `holder == c->id` on both dongles, two takes logged, no lock held |
| `1` | gave up because the simulation stopped, or `n == 1` | no dongle held, queue entries removed, no lock held |

`compile_cycle` treats `1` as "stop now" and ends the coder's loop; on `0` it
proceeds to `do_compile` and later to `release_two_dongles`.
