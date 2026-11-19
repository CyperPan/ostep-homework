# Chapter 32 (Common Concurrency Problems) — Homework (Code)

Zhiyuan Pan

All code is in [`code/chapter32/`](code/chapter32/): a copy of `threads-bugs/`
with these changes.

| change | why |
|---|---|
| `vector-nolock.c`: `fetch_and_add()` uses `__atomic_fetch_add()` when the CPU is not x86 | the original inline assembly (`lock; xaddl`) is x86 only, and the VM runs on an ARM CPU (aarch64). gcc 13 compiles the builtin to a call of `__aarch64_ldadd4_acq_rel` (checked with `objdump`), a helper that uses the atomic instruction `ldadd` when the CPU supports it (it does: `atomics` in `/proc/cpuinfo`) |
| `main-common.c`: time printed with `%.4f` instead of `%.2f` | most runs take 0.005–0.05 s here, so two decimals show almost nothing |
| `main-common.c`: new flag `-c` | after the run, checks that every vector is still uniform (Q9) |
| `vector-try-wait.c` with `-DBLOCK_FIRST` → `vector-try-wait-block` | Q7: the first lock taken with `pthread_mutex_lock()` |
| `ch32_exp.py`, `run.sh` | run every experiment, output in `data/` |

**Machine.** Everything was run in the course VM: Ubuntu 24.04.5 LTS
(kernel 7.0.0-38, aarch64) in VirtualBox 7.2 with **4 virtual CPUs** and
8 GB of memory, on a Mac with an Apple M4. Compiled with `gcc -O` as in the
original Makefile (gcc 13.3). With 8 or 16 threads there are more threads
than CPUs. **Times** are the median
of 5 runs, as reported by the program's own `-t`. **Deadlock detection:** a run
of `vector-deadlock` (Q1–Q3) that has not finished after 1.5 s counts as
deadlocked and is killed. Its runs that finish take well under a second.
The timed runs of the other versions (Q5–Q10) get 120 s.

---

## Q1

> First let's make sure you understand how the programs generally work, and
> some of the key options. Study the code in `vector-deadlock.c`, as well as
> in `main-common.c` and related files. Now, run `./vector-deadlock -n 2 -l 1
> -v`, which instantiates two threads (`-n 2`), each of which does one vector
> add (`-l 1`), and does so in verbose mode (`-v`). Make sure you understand
> the output. How does the output change from run to run?

**How the program works.** `main()` initializes 200 vectors: `v[i]` has 100
elements, all equal to `i`, and its own mutex. It then starts `-n` threads.
Each thread calls `vector_add(&v[0], &v[1])` (`v[0] += v[1]`, element by
element) `-l` times. `vector_add()` in `vector-deadlock.c` locks the
destination, then the source, adds, and unlocks both. With `-v`, a thread
prints `->add(a, b)` before the call and `<-add(a, b)` after it. Thread *i*
indents its lines by *i* × 14 spaces, so each column is one thread.

Output of 200 runs, sorted by how often each one appeared:

```
176 runs:                 22 runs:                  2 runs (1 each):
->add(0, 1)                             ->add(0, 1)   ->add(0, 1)
<-add(0, 1)                             <-add(0, 1)                 ->add(0, 1)
              ->add(0, 1) ->add(0, 1)                 <-add(0, 1)
              <-add(0, 1) <-add(0, 1)                               <-add(0, 1)
```

(The third column is one of the two runs where the calls overlapped; in the
other one, thread 1 entered first.)

- Mostly only the **order of the two threads** changes. Thread 0 (left
  column) usually goes first because it is created first, but in 22 of 200
  runs thread 1 started and finished before thread 0 did anything.
- In 198 of 200 runs one thread's whole add came before the other's. One
  `vector_add()` of 100 integers takes well under a microsecond, much less
  than the time to create the next thread. In **2 runs the two calls
  overlapped**: both threads printed `->add` before either printed `<-add`.
  Both calls are `add(0, 1)` and lock in the same order (v0 then v1), so the
  second one simply waited for the first. Without `-d` there is no deadlock.

