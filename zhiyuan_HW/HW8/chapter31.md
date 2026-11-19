# Chapter 31 (Semaphores) — Homework (Code)

Zhiyuan Pan

All code is in [`code/chapter31/`](code/chapter31/). The six `.c` files are the
skeletons from `threads-sema/` with the missing parts filled in. Below the
"don't change" lines of the skeletons I changed nothing.

| file | what it is |
|---|---|
| `common_threads.h` | the original header; I only added a semaphore for macOS (see below) |
| `fork-join.c`, `rendezvous.c`, `barrier.c` | Q1–Q3 |
| `reader-writer.c` | Q4: the reader-writer lock of Figure 31.13 |
| `reader-writer-nostarve.c` | Q5: the same lock plus a FIFO turnstile |
| `rw_test.h` | test code for Q4/Q5: holding times, a mutual-exclusion check, longest waits |
| `mutex-nostarve.c` | Q6: Morris's starvation-free mutex and a test that measures starvation |
| `ch31_tests.py` | runs every program many times and checks the order of its output |
| `run.sh` | builds everything (`make`) and reruns all tests; output in `data/` |

**Machine.** Everything was run in the course VM: Ubuntu 24.04.5 LTS
(kernel 7.0.0-38, aarch64) in VirtualBox 7.2 with **4 virtual CPUs** and
8 GB of memory, on a Mac with an Apple M4. Compiled with `gcc -O2` (gcc 13.3).
`run.sh` reruns everything; the raw output is in `code/chapter31/data/`.

**Semaphores.** On Linux the programs use the system's `sem_t` (glibc), via
the `Sem_init` / `Sem_wait` / `Sem_post` macros of `common_threads.h`. glibc's
semaphores give **no fairness guarantee**. `sem_wait()` takes the value with
an atomic decrement whenever it is positive. So a thread that calls
`sem_wait()` just after a `sem_post()` can take the value before the thread
that was woken up ("barging"). Q4–Q6 show that this detail decides whether a
thread can starve. (macOS declares `sem_init()` but does not implement it: it
returns −1 with `errno` = ENOSYS. So that the files also build on a Mac, I
added the Zemaphore of Figure 31.17 to `common_threads.h` under
`#ifdef __APPLE__`. It is not used in the VM.)

**How the output is checked.** "Test by adding sleep(1) calls" only shows one
run. `ch31_tests.py` runs each program hundreds of times and checks the order
of the printed lines. For the barrier it also needs a broken program on which
the check fails: `fork-join-broken` and `barrier-broken` are built from the
same sources with `-DBROKEN`, which removes the one `Sem_wait` that matters.

---

## Q1

> The first problem is just to implement and test a solution to the
> **fork/join problem**, as described in the text. Even though this solution
> is described in the text, the act of typing it in on your own is
> worthwhile; even Bach would rewrite Vivaldi, allowing one soon-to-be master
> to learn from an existing one. See `fork-join.c` for details. Add the call
> `sleep(1)` to the child to ensure it is working.

```c
sem_t s;

void *child(void *arg) {
    sleep(1);          // the child is slow: the parent must still wait for it
    printf("child\n");
    Sem_post(&s);      // tell the parent we are done
    return NULL;
}

int main(int argc, char *argv[]) {
    ...
    Sem_init(&s, 0);   // 0: nothing has happened yet, so a wait must block
    Pthread_create(&p, NULL, child, NULL);
    Sem_wait(&s);      // block until the child has posted
    printf("parent: end\n");
}
```

The semaphore starts at **0**. If the parent reaches `Sem_wait` first, the
value is 0, so the parent sleeps until the child's `Sem_post`. If the child
finishes first, its post makes the value 1, and the parent's later
`Sem_wait` returns at once. Either way, "parent: end" comes after "child".

```
parent: begin
child
parent: end              (1.00 s in total: the parent waited for the sleep)
```

| program | "child" printed before "parent: end" |
|---|---|
| `fork-join` | 10 / 10 runs |
| `fork-join-broken` (no `Sem_wait`) | 0 / 10 runs (the parent returns from `main` while the child sleeps; the program ends and "child" is never printed) |

---

## Q2

