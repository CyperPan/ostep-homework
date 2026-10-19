# HW5 — OSTEP Chapters 15, 16, 17

Zhiyuan Pan

| Chapter | Write-up | Code |
|---|---|---|
| 15. Mechanism: Address Translation (simulation) | [`chapter15.pdf`](chapter15.pdf) ([`chapter15.md`](chapter15.md)) | [`code/chapter15/run.sh`](code/chapter15/run.sh) reruns every `relocation.py` command; `reloc_exp.py` runs the Q2/Q3/Q5 experiments and draws `valid_fraction.png` |
| 16. Segmentation (simulation) | [`chapter16.pdf`](chapter16.pdf) ([`chapter16.md`](chapter16.md)) | [`code/chapter16/run.sh`](code/chapter16/run.sh) reruns every `segmentation.py` command; `seg_exp.py` measures the valid fraction for Q4/Q5 |
| 17. Free-Space Management (simulation) | [`chapter17.pdf`](chapter17.pdf) ([`chapter17.md`](chapter17.md)) | [`code/chapter17/run.sh`](code/chapter17/run.sh) reruns every `malloc.py` command; `malloc_stats.py` collects free-list statistics for Q4–Q7 |

```sh
./code/chapter15/run.sh
./code/chapter16/run.sh
./code/chapter17/run.sh
```

All scripts call the official simulators in `../../vm-mechanism/`,
`../../vm-segmentation/` and `../../vm-freespace/` and only add statistics on
top. Plotting (chapter 15) is skipped if matplotlib is not installed.