---

## Q2

> Now add the `-d` flag, and change the number of loops (`-l`) from 1 to
> higher numbers. What happens? Does the code (always) deadlock?

With `-d`, thread 1 calls `vector_add(&v[1], &v[0])`: it locks v1 first and
then v0, while thread 0 locks v0 first and then v1. If thread 0 holds v0 and
thread 1 holds v1, each waits forever for the other's lock. This is the
**circular wait** of the chapter.

| `-n 2 -d -l` | 1 | 10 | 100 | 1 000 | 10 000 | 100 000 |
|---|---|---|---|---|---|---|
| runs that deadlocked | 0 / 50 | 1 / 50 | 2 / 50 | **16 / 50** | **49 / 50** | **50 / 50** |

- **It does not always deadlock.** A deadlock needs one particular
  interleaving: each thread must get its first lock in the short window
  between the other thread's first and second lock. With `-l 1` the two
  threads rarely run at the same time at all (Q1). No run deadlocked here,
  but in Q3 one run of 20 with `-n 2 -l 1` did.
- **The more loops, the more likely**, because every loop is another
  chance for that interleaving. From about 1000 loops on, the two threads
  run at the same time for a while, and the chance that some loop hits the
  window grows quickly: 16 of 50 runs at 1 000 loops, 49 of 50 at 10 000,
  all 50 at 100 000.
- A deadlocked process just stays there at 0% CPU (checked with `ps`: state
  `S`, 0.0% CPU, CPU time 0:00.00 after 3 s). `pthread_mutex_lock()`
  sleeps and does not spin, and nothing ever wakes it up.

---

## Q3

> How does changing the number of threads (`-n`) change the outcome of the
> program? Are there any values of `-n` that ensure no deadlock occurs?

With `-d`, even-numbered threads add (v0, v1) and odd-numbered threads add
(v1, v0). 20 runs each:

| `-d -l` | `-n 1` | 2 | 3 | 4 | 8 | 16 | 32 |
|---|---|---|---|---|---|---|---|
| 1 | 0 | 1 | 0 | 0 | 0 | 0 | 0 |
| 100 | 0 | 1 | 5 | 5 | 11 | 11 | **18** |
| 10 000 | 0 | **20** | **20** | **20** | **20** | **20** | **20** |

(Number of the 20 runs that deadlocked. Without `-d`: 0 of 20 with `-n 8
-l 10000`.)

- **More threads make deadlock more likely.** There are more pairs of an
  even and an odd thread, and so more chances to hit the window. With 100
  loops the rate grows from 1/20 (2 threads) to 18/20 (32 threads). With
  10 000 loops two threads are already enough for every run to deadlock.
- **`-n 1` (or 0) guarantees that no deadlock occurs.** With one thread
  there is nobody to wait for: the thread always gets both locks. (A
  thread can deadlock with itself only by locking the same mutex twice, see
  Q4, and `main()` always passes two different vectors.) For any `-n` ≥ 2
  with `-d` there is at least one even and one odd thread, so a deadlock is
  possible, even though with `-l 1` it is rare.

---

## Q4

> Now examine the code in `vector-global-order.c`. First, make sure you
> understand what the code is trying to do; do you understand why the code
> avoids deadlock? Also, why is there a special case in this `vector_add()`
> routine when the source and destination vectors are the same?

```c
void vector_add(vector_t *v_dst, vector_t *v_src) {
    if (v_dst < v_src) {               // lower address first
        Pthread_mutex_lock(&v_dst->lock);
        Pthread_mutex_lock(&v_src->lock);
    } else if (v_dst > v_src) {
        Pthread_mutex_lock(&v_src->lock);
        Pthread_mutex_lock(&v_dst->lock);
    } else {                           // the same vector: lock it once
        Pthread_mutex_lock(&v_src->lock);
    }
    ...
}
```