> Let's now generalize this a bit by investigating the **rendezvous
> problem**. The problem is as follows: you have two threads, each of which
> are about to enter the rendezvous point in the code. Neither should exit
> this part of the code before the other enters it. Consider using two
> semaphores for this task, and see `rendezvous.c` for details.

Two semaphores, both starting at 0: `s1` means "child 1 has arrived" and `s2`
means "child 2 has arrived". Each child **first announces its own arrival,
then waits for the other's**:

```c
void *child_1(void *arg) {                 void *child_2(void *arg) {
    printf("child 1: before\n");               printf("child 2: before\n");
    Sem_post(&s1);                             Sem_post(&s2);
    Sem_wait(&s2);                             Sem_wait(&s1);
    printf("child 1: after\n");                printf("child 2: after\n");
}                                          }
```

The order of post and wait matters. If both children waited first, each
would block until the other posts, and neither would ever post: a deadlock.
(One child may wait first, if the other posts first. That works, but it can
cost one extra context switch.)

For testing I added an optional argument: `./rendezvous 1` makes child 1
sleep one second before it arrives, and `./rendezvous 2` does the same for
child 2.

| run | correct (both "before" lines before either "after") | example order |
|---|---|---|
| no sleep | 500 / 500 | 1 before, 2 before, 2 after, 1 after |
| child 1 sleeps | 5 / 5 | 2 before, **1 before**, 1 after, 2 after |
| child 2 sleeps | 5 / 5 | 1 before, **2 before**, 2 after, 1 after |

When child 1 sleeps, child 2 prints "before" and then blocks in
`Sem_wait(&s1)` for a whole second. It prints "after" only once child 1 has
arrived.

---

## Q3

> Now go one step further by implementing a general solution to **barrier
> synchronization**. Assume there are two points in a sequential piece of
> code, called P1 and P2. Putting a barrier between P1 and P2 guarantees that
> all threads will execute P1 before any one thread executes P2. Your task:
> write the code to implement a `barrier()` function that can be used in this
> manner. It is safe to assume you know N (the total number of threads in the
> running program) and that all N threads will try to enter the barrier.
> Again, you should likely use two semaphores to achieve the solution, and
> some other integers to count things. See `barrier.c` for details.

```c
typedef struct __barrier_t {
    int num_threads;   // N
    int count;         // how many have arrived so far
    sem_t mutex;       // initialized to 1: a lock protecting count
    sem_t turnstile;   // initialized to 0: threads wait here
} barrier_t;

void barrier(barrier_t *b) {
    Sem_wait(&b->mutex);
    b->count++;
    int last = (b->count == b->num_threads);
    Sem_post(&b->mutex);

    if (last)                    // the N-th thread opens the turnstile ...
        Sem_post(&b->turnstile);
    Sem_wait(&b->turnstile);     // everyone (the N-th one too) passes it ...
    Sem_post(&b->turnstile);     // ... and lets the next one through
}
```

- `count` is shared, so it is changed under `mutex`. I copy the result of the
  comparison into the local `last` before releasing `mutex`, so exactly one
  thread sees `last == 1`.
- The **turnstile** starts at 0, so threads 1 … N−1 block in
  `Sem_wait(&turnstile)`. The N-th thread posts once. After that, each thread
  that gets through the turnstile posts again for the next one. One post
  therefore lets all N threads through, one after another.
- The N-th thread posts only after every thread has incremented `count`. So
  every "before" has been printed when the first "after" is printed.

This barrier can be used only once, which is all the program needs. At the end
`count` is N and the turnstile's value is 1, so a second call would let
everybody straight through. A reusable barrier needs a second turnstile that
closes the first one again (Downey, *The Little Book of Semaphores*, 3.6).

**Testing.** Below the "don't change" line the children print "before", call
`barrier()` and print "after". To vary the order in which threads arrive, I
added a random sleep at the start of `barrier()` (environment variable
`BARRIER_SLEEP_US`). 200 runs of each configuration:

