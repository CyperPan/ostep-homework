# HW3 — OSTEP Chapters 8, 9

Zhiyuan Pan

| Chapter | Write-up | Code |
|---|---|---|
| 8. Scheduling: The Multi-Level Feedback Queue (simulation) | [`chapter8.pdf`](chapter8.pdf) ([`chapter8.md`](chapter8.md)) | [`code/chapter8/run.sh`](code/chapter8/run.sh) reruns every `mlfq.py` command |
| 9. Scheduling: Proportional Share (simulation) | [`chapter9.pdf`](chapter9.pdf) ([`chapter9.md`](chapter9.md)) | [`code/chapter9/run.sh`](code/chapter9/run.sh) reruns every `lottery.py` experiment |

```sh
./code/chapter8/run.sh                     # all chapter 8 runs
./code/chapter9/run.sh                     # all chapter 9 runs (~2 min, also redraws fairness.png)
./code/chapter9/lottery_exp.py q3          # a single experiment: q2, q3, q4, q5 or all
```

- `code/chapter8/mlfq_tools.py` runs `mlfq.py -c` and either compresses the
  per-millisecond trace into `start-end job J Qp` runs, or reports how much CPU
  each job used in a time window.
- `code/chapter9/lottery_exp.py` runs `lottery.py -c` over many random seeds and
  collects statistics (fairness F, how often a job runs early). It also
  implements a stride scheduler and draws `fairness.png` (needs matplotlib).

All scripts call the official simulators in `../../cpu-sched-mlfq/` and
`../../cpu-sched-lottery/`; they only add statistics on top.
