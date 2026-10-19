#!/usr/bin/env python3
# Q4: run /bin/ls with every exec() variant.
import os
import sys

args = ["ls", "-ld", "/tmp"]
env = {"MYVAR": "hello"}

variants = [
    ("execl",   lambda: os.execl("/bin/ls", *args)),
    ("execle",  lambda: os.execle("/bin/ls", *args, env)),
    ("execlp",  lambda: os.execlp("ls", *args)),
    ("execv",   lambda: os.execv("/bin/ls", args)),
    ("execvp",  lambda: os.execvp("ls", args)),
    ("execvpe", lambda: os.execvpe("ls", args, env)),
]

for name, run in variants:
    print(f"{name:<8}: ", end="")
    sys.stdout.flush()
    rc = os.fork()
    if rc == 0:
        try:
            run()
        except OSError as e:  # only reached if exec failed
            print(f"{name}: {e}", file=sys.stderr)
        os._exit(1)
    os.waitpid(rc, 0)