| program | N = 1 | N = 2 | N = 5 | N = 10 | N = 50 |
|---|---|---|---|---|---|
| `barrier` | 200 / 200 | 200 / 200 | 200 / 200 | 200 / 200 | 200 / 200 |
| `barrier`, random sleep < 2 ms | 200 / 200 | 200 / 200 | 200 / 200 | 200 / 200 | 200 / 200 |
| `barrier-broken` (no turnstile wait) | 200 / 200 | **20** / 200 | **0** / 200 | **0** / 200 | **0** / 200 |
| `barrier-broken`, random sleep < 2 ms | 200 / 200 | 200 / 200 | 200 / 200 | 184 / 200 | 0 / 200 |

The correct barrier passes every run. The broken one fails almost always,
which shows that the check works. One detail in the table: the sleep makes
the broken barrier look *better*. The sleep comes after "before" (it is
inside `barrier()`). So all threads print "before" quickly, and the sleep
then delays all the "after" lines. A sleep at this place can only reveal a
barrier that lets threads out too *late*. Finding one that lets them out too
early needs the runs without the sleep.

---

## Q4

> Now let's solve the **reader-writer problem**, also as described in the
> text. In this first take, don't worry about starvation. See the code in
> `reader-writer.c` for details. Add `sleep()` calls to your code to
> demonstrate it works as you expect. Can you show the existence of the
> starvation problem?

The lock of Figure 31.13:

```c
typedef struct __rwlock_t {
    sem_t lock;        // 1: protects readers
    sem_t writelock;   // 1: held by one writer, or by the group of readers
    int readers;
} rwlock_t;

void rwlock_acquire_readlock(rwlock_t *rw) {
    Sem_wait(&rw->lock);
    rw->readers++;
    if (rw->readers == 1)        // first reader: lock out the writers
        Sem_wait(&rw->writelock);
    Sem_post(&rw->lock);
}

void rwlock_release_readlock(rwlock_t *rw) {
    Sem_wait(&rw->lock);
    rw->readers--;
    if (rw->readers == 0)        // last reader: let a writer in
        Sem_post(&rw->writelock);
    Sem_post(&rw->lock);
}

void rwlock_acquire_writelock(rwlock_t *rw) { Sem_wait(&rw->writelock); }
void rwlock_release_writelock(rwlock_t *rw) { Sem_post(&rw->writelock); }
```

**Test instrumentation (`rw_test.h`).** The given `reader()` and `writer()`
hold the lock for only one `printf`. I cannot change them, so the sleeps go
into the lock functions. After getting the lock, a reader sleeps
`RW_READ_US` and a writer sleeps `RW_WRITE_US` microseconds. The same
functions also count the readers and writers inside and stop the program if
a writer is ever inside together with anyone else. This never happened in any
run. At exit they print the longest time any reader and any writer waited in
acquire.

Readers keep their lock for 1 ms. Results (worst of 3 runs):

| readers, writers, loops each | reads before the first write | longest wait of a reader | longest wait of the writer |
|---|---|---|---|
| 5, 1, 200 | **1000 of 1000** | 0.0 ms | **224.1 ms** (the whole run: 0.22 s) |
| 10, 1, 100 | **1000 of 1000** | 0.0 ms | **112.3 ms** (the whole run: 0.11 s) |
| 5 readers and 5 writers, 100 loops, both hold 1 ms | **500 of 500** | 0.0 ms | **560.4 ms** |

**Yes, this shows starvation.** The writer is created right after the
readers and calls `acquire_writelock` almost immediately. Yet the first
"write" line appears only after all 1000 reads, i.e., when the readers have
finished their loops and quit. With five readers each holding the lock for
1 ms, there is always at least one reader inside. `readers` never drops to 0,
so `writelock` is never posted. Readers that come later join the ones
already inside, even though a writer has been waiting all along. If the
readers looped forever, the writer would wait forever.

Without the sleep (5 readers, 1 writer, 200 loops, no holding time), the
writer also came only after all 1000 reads in the worst of the 3 runs. With
2000 loops, 1297 of the 10000 reads came before the first write. So the
writer gets in only where, by chance, all readers are outside at the same
moment.

Writers can also starve **each other** with this lock. With 1 reader and
5 writers that hold the lock for 1 ms, one writer waited up to **938 ms** of
a 1.16 s run. (The reader always finished its 200 quick reads before the
writers started.) The cause is the weak semaphore: a writer that releases
`writelock` calls `Sem_wait` again in its next loop iteration, and it can
take the value before the writer that was woken up gets to run.

