# Dongle Priority Queue — FIFO vs EDF

Each USB dongle owns a **binary min-heap** wait queue (`t_dongle.queue`). When a coder cannot take the dongle immediately, they enqueue a `t_req`. Who sits at the top (`data[0]`) depends on the CLI scheduler:

```text
./codexion ... fifo   # CX_FIFO — oldest arrival first
./codexion ... edf    # CX_EDF  — earliest burnout deadline first
```

---

## 1. What lives in the queue

```c
typedef struct s_req {
    int         coder_id;   // who is waiting
    long long   arrival;    // time enqueued (get_time_ms)
    long long   deadline;   // last_compile|start_ms + t_burnout
} t_req;
```

| Field | Set by | Used by |
|-------|--------|---------|
| `coder_id` | `enqueue_waiter` | Both policies as **tie-breaker** (lower id wins) |
| `arrival` | `get_time_ms()` at enqueue | **FIFO** primary key |
| `deadline` | `get_deadline(c)` | **EDF** primary key |

```mermaid
flowchart LR
  subgraph req["t_req"]
    id["coder_id"]
    arr["arrival"]
    dl["deadline"]
  end
  enqueue["enqueue_waiter"] --> req
  req --> heap["dongle.queue min-heap"]
  heap --> top["data[0] = next to grant"]
```

### Deadline formula (`get_deadline`)

```mermaid
flowchart TD
  G["get_deadline(c)"] --> B{"last_compile != 0?"}
  B -->|yes| L["base = last_compile"]
  B -->|no| S["base = start_ms"]
  L --> D["deadline = base + t_burnout"]
  S --> D
```

Under EDF, a coder closer to burnout ranks higher in the wait heap.

---

## 2. Where the heap sits in the dongle lifecycle

```mermaid
sequenceDiagram
  participant C as Coder
  participant D as Dongle
  participant H as Wait heap

  C->>D: take_dongle (lock mtx)
  alt free + ready + empty queue or self on top
    D->>C: try_acquire → holder = c
  else contended
    C->>H: enqueue_waiter (push + sift_up)
    C->>D: wait_for_grant (cond_wait / timedwait)
  end

  Note over C,D: later, holder finishes
  C->>D: release_dongle
  D->>D: holder = -1, arm_cooldown
  D->>H: grant_next → peek/pop top
  D->>D: holder = next.coder_id, broadcast cv
```

| Function | File | Role for the queue |
|----------|------|--------------------|
| `heap_init` | `heap.c` | Alloc array; store `sched` (`CX_FIFO` / `CX_EDF`) |
| `enqueue_waiter` | `dongle_take.c` | Build `t_req`, `heap_push` |
| `try_acquire` | `dongle_take.c` | If queue nonempty, only top waiter may take |
| `grant_next` | `dongle_release.c` | Pop best request and set `holder` |
| `req_better` | `heap_sift.c` | Compare two requests under current policy |
| `heap_sift_up` / `heap_sift_down` | `heap_sift.c` | Restore heap order after push/pop |

---

## 3. Comparison rule (`req_better`)

`req_better(h, a, b)` is true when **`a` should be closer to the root than `b`**.

```mermaid
flowchart TD
  R["req_better(a, b)"] --> Sch{"h->sched == CX_FIFO?"}
  Sch -->|yes FIFO| A{"a.arrival != b.arrival?"}
  A -->|yes| FA["a.arrival < b.arrival"]
  A -->|no| FT["a.coder_id < b.coder_id"]
  Sch -->|no EDF| D{"a.deadline != b.deadline?"}
  D -->|yes| ED["a.deadline < b.deadline"]
  D -->|no| ET["a.coder_id < b.coder_id"]
```

| Policy | Primary key | Tie-break |
|--------|-------------|-----------|
| **FIFO** | smaller `arrival` wins | smaller `coder_id` |
| **EDF** | smaller `deadline` wins | smaller `coder_id` |

---

## 4. FIFO — first come, first served

FIFO ignores deadlines for ordering. Whoever **queued first** becomes next holder (after cooldown).

### Example

| Coder | arrival (ms) | deadline (stored, unused for order) |
|-------|--------------|-------------------------------------|
| 3 | 100 | 900 |
| 1 | 120 | 500 |
| 2 | 110 | 400 |

Order (best → worst): **3 → 2 → 1** (arrival 100 < 110 < 120).

```mermaid
flowchart TB
  subgraph fifoHeap["FIFO min-heap by arrival"]
    R["root: coder 3<br/>arrival=100"]
    L["coder 2<br/>arrival=110"]
    Ri["coder 1<br/>arrival=120"]
    R --> L
    R --> Ri
  end
```

```mermaid
flowchart LR
  Free["dongle free + cooled"] --> Peek["peek root: coder 3"]
  Peek --> Pop["pop coder 3"]
  Pop --> Hold["holder = 3"]
  Hold --> Next["new root: coder 2"]
```

Even if coder 1 is closer to burnout, FIFO still serves coder 3 first because they arrived earlier.

---

## 5. EDF — earliest deadline first

EDF ignores arrival for ordering. Whoever has the **soonest burnout deadline** is next.

### Same waiters, different order