**Why it avoids deadlock.** The code always takes the lock of the vector
with the **lower address** first, whichever is source and whichever is
destination. Addresses give one **total order** of all locks, and every
thread acquires locks in increasing order. A circular wait would need thread
A to hold lock *x* and wait for *y* > *x*, while thread B holds *y* and
waits for *x* < *y*. That means B took *y* before *x*, against the order.
Without a cycle, one of the four conditions for deadlock (circular wait) is
gone. With `-d`, `add(0, 1)` and `add(1, 0)` now both lock v0 first, so they
cannot deadlock (Q5 runs it 100 000 times without a hang).

**Why the special case.** If `v_dst == v_src` (for example
`vector_add(&v[3], &v[3])`, doubling a vector), neither `<` nor `>` holds.
Without the third branch the code would lock the *same* mutex twice.
A default pthread mutex is not recursive, so the second
`pthread_mutex_lock()` waits for the lock the thread already holds, and the
thread deadlocks with itself. I checked this in the VM: `pthread_mutex_trylock`
on a mutex the thread already holds returns `EBUSY`, and a second
`pthread_mutex_lock` never returned (the test program was killed by its
2-second `alarm()`). The unlock code has the same special case: it unlocks
`v_dst` only if it differs from `v_src`, so the one lock is not released
twice.

---

## Q5

> Now run the code with the following flags: `-t -n 2 -l 100000 -d`. How
> long does the code take to complete? How does the total time change when
> you increase the number of loops, or the number of threads?

`./vector-global-order -t -n 2 -l 100000 -d` takes **0.0145 s**. It never
deadlocked.

| `-n 2 -d`, loops | 100 000 | 200 000 | 400 000 | 800 000 |
|---|---|---|---|---|
| time | 0.0129 s | 0.0243 s | 0.0478 s | 0.1238 s |

**Loops: the time is proportional to the number of loops.** Doubling `-l`
doubles the time (roughly). Each loop is one more `vector_add()` at the same
cost.

| `-l 100000 -d`, threads | 1 | 2 | 4 | 8 | 16 |
|---|---|---|---|---|---|
| time | 0.0047 s | 0.0123 s | 0.0481 s | 0.1296 s | 0.2361 s |
| time per `vector_add()` (time / (n × loops)) | 47 ns | 62 ns | 120 ns | 162 ns | 148 ns |

**Threads: the time grows faster than the work.** With *n* threads there is
*n* times as much work, but all threads add the same two vectors and need
the same two locks. So the adds cannot run in parallel at all: they happen
one at a time, like with one thread. On top of that, every add gets slower
with more threads (47 → 162 ns). Each add has to take the locks and the
vectors' data from another CPU's cache, and a thread that finds a lock taken
goes to sleep and must be woken up again. 16 threads do 16× the work of one
thread in **50×** the time. Beyond 4 threads the cost per add stops growing
(162 and 148 ns): the VM has only 4 virtual CPUs, so at most 4 threads
compete at any moment, and the others are not running.

---

## Q6

> What happens if you turn on the parallelism flag (`-p`)? How much would
> you expect performance to change when each thread is working on adding
> different vectors (which is what `-p` enables) versus working on the same
> ones?

With `-p`, thread *i* adds vectors 2*i* and 2*i*+1, so no two threads ever need
the same lock. `-d` then only swaps the order inside a thread's own pair and
cannot cause contention or deadlock. **Expectation:** the threads no longer
wait for each other. If there are enough cores, the time should stay at the
1-thread time, since each core does one thread's 100 000 adds. That is *n*
times faster than without `-p`, where the adds take turns. Once there are
more threads than cores, the time should grow again.

| `-l 100000 -d`, threads | 1 | 2 | 4 | 8 | 16 |
|---|---|---|---|---|---|
| without `-p` | 0.0044 s | 0.0136 s | 0.0475 s | 0.1304 s | 0.2572 s |
| **with `-p`** | 0.0046 s | **0.0048 s** | **0.0109 s** | **0.0190 s** | **0.0299 s** |
| speedup from `-p` | 1.0× | 2.8× | 4.4× | 6.9× | **8.6×** |

