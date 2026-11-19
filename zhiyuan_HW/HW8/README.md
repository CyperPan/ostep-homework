# HW8 — OSTEP Chapters 31, 32

Zhiyuan Pan

| Chapter | Write-up | Code |
|---|---|---|
| 31. Semaphores (code) | [`chapter31.pdf`](chapter31.pdf) ([`chapter31.md`](chapter31.md)) | [`code/chapter31/`](code/chapter31/): the `threads-sema/` skeletons filled in (fork/join, rendezvous, barrier, reader-writer lock, starvation-free reader-writer lock, Morris's no-starve mutex), a semaphore for macOS in `common_threads.h`, `ch31_tests.py` (runs each program many times and checks its output), `run.sh` |
| 32. Common Concurrency Problems (code) | [`chapter32.pdf`](chapter32.pdf) ([`chapter32.md`](chapter32.md)) | [`code/chapter32/`](code/chapter32/): a copy of `threads-bugs/` (with an ARM `fetch_and_add`, 4-decimal timing, a `-c` result check and a `vector-try-wait-block` variant), `ch32_exp.py` (deadlock counts with a timeout, timing tables), `run.sh` |

```sh
./code/chapter31/run.sh   # builds with make; about 15 minutes in the VM
./code/chapter32/run.sh   # builds with make; about 20 minutes (deadlocked runs time out)
```

Both chapters were run in the course VM: Ubuntu 24.04.5 LTS (aarch64) in
VirtualBox with 4 virtual CPUs, gcc 13.3. The code also builds on macOS. The
raw output of every test is in `code/chapter31/data/` and
`code/chapter32/data/`, and the write-ups quote it.
