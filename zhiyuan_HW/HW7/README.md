# HW7 — OSTEP Chapters 26, 28, 29

Zhiyuan Pan

| Chapter | Write-up | Code |
|---|---|---|
| 26. Concurrency: An Introduction (simulation) | [`chapter26.pdf`](chapter26.pdf) ([`chapter26.md`](chapter26.md)) | [`code/chapter26/run.sh`](code/chapter26/run.sh) reruns every `x86.py` command, including the sweeps over `-i` and `-s` |
| 28. Locks (simulation) | [`chapter28.pdf`](chapter28.pdf) ([`chapter28.md`](chapter28.md)) | [`code/chapter28/run.sh`](code/chapter28/run.sh) reruns every experiment; `lock_stats.py` summarizes long `x86.py` traces (lost updates, mutual exclusion, spin instructions, waiting time) and runs random `-P` schedules |
| 29. Lock-based Concurrent Data Structures (code) | [`chapter29.pdf`](chapter29.pdf) ([`chapter29.md`](chapter29.md)) | [`code/chapter29/`](code/chapter29/): `timer.c`, `counter.c` (simple and approximate counters), `list.c` (one lock vs hand-over-hand), `hash.c` (hash table with 1 to one-per-bucket locks), `ch29_exp.py`, `run.sh`, graphs `*.png` |

```sh
./code/chapter26/run.sh
./code/chapter28/run.sh      # about 1 minute
./code/chapter29/run.sh mac  # builds with make; about 15 minutes
```

Chapters 26 and 28 call the official simulators in `../../threads-intro/` and
`../../threads-locks/` (two different versions of `x86.py`) and only add
statistics on top. Chapter 29 was measured natively on macOS (Apple M4,
4 performance + 6 efficiency cores). The code also builds on Linux, but I
could not run it in the Ubuntu VM: its saved state no longer matches the VM's
configuration, and I left the VM untouched. The raw data are in
`code/chapter29/data/`, and every program checks its own result (the last
field of each line is `ok`). Plotting is skipped if matplotlib is not
installed.
