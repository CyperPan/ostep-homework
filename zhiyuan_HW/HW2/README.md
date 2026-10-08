# HW2 — OSTEP Chapters 5, 6, 7

Zhiyuan Pan

| Chapter | Write-up | Code |
|---|---|---|
| 5. Process API (simulation) | [`chapter5.pdf`](chapter5.pdf) | [`code/chapter5/simulation.sh`](code/chapter5/simulation.sh) reruns every `fork.py` command |
| 5. Process API (code) | [`chapter5_code.pdf`](chapter5_code.pdf) | [`code/chapter5/`](code/chapter5/) `q1.c` … `q8.c` |
| 6. Limited Direct Execution (measurement) | [`chapter6.pdf`](chapter6.pdf) | [`code/chapter6/`](code/chapter6/) `syscall.c`, `ctxswitch.c` |
| 7. Scheduling: Introduction (simulation) | [`chapter7.pdf`](chapter7.pdf) | [`code/chapter7/run.sh`](code/chapter7/run.sh) reruns every `scheduler.py` command |

## Building and running (Linux)

```sh
make                          # builds code/chapter5 (-g) and code/chapter6 (-O2)

cd code/chapter5 && ./q1      # ... ./q8
cd code/chapter6 && ./syscall && ./ctxswitch same && ./ctxswitch diff

./code/chapter5/simulation.sh # chapter 5 simulation runs
./code/chapter7/run.sh        # chapter 7 simulation runs
```

`q4.c` uses `execvpe()` and `ctxswitch.c` uses `sched_setaffinity()`, both of
which are Linux-only, so build and run on Linux (tested on Ubuntu 24.04).