- With `-p` the time is **almost flat from 1 to 2 threads** (0.0046 →
  0.0048 s), as expected. Beyond 4 threads it grows (0.019 s with 8, 0.030 s
  with 16), because only 4 threads can run at a time on 4 virtual CPUs.
  16 threads on 4 CPUs is 4 rounds of 4 threads, so I expected about
  4 × 0.0046 = 0.018 s. The measured 0.030 s includes time slicing and
  creating 16 threads.
- At 4 threads (0.0109 s) it is not flat either, although there are 4
  virtual CPUs. The VM's CPUs are threads of the host, which also runs the
  VM itself and other programs. I did not investigate this further.
- The speedup from `-p` grows with the number of threads (8.6× at 16). It is
  **larger than *n*** up to 4 threads (4.4× at 4 threads). Without `-p` the
  adds are not only serialized, each one also becomes slower (Q5), and with
  `-p` that cost disappears.
- So the lesson of the chapter holds in both directions. Avoiding deadlock
  does not make threads that share locks fast. Not sharing locks at all
  makes them fast.

---

## Q7

> Now let's study `vector-try-wait.c`. First make sure you understand the
> code. Is the first call to `pthread_mutex_trylock()` really needed? Now run
> the code. How fast does it run compared to the global order approach? How
> does the number of retries, as counted by the code, change as the number
> of threads increases?

```c
  top:
    if (pthread_mutex_trylock(&v_dst->lock) != 0)
        goto top;                         // spin until we have the first lock
    if (pthread_mutex_trylock(&v_src->lock) != 0) {
        retry++;
        Pthread_mutex_unlock(&v_dst->lock);  // can't get the second: give
        goto top;                            // back the first and start over
    }
```

The code avoids deadlock by breaking **hold-and-wait**: a thread never
*waits* while holding a lock. If it cannot get the second lock immediately,
it gives the first one back and starts over.

**Is the first `trylock` needed? No.** When a thread waits for its first
lock it holds no lock, so it cannot be part of a circular wait. It may
simply block in `pthread_mutex_lock()`. Only the second acquisition, which
happens while holding the first lock, must not block. The first `trylock`
with `goto top` is just a busy-wait loop for the same lock. I built the
variant `vector-try-wait-block` (`-DBLOCK_FIRST`) that takes the first lock
with `Pthread_mutex_lock()`. It never deadlocked either.

Results (median of 5 runs, `-l 100000`; "retries" is the code's own counter,
from a separate set of runs):

| threads | 1 | 2 | 4 | 8 | 16 |
|---|---|---|---|---|---|
| global order, `-d` | 0.0043 s | 0.0120 s | 0.0487 s | 0.1338 s | 0.2511 s |
| **try-wait**, `-d` | 0.0052 s | 0.0113 s | **0.2765 s** | **0.2838 s** | **2.0036 s** |
| try-wait retries | 0 | 32 877 | **1 850 549** | **1 489 205** | **16 954 192** |
| try-wait-block, `-d` | 0.0046 s | 0.0177 s | 0.0537 s | 0.2834 s | 0.7718 s |
| try-wait-block retries | 0 | 2 529 | 96 594 | 2 807 023 | 8 041 968 |
| with `-p`: global order / try-wait / try-wait-block | 0.0043 / 0.0049 / 0.0050 s | 0.0043 / 0.0048 / 0.0048 s | 0.0074 / 0.0083 / 0.0086 s | 0.0123 / 0.0129 / 0.0139 s | 0.0239 / 0.0248 / 0.0253 s |

- **Compared with global order:** with 2 threads try-wait is as fast
  (0.0113 vs 0.0120 s). **With more threads it is 2–8× slower**: 5.7× with
  4 threads, 2.1× with 8, 8.0× with 16 (2.0 s instead of 0.25 s). It is also
  unpredictable: with 8 threads it took between 0.18 and 0.40 s in the four
  sets of runs in `data/`, and in two of them it was faster than with
  4 threads. With more threads than the 4 CPUs, only 4 run at a time, and
  how often two of them collide depends on the scheduling.
