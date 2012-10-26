#!/usr/bin/python3
import os
import shutil
import subprocess
import time
import sys

tests = set(sys.argv[1:])

passes = 0
fails = 0

for i in os.listdir('tests'):
    # Check for test filter
    if tests and i.replace('.py', '') not in tests:
        continue

    # Clear the temp directory
    cwd = '.temp'
    shutil.rmtree(cwd)
    os.mkdir(cwd)

    # Copy Pythonc scripts into temp directory
    for j in ['pythonc.py', 'syntax.py', 'transform.py', 'backend.cpp', 'alloc.py', '__builtins__.py']:
        shutil.copy(j, cwd)

    # Copy the test and/or test data
    path = 'tests/%s' % i
    if i.endswith('.py'):
        shutil.copy(path, '%s/test.py' % cwd)
    elif os.path.isdir(path):
        for j in os.listdir(path):
            shutil.copy('%s/%s' % (path, j), cwd)
    else:
        continue

    # Compile
    comp_start = time.time()
    out_p = subprocess.check_output('./pythonc.py -O -c test.py', cwd=cwd, shell=True)

    # Run pythonc-compiled version
    start = time.time()
    out_p = subprocess.check_output('./test', cwd=cwd, shell=True)

    # Run CPython version
    mid = time.time()
    out_c = subprocess.check_output('python3 test.py', cwd=cwd, shell=True)
    end = time.time()

    if out_p != out_c:
        print('%s mismatched!\nPythonc:\n%s\nCPython:\n%s' % (i,
            out_p.decode(), out_c.decode()))
        fails += 1
    else:
        passes += 1
    time_compile = start - comp_start
    time_p = mid - start
    time_c = end - mid
    print('%s: pythonc=%.3fs (compile=%.3fs) cpython=%.3fs (%.3fx)' % (i, time_p, time_compile, time_c, time_c / time_p))

print('%s/%s tests passed.' % (passes, passes + fails))