---

## Q5

> Let's look at the reader-writer problem again, but this time, worry about
> starvation. How can you ensure that all readers and writers eventually
> make progress? See `reader-writer-nostarve.c` for details.

**Idea:** add a **turnstile** (Downey, *The Little Book of Semaphores*,
4.2.3) that every thread passes on the way in. A reader passes straight
through it. A writer keeps it closed while it waits for the readers inside
to leave and while it writes. So once a writer is waiting, new readers queue
up at the turnstile instead of joining the readers inside. The readers inside
finish, the last one posts `writelock`, and the writer goes in. When it is
done it opens the turnstile again.

```c
void rwlock_acquire_readlock(rwlock_t *rw) {
    T_wait(&rw->turnstile);      // blocks while a writer is waiting
    T_post(&rw->turnstile);
    ... the same as in Q4 ...
}

void rwlock_acquire_writelock(rwlock_t *rw) {
    T_wait(&rw->turnstile);      // close the turnstile: no new readers
    Sem_wait(&rw->writelock);    // wait for the readers inside to leave
}

void rwlock_release_writelock(rwlock_t *rw) {
    T_post(&rw->turnstile);      // open the turnstile first ...
    Sem_post(&rw->writelock);    // ... then the room
}
```

**The turnstile itself must be fair.** My first version used a plain `sem_t`
as the turnstile. It fixed the writer starvation of Q4, but a reader could
still starve. With 1 reader and 5 writers the reader waited up to
**967 ms**: a writer that opens the turnstile and loops around can close it
again before the reader, which has been waiting on it all along, wakes up.
Downey's argument assumes that a thread waiting on a semaphore is eventually
woken up, and glibc's semaphore does not promise that. The turnstile therefore is
a **FIFO semaphore**, `fifo_sem_t`: a value plus a ticket queue, as in the
ticket lock of Chapter 28. A waiter takes a ticket and may decrement the
value only when its ticket is being served. Threads pass the turnstile
exactly in the order they arrived. The old version is still built as
`reader-writer-nostarve-weak` (`-DWEAK_TURNSTILE`).

**Why no thread starves now.**

- **A writer** reaches the front of the FIFO turnstile after the threads
  ahead of it, each of which holds it only for a bounded time. Then it waits
  only for the readers that are already inside. No new reader can join them,
  because the writer holds the turnstile.
- **A reader** also gets through the turnstile after the threads ahead of it.
  At most one writer at a time is past the turnstile (the one holding it), so
  it then waits for at most that one writer.

Results (worst of 3 runs; "first write" is the number of reads printed before
the first write):

| readers, writers, loops; holding time | lock | first write | longest wait, reader | longest wait, writer |
|---|---|---|---|---|
| 5, 1, 200; readers 1 ms | Q4 | 1000 / 1000 | 0.0 ms | **224.1 ms** |
| | weak turnstile | 5 / 1000 | 5.1 ms | 1.3 ms |
| | **FIFO turnstile** | **5** / 1000 | **1.6 ms** | **1.8 ms** |
| 10, 1, 100; readers 1 ms | Q4 | 1000 / 1000 | 0.0 ms | **112.3 ms** |
| | weak turnstile | 10 / 1000 | 6.0 ms | 1.2 ms |
| | **FIFO turnstile** | **10** / 1000 | **2.2 ms** | **2.4 ms** |
| 1, 5, 200; writers 1 ms | Q4 | 200 / 200 | 0.0 ms | **938.0 ms** |
| | weak turnstile | 200 / 200 | **967.1 ms** | 816.1 ms |
| | **FIFO turnstile** | 200 / 200 | **16.3 ms** | **17.7 ms** |
| 5, 5, 100; both 1 ms | Q4 | 500 / 500 | 0.0 ms | **560.4 ms** |
| | weak turnstile | 5 / 500 | **570.7 ms** | 458.9 ms |
| | **FIFO turnstile** | 5 / 500 | **8.7 ms** | **8.1 ms** |

- With the turnstile, the first write comes **right after the first round of
  reads** (5 reads with 5 readers, 10 with 10). In Q4 it came after all of
  them.