- **Retries grow quickly with the number of threads:** 32 877 with 2
  threads, 1.85 million with 4, 17 million with 16, which is about 11 failed
  attempts per successful `vector_add()`. Every failed attempt takes a lock
  and gives it up again without doing work. The threads keep each other
  busy without making progress. This is the **livelock** the chapter warns
  about: two threads each get their first lock, both fail on the second,
  both release, and both try again at the same moment. The count is only
  approximate, because `retry++` is not protected by a lock and concurrent
  increments can be lost. The real number is, if anything, higher.
- **Blocking on the first lock helps, but less than I expected:** 0.054 s
  instead of 0.28 s with 4 threads, 0.77 s instead of 2.0 s with 16, and the
  same with 8. It has fewer retries because the threads that cannot get the
  first lock sleep instead of spinning. It is still slower than global order
  with 8 and 16 threads (0.77 vs 0.25 s).
- **With `-p` there are no retries at all** (no lock is ever taken by another
  thread), and all three versions run at about the same speed.

---

## Q8

> Now let's look at `vector-avoid-hold-and-wait.c`. What is the main problem
> with this approach? How does its performance compare to the other
> versions, when running both with `-p` and without it?

```c
pthread_mutex_t global = PTHREAD_MUTEX_INITIALIZER;

void vector_add(vector_t *v_dst, vector_t *v_src) {
    Pthread_mutex_lock(&global);           // all lock acquisition is atomic
    Pthread_mutex_lock(&v_dst->lock);
    Pthread_mutex_lock(&v_src->lock);
    Pthread_mutex_unlock(&global);
    ...
}
```

Deadlock is avoided because a thread takes both of its locks as one atomic
step, under `global`. No thread can get one of the two locks while another
thread is between its first and second lock.

**The main problem: the global lock serializes every lock acquisition in the
program**, even for threads that need completely different locks. The fine
per-vector locks were meant to let unrelated operations run in parallel. Now
every `vector_add()` in the program first queues on one lock, and a thread
that holds `global` while it waits for a vector lock blocks all the others
too, even threads that need other vectors. (The chapter also mentions that
the code must know all the locks it needs up front, which is often not
possible, and that it breaks encapsulation.)

Median of 5 runs, `-l 100000`:

| threads | 1 | 2 | 4 | 8 | 16 |
|---|---|---|---|---|---|
| `-d`: global order | 0.0049 s | 0.0137 s | 0.0525 s | 0.1240 s | 0.2392 s |
| `-d`: try-wait | 0.0048 s | 0.0106 s | 0.2619 s | 0.1843 s | 1.5442 s |
| `-d`: **avoid-hold-and-wait** | 0.0059 s | 0.0361 s | 0.1016 s | 0.1796 s | 0.3341 s |
| `-d -p`: global order | 0.0042 s | 0.0048 s | 0.0076 s | 0.0123 s | 0.0240 s |
| `-d -p`: try-wait | 0.0047 s | 0.0054 s | 0.0084 s | 0.0136 s | 0.0247 s |
| `-d -p`: **avoid-hold-and-wait** | 0.0055 s | **0.0145 s** | **0.0304 s** | **0.0610 s** | **0.1135 s** |

- **Without `-p`** all threads need the same two locks anyway, so they are
  serialized no matter what. Here avoid-hold-and-wait is 1.2–2.6× slower
  than global order, because each add takes a third, also contended, lock.
  It is faster than try-wait with 4 and 16 threads (0.10 vs 0.26 s, 0.33 vs
  1.54 s).
- **With `-p`** the problem is clear. The other versions take 0.004 →
  0.024 s from 1 to 16 threads, and the threads never meet. avoid-hold-and-wait
  is **3× slower already with 2 threads** (0.0145 vs 0.0048 s) and **4.7×
  slower with 16** (0.11 s). All threads still meet at `global`, although
  they do not share a single vector. This is the version where `-p` helps
  least: 2.9× instead of 10× (global order) with 16 threads.