| Coder | arrival | deadline |
|-------|---------|----------|
| 3 | 100 | 900 |
| 1 | 120 | 500 |
| 2 | 110 | 400 |

Order: **2 → 1 → 3** (deadlines 400 < 500 < 900).

```mermaid
flowchart TB
  subgraph edfHeap["EDF min-heap by deadline"]
    R["root: coder 2<br/>deadline=400"]
    L["coder 1<br/>deadline=500"]
    Ri["coder 3<br/>deadline=900"]
    R --> L
    R --> Ri
  end
```

```mermaid
flowchart LR
  Free["dongle free + cooled"] --> Peek["peek root: coder 2"]
  Peek --> Pop["pop coder 2"]
  Pop --> Hold["holder = 2"]
  Hold --> Next["new root: coder 1"]
```

```mermaid
flowchart TD
  Burn["t_burnout from last_compile or start_ms"] --> DL["deadline = base + t_burnout"]
  DL --> Heap["smaller deadline = higher priority"]
  Heap --> Grant["grant_next prefers coder nearest burnout"]
```

Coder 2 skips ahead of earlier arriver coder 3 — that is the point of EDF vs FIFO.

---

## 6. Side-by-side

```mermaid
flowchart TD
  W["Same waiters:<br/>coder3 arr=100 dl=900<br/>coder2 arr=110 dl=400<br/>coder1 arr=120 dl=500"]
  W --> F["FIFO"]
  W --> E["EDF"]
  F --> FN["Next holder: coder 3<br/>oldest arrival"]
  E --> EN["Next holder: coder 2<br/>earliest deadline"]
```

| | FIFO | EDF |
|--|------|-----|
| Goal | Fairness by wait time | Prefer urgency / avoid burnout |
| Primary sort | `arrival` ↑ | `deadline` ↑ |
| Same timestamps | lower `coder_id` | lower `coder_id` |
| Who benefits | Early requesters | Coders closest to `t_burnout` |

---

## 7. Heap operations (same mechanics, different compare)

### Push (enqueue)

```mermaid
flowchart TD
  P["heap_push"] --> Put["data[size] = req"]
  Put --> Up["heap_sift_up"]
  Up --> Cmp{"req_better(child, parent)?"}
  Cmp -->|yes| Sw["swap with parent, continue"]
  Cmp -->|no| Done["stop"]
  Sw --> Cmp
```

### Pop (grant / dequeue)

```mermaid
flowchart TD
  Po["heap_pop"] --> Out["out = data[0]"]
  Out --> Move["move last element to root"]
  Move --> Down["heap_sift_down"]
  Down --> Best["pick better of left/right child"]
  Best --> Need{"child better than node?"}
  Need -->|yes| Sw["swap, continue down"]
  Need -->|no| Done["stop"]
```

`heap_init(..., cap = n_coders, sched)` sizes the array for at most one waiter per coder per dongle on the normal path.

---

## 8. Fairness gate: empty queue vs top-of-heap

```mermaid
flowchart TD
  TA["try_acquire(d, c)"] --> Free{"holder free and ready?"}
  Free -->|no| No["return 0"]
  Free -->|yes| Q{"queue.size > 0?"}
  Q -->|no| Take["holder = c success<br/>fast path no waiters"]
  Q -->|yes| Top{"peek.coder_id == c->id?"}
  Top -->|no| No2["return 0 not your turn"]
  Top -->|yes| Pop["pop self, holder = c"]
```

```mermaid
flowchart TD
  GN["grant_next"] --> Empty{"queue empty?"}
  Empty -->|yes| Ret["return"]
  Empty -->|no| Cool{"dongle_ready?"}
  Cool -->|no| Sig["broadcast only<br/>waiters timedwait to ready_at"]
  Cool -->|yes| Pop["pop heap root"]
  Pop --> Hold["holder = next.coder_id"]
  Hold --> Wake["broadcast cv"]
```

1. **No waiters** → first free coder who hits `try_acquire` takes it.
2. **Waiters present** → only the current root (FIFO- or EDF-best) may become holder.

---

## 9. Functions map

```mermaid
flowchart TB
  subgraph policy["Policy decision"]
    parse["parse_scheduler → cfg.sched"]
    init["heap_init(..., sched)"]
    better["req_better uses h->sched"]
    parse --> init --> better
  end

  subgraph mutate["Heap mutate"]
    push["heap_push → sift_up"]
    pop["heap_pop → sift_down"]
    peek["heap_peek"]
  end

  subgraph use["Dongle use"]
    enq["enqueue_waiter"]
    try["try_acquire"]
    grant["grant_next"]
    enq --> push
    try --> peek
    try --> pop
    grant --> peek
    grant --> pop
  end

  better --> push
  better --> pop
```

---

## 10. Summary

- Every dongle has a **min-heap** of waiting `t_req` records.
- **FIFO**: who asked first → sort by `arrival`, then `coder_id`.
- **EDF**: who burns out soonest → sort by `deadline` (`last_compile|start_ms + t_burnout`), then `coder_id`.
- On release, after cooling down, `grant_next` gives the dongle to the current heap root under the policy chosen at startup.

Pair with [Workflow.md](Workflow.md) for the full sim lifecycle; this file focuses only on dongle queue priority.