- With the FIFO turnstile, **no thread waits longer than about 18 ms in any
  configuration**. That is a few times the 1 ms holding time (with 5 writers
  queued, about 5 ms per round), which is what a fair queue gives. With the
  weak turnstile, a thread still waited almost a second.
- **Fairness costs some throughput.** With 1 ms holding times the FIFO
  version's total run time was 10–80% longer (0.30 vs 0.22 s for 5 readers
  and 1 writer, 1.26 vs 1.16 s for 1 reader and 5 writers). Without holding
  time it was much slower: 0.08 s instead of under 0.01 s for 200 loops,
  and 1.03 s instead of 0.01 s for 2000 loops (worst of 3 runs).
  Every `fifo_sem_post` wakes *all* waiting threads with a broadcast, and
  only the one whose ticket is served may continue. Waking threads is
  expensive in the VM. Readers also share the lock less: a reader that
  arrives after a waiting writer can no longer join the readers inside.

The mutual-exclusion check in `rw_test.h` never fired, and every run ended
with `value` = writers × loops.

---

## Q6

> Use semaphores to build a **no-starve mutex**, in which any thread that
> tries to acquire the mutex will eventually obtain it. See the code in
> `mutex-nostarve.c` for more information.

A binary semaphore used as a lock does not guarantee this. When several
threads wait, `post` wakes *some* waiter, and nothing prevents it from
picking the same one or two threads again and again. I used **Morris's
algorithm** (J. M. Morris, 1979; Downey 4.3). It is starvation-free even with
weak semaphores, which may wake any one of the waiting threads. (The code is
shown with `sem_t` / `Sem_*`. The file uses its own semaphore `xsem_t`, see
"Which semaphore" below.)

```c
typedef struct __ns_mutex_t {
    int room1, room2;  // number of threads in each waiting room
    sem_t mutex;       // 1: protects room1
    sem_t t1, t2;      // 1 and 0: turnstiles into room 2 and into the critical section
} ns_mutex_t;

void ns_mutex_acquire(ns_mutex_t *m) {
    Sem_wait(&m->mutex);  m->room1++;  Sem_post(&m->mutex);

    Sem_wait(&m->t1);                 // move from room 1 to room 2
    m->room2++;
    Sem_wait(&m->mutex);
    m->room1--;
    if (m->room1 == 0) { Sem_post(&m->mutex); Sem_post(&m->t2); }  // last one: close t1, open t2
    else               { Sem_post(&m->mutex); Sem_post(&m->t1); }  // let the next one in

    Sem_wait(&m->t2);                 // into the critical section
    m->room2--;
}

void ns_mutex_release(ns_mutex_t *m) {
    if (m->room2 == 0) Sem_post(&m->t1);   // round over: open t1 again
    else               Sem_post(&m->t2);   // next thread of this round
}
```

**Why no thread starves.** The lock works in **rounds**. In phase 1,
the arriving threads gather in room 1 and pass `t1` one at a time into
room 2. The last thread to pass closes `t1` and opens `t2`. In phase 2, the
threads in room 2 go through the critical section one after another, and the
last of them opens `t1` again. While phase 2 runs, `t1` is closed, so any
thread that arrives now, including a thread that has just left the critical
section and wants the lock again, waits in room 1 for the next round. A
waiting thread therefore waits at most for the rest of the current round and
then for the other threads in its own round. Each round holds at most N − 1
other threads, so at most **2(N − 1) other acquisitions** can come before it.
The bound does not depend on how long the program runs.

**How to show that no thread starves.** One run only shows that a thread got
the lock *this time*. So the test measures **overtakes**: for every
acquisition, the number of times *other* threads got the lock while this
thread was waiting. If the lock is starvation-free, the largest value stays
at or below 2(N − 1) however long the run is. If it allows starvation, the
largest value grows with the run. For Morris's lock, waiting starts when the
thread is counted in room 1. Before that point the OS may simply not have
run the thread yet, and no lock can bound that.

Each of the N threads acquires the lock `loops` times, with a little work
inside and outside the critical section. The comparison is a plain binary
semaphore (`-m sem`). A counter incremented without atomics under the lock
must equal N × loops (it always did). A check that at most one thread is
inside never fired.

