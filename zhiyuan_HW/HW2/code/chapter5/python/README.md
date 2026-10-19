# Chapter 5 code questions — Python versions

Python ports of `../q1.c` … `../q8.c`. They make the same system calls through
the `os` module (`os.fork`, `os.wait`, `os.waitpid`, `os.pipe`, `os.dup2`,
`os.exec*`, `os.open`, `os.write`). Run on Linux:

```sh
./q1.py      # ... ./q8.py
./q2.py 1000 # q2 with 1000 lines per process, to see concurrent writes interleave
```

The results match the C programs (see `../README.md` for the full answers).
Tested on Ubuntu 24.04 with Python 3.12.3.

## Where Python behaves differently from C

| Question | C | Python |
|---|---|---|
| Q1 | can print `&x` to show both processes use the same virtual address | no way to print a variable's address; `id(x)` is the address of the int object, not of the variable |
| all | `fork()` returns -1 on failure | `os.fork()` raises `OSError` instead |
| Q2 | 5 lines per process usually came out in order | interleaving showed up even with 5 lines (`parent 0, child 0, parent 1 ...`), because the interpreter is slower between system calls |
| Q4 | `execl`/`execvpe` etc. are separate libc functions | all `os.exec*` variants are implemented in Python on top of `execv()`/`execve()`; `os.execvpe` searches the `PATH` of the environment passed in |
| Q5 | `wait()` in the child returns -1 with `errno = ECHILD` | `os.wait()` raises `ChildProcessError` (errno 10, `ECHILD`) |
| Q7 | `printf()` silently fails; only the return value shows `EBADF` | `print()` raises `OSError: [Errno 9] Bad file descriptor`; the child uses `os._exit()` so the interpreter does not try to flush the closed stdout again at exit |

The point about output buffering from Q1 applies to Python too: the scripts
call `sys.stdout.flush()` before `fork()`. Otherwise, when stdout is a pipe, the
unflushed buffer is copied into the child and printed twice.
