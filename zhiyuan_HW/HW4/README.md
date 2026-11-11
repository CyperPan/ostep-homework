# HW4 — OSTEP Chapters 10, 13, 14

Zhiyuan Pan

| Chapter | Write-up | Code |
|---|---|---|
| 10. Multiprocessor Scheduling (simulation) | [`chapter10.pdf`](chapter10.pdf) ([`chapter10.md`](chapter10.md)) | [`code/chapter10/run.sh`](code/chapter10/run.sh) reruns every `multi.py` command |
| 13. The Abstraction: Address Spaces (code) | [`chapter13.pdf`](chapter13.pdf) ([`chapter13.md`](chapter13.md)) | [`code/chapter13/`](code/chapter13/) `memory-user.c`; `run.sh` reruns the `free` / `pmap` measurements |
| 14. Interlude: Memory API (code) | [`chapter14.pdf`](chapter14.pdf) ([`chapter14.md`](chapter14.md)) | [`code/chapter14/`](code/chapter14/) `null.c`, `leak.c`, `overflow.c`, `use_after_free.c`, `bad_free.c`, `vector.c`; `run.sh` runs each one normally, under gdb and under valgrind |

Chapters 11 and 12 are dialogues and have no homework.

```sh
./code/chapter10/run.sh
cd code/chapter13 && make && ./memory-user 100 10   # 100 MB for 10 seconds
./code/chapter13/run.sh                             # free / pmap measurements
./code/chapter14/run.sh                             # every program: plain, gdb, valgrind
```

Chapters 13 and 14 use Linux tools (`free`, `pmap`, `gdb`, `valgrind`), so run
them on Linux.