---

## Q9

> Finally, let's look at `vector-nolock.c`. This version doesn't use locks at
> all; does it provide the exact same semantics as the other versions? Why or
> why not?

```c
void vector_add(vector_t *v_dst, vector_t *v_src) {
    for (int i = 0; i < VECTOR_SIZE; i++)
        fetch_and_add(&v_dst->values[i], v_src->values[i]);
}
```

**No.** In the locked versions a whole `vector_add()` is atomic: no other
thread sees or changes either vector until all 100 elements have been
added. Here only **each single element's addition** is atomic. The vector
operation as a whole is not, which allows two differences:

1. **Different elements can see the adds in different orders.** If thread A
   does `v0 += v1` while thread B does `v1 += v0`, element 0 may see A's
   add first and element 1 may see B's add first. The result matches no
   order of whole `vector_add()` calls.
2. **The source is read without any protection.** `v_src->values[i]` is a
   normal load, and the atomic add happens after it. Another thread may
   change the source element in between. A thread may also see a vector
   half-updated, with some elements already added and others not.

**Measurement (flag `-c`).** Every vector starts *uniform* (all 100
elements equal: `v[i]` is all `i`). Any sequence of whole `vector_add()`
calls keeps it uniform, since every element undergoes the same additions.
`-c` checks this for all vectors and also prints `v[0].values[0]` and
`v[1].values[0]`.
In addition, the lock-based versions can **never** end with `v[0]` and `v[1]`
both all zeros. Element by element, every whole add multiplies the pair
(v0, v1) by a matrix with determinant 1, such as (1 1 / 0 1), so the pair
(0, 1) can never become (0, 0), even modulo 2³². 20 runs per configuration
("zero" means `v[0].values[0] = v[1].values[0] = 0`; when the vectors are
also uniform, both are entirely zero):

| version | `-n 2 -d`, `-l 1000` | `-n 2 -d`, `-l 100000` | `-n 4 -d`, `-l 1000` | `-n 4 -d`, `-l 100000` | `-n 8 -d` |
|---|---|---|---|---|---|
| global order, try-wait, try-wait-block, avoid-hold-and-wait | uniform | uniform | uniform | uniform | uniform |
| **nolock**: not uniform | 0 / 20 | **11 / 20** | **20 / 20** | 0 / 20 | **20 / 20** with `-l 1000`, 0 / 20 with `-l 100000` |
| **nolock**: zero | 0 / 20 | **14 / 20** | 2 / 20 | **20 / 20** | 8 / 20 with `-l 1000`, **20 / 20** with `-l 100000` |

(The four locked versions were uniform and never all zeros in every run
and configuration, also without `-d`. For nolock without `-d`, see below.)

- **With `-d`, nolock produces results that no correct order of whole
  adds can produce.** In one run (`-n 4 -l 1000 -d`), for example,
  `v[0].values[0] = -338156032` but `v[0].values[1] = -1433001792`. With 2
  threads and only 1000 loops the two threads rarely overlap, and all 20 runs
  happened to be correct.
- **With many adds, nolock ends with both vectors all zero** (every run with
  4 or 8 threads and 100 000 loops). That is why the uniformity check alone
  said "yes" there: all zeros are uniform. The
  zeros come from difference 2 above. Two threads that add v0 += v1 and
  v1 += v0 at the same time can both read the *old* values (x, y), and the
  element becomes (x + y, x + y). Unlike a whole add, this step loses
  information: its matrix (1 1 / 1 1) has determinant 0. Count the factors
  of 2 that x and y have in common. A whole add never changes that count,
  because its matrix has determinant 1, which is odd. A step to
  (x + y, x + y) can only raise it (two odd numbers have an even sum). So
  over many adds the count only goes up. Once it reaches 32, both 32-bit
  numbers are 0, and they stay 0. A whole-vector add cannot do this, as
  shown above.
- **Without `-d`** (all threads do `v0 += v1`) nolock gives the same result as
  the locked versions in every run. Only v0 changes, and additions of a
  constant v1 commute, so the order does not matter. The semantics differ
  only when operations that depend on each other run at the same time.