**Which semaphore.** Downey's proof assumes weak semaphores *without
barging*: a post must go to a thread that is already waiting. glibc's
semaphore allows barging (see the top). So `mutex-nostarve.c` has its own
semaphore `xsem_t` in two kinds:

- `-s lifo`: a weak semaphore without barging and with the **worst possible
  choice**. `post` hands the semaphore directly to the thread that started
  waiting *last*.
- `-s zem`: the Zemaphore of Figure 31.17 (a value, a lock and a condition
  variable). Like glibc's semaphore, it allows barging.

In the VM, handing the lock to a sleeping thread is slow: Morris's lock with
16 threads and 100 000 loops each took 88 s. So the test uses 2 000 and
20 000 loops per thread. Worst case over 3 runs (`data/q6.txt`):

| semaphore | lock | N = 2 | N = 3 | N = 4 | N = 8 | N = 16 |
|---|---|---|---|---|---|---|
| | bound 2(N − 1) | 2 | 4 | 6 | 14 | 30 |
| LIFO | **Morris**, 2 000 loops | 2 | 4 | 6 | 14 | 18 |
| LIFO | **Morris**, 20 000 loops | 2 | 4 | 6 | 14 | 29 |
| LIFO | plain semaphore, 2 000 loops | 24 | **3 998** | 5 258 | 12 876 | 29 829 |
| LIFO | plain semaphore, 20 000 loops | 234 | **17 170** | 54 029 | 127 995 | **297 753** |
| Zemaphore | **Morris**, 2 000 loops | 2 | 4 | 6 | 14 | 30 |
| Zemaphore | **Morris**, 20 000 loops | 2 | 4 | 6 | 14 | 30 |
| Zemaphore | plain semaphore, 2 000 loops | 574 | 3 202 | 2 265 | 4 460 | 10 460 |
| Zemaphore | plain semaphore, 20 000 loops | 907 | 3 955 | 6 724 | 11 197 | 22 775 |

- **Morris's lock never exceeds 2(N − 1)**, and running 10× longer does not
  change that, with either semaphore. With the Zemaphore it reaches the bound
  exactly in every configuration, so the bound is tight.
- **The plain semaphore starves threads.** With the LIFO semaphore and
  N = 3, one thread waited while the other two did **3 998**
  acquisitions, i.e., almost *all* of their 2 × 2 000 loops. It got the lock
  only when the others were nearly finished. With 16 threads one thread was
  overtaken 297 753 times and waited **8.36 s** of an 8.47 s run. With the
  Zemaphore the starvation is milder but still clear (up to 22 775
  overtakes), and it grows with the run length.
- For the plain semaphore with N = 2 the bound is exceeded too (24 and 234).
  Here waiting is measured from the call of `xsem_wait()`, and part of that
  time a thread waits for the semaphore's internal pthread lock, which is not
  fair either.
- **Fairness costs throughput.** Morris's lock needs 8 semaphore
  operations per acquisition and release instead of 2. It also hands the lock to a
  thread that is asleep instead of one that is running. It took 7.4 s
  instead of 4.9 s (LIFO, N = 8, 20 000 loops), and 1.14 s instead of
  0.008 s (Zemaphore, N = 2, 20 000 loops).

The longest waits in milliseconds are also in `data/q6.txt`. For Morris's
lock they are a few milliseconds in most runs, but up to 91 ms with 16
threads on 4 virtual CPUs. That wait is the OS not running a thread, not the
lock: the overtakes stayed within the bound.

---

## Q7

> Liked these problems? See Downey's free text for more just like them. And
> don't forget, have fun! But, you always do when you write code, no?

Yes. Two of the lessons here are also in Downey's book. Solutions that look
alike can differ in whether they guarantee progress (Q4 vs Q5). And the
guarantee of a solution depends on what the semaphore underneath promises:
the turnstile in Q5 needed a FIFO semaphore, and Morris's algorithm in Q6
needs a semaphore without barging. The surprise for me was that the system's
own semaphore (glibc) gives neither guarantee. A program that is "correct"
with semaphores can still let a thread wait for almost the whole run.
