# HW4 — OSTEP Chapters 10, 13, 14

Zhiyuan Pan

| Chapter | Write-up | Code |
|---|---|---|
| 10. Multiprocessor Scheduling (simulation) | [`chapter10.pdf`](chapter10.pdf) ([`chapter10.md`](chapter10.md)) | [`code/chapter10/run.sh`](code/chapter10/run.sh) reruns every `multi.py` command |
| 13. The Abstraction: Address Spaces (code) | *in progress* | [`code/chapter13/`](code/chapter13/) `memory-user.c` |
| 14. Interlude: Memory API (code) | *in progress* | [`code/chapter14/`](code/chapter14/) `null.c`, `leak.c`, `overflow.c`, `use_after_free.c`, `bad_free.c`, `vector.c` |

Chapters 11 and 12 are dialogues and have no homework.

```sh
./code/chapter10/run.sh
cd code/chapter13 && make && ./memory-user 100 10   # 100 MB for 10 seconds
cd code/chapter14 && make                           # then run each program, under gdb / valgrind
```

Chapters 13 and 14 use Linux tools (`free`, `pmap`, `gdb`, `valgrind`), so run
them on Linux.