So nolock gives the same final result only when the order of the additions
does not matter. It does not give **atomicity of the whole operation**.

---

## Q10

> Now compare its performance to the other versions, both when threads are
> working on the same two vectors (no `-p`) and when each thread is working
> on separate vectors (`-p`). How does this version perform?

Median of 5 runs, `-l 100000`, all with `-d`:

| threads | 1 | 2 | 4 | 8 | 16 |
|---|---|---|---|---|---|
| global order | 0.0040 s | 0.0124 s | 0.0546 s | 0.1500 s | 0.2788 s |
| try-wait | 0.0043 s | 0.0114 s | 0.2621 s | 0.4045 s | 1.7211 s |
| try-wait-block | 0.0053 s | 0.0172 s | 0.0495 s | 0.2553 s | 0.7658 s |
| avoid-hold-and-wait | 0.0047 s | 0.0249 s | 0.0788 s | 0.1255 s | 0.2562 s |
| **nolock** | **0.0144 s** | 0.0321 s | 0.1066 s | 0.1717 s | 0.3451 s |
| *with `-p`:* | | | | | |
| global order | 0.0044 s | 0.0050 s | 0.0079 s | 0.0143 s | 0.0230 s |
| try-wait | 0.0050 s | 0.0055 s | 0.0086 s | 0.0140 s | 0.0257 s |
| try-wait-block | 0.0053 s | 0.0054 s | 0.0089 s | 0.0151 s | 0.0283 s |
| avoid-hold-and-wait | 0.0070 s | 0.0164 s | 0.0343 s | 0.0654 s | 0.1238 s |
| **nolock** | **0.0166 s** | **0.0184 s** | **0.0180 s** | **0.0365 s** | **0.0722 s** |

- **One thread: nolock is by far the slowest version (0.0144 vs 0.0040 s,
  3.6×).** A locked add does 2 lock and 2 unlock operations and then 100
  plain additions. nolock does **100 atomic read-modify-write operations**,
  here each one a call of the helper `__aarch64_ldadd4_acq_rel`, which then
  executes `ldadd`. Each of them is much more expensive than a plain add,
  even without contention, and the compiler cannot vectorize them.
- **Same two vectors (no `-p`): nolock is slower than global order at every
  thread count**: 2.6× with 2 threads, 2.0× with 4, and 1.1–1.2× with 8 and
  16. All threads hammer the same 100 integers. The cache lines holding them
  move between CPUs on every atomic add, i.e., 100 times per `vector_add()`
  instead of a few times for a lock. It is faster than try-wait from 4
  threads on (0.35 vs 1.72 s with 16), and it can never deadlock or
  livelock.
- **Separate vectors (`-p`): nolock scales best, but is still slowest in
  absolute time.** From 1 to 4 threads its time rises only 8% (0.0166 →
  0.0180 s), while global order rises 80% (0.0044 → 0.0079 s). Without
  shared data, an atomic add on a cache line the CPU already owns is cheap,
  and there is no lock state to keep coherent. But it starts 3.8× slower,
  and it is still 3× slower with 16 threads (0.072 vs 0.023 s). With more
  threads than the 4 CPUs, its time doubles from 4 to 8 and again to 16
  threads, as expected when the work just waits for a free CPU.

**Summary.** The lock-free version is not automatically faster. Without
contention it pays for 100 atomic operations per add, and with contention
it bounces 100 integers between caches instead of one lock. In exchange it
cannot deadlock, and it scales best when threads do not share data, at the
price of weaker semantics (Q9). In the VM it was never the fastest version.
Taken together, the cheapest fix for deadlock here is lock ordering (global
order), which was the fastest or close to the fastest locked version in
almost every column. Plain try-wait livelocks. Blocking on its first lock
helps, but not enough. avoid-hold-and-wait serializes everything. The
biggest gain in all tables, however, comes from `-p`, i.e., from threads not
sharing data at all.
