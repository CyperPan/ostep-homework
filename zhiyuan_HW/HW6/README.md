# HW6 — OSTEP Chapters 18, 19

Zhiyuan Pan

| Chapter | Write-up | Code |
|---|---|---|
| 18. Paging: Introduction (simulation) | [`chapter18.pdf`](chapter18.pdf) ([`chapter18.md`](chapter18.md)) | [`code/chapter18/run.sh`](code/chapter18/run.sh) reruns every `paging-linear-translate.py` command; `paging_stats.py` counts page-table entries for Q1/Q2 |
| 19. Paging: Faster Translations (TLBs) (measurement) | [`chapter19.pdf`](chapter19.pdf) ([`chapter19.md`](chapter19.md)) | [`code/chapter19/`](code/chapter19/): `tlb.c`, `timer.c`, `tlb_exp.py`, `run.sh`, graphs `tlb.png` and `tlb_conflict.png` |

```sh
./code/chapter18/run.sh
./code/chapter19/run.sh      # builds with make; ~5 minutes; pinning (-c) only works on Linux
```

Chapter 18 calls the official simulator in `../../vm-paging/` and only adds
statistics on top. Chapter 19 was measured in the Ubuntu VM (4 KB pages,
pinned) and natively on macOS (16 KB pages); the raw data are in
`code/chapter19/data/`. Plotting is skipped if matplotlib is not installed.
