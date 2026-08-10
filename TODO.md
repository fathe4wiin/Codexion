# Codexion — TODO

## Done
- [x] Project layout (`includes/`, `srcs/`, Makefile)
- [x] Argument parsing (`parse_args`, `parse_number`)
- [x] Scheduler validation (`fifo` / `edf` → `CX_FIFO` / `CX_EDF`)
- [x] Shared helpers (`util.c`, `time_utils.c`)
- [x] Init + cleanup (sim / dongles / coders)
- [x] Wire `main`: parse → init → run → cleanup
- [x] Heap (FIFO / EDF priority queue)
- [x] Dongle take / release + cooldown
- [x] Coder threads (compile → debug → refactor)
- [x] Logger (serialized state messages)
- [x] Monitor thread (burnout + all-compiled stop)
- [x] `start_simulation` / join threads

## Remaining for submission polish
- [ ] README.md (subject-required sections)
- [ ] Norminette / peer-eval hardening
- [ ] Optional: purge stopped waiters from dongle heaps
