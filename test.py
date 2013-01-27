#!/usr/bin/python3
import os
import shutil
import subprocess
import tempfile
import time
import sys

optimize = '-O'
if '-d' in sys.argv:
    sys.argv.remove('-d')
    optimize = ''

tests = set(sys.argv[1:])

passes = 0
fails = 0

for i in os.listdir('tests'):
    # Check for test filter
    if tests and i.replace('.py', '') not in tests:
        continue

    # Create the temp directory
    temp_dir = tempfile.TemporaryDirectory()
    cwd = temp_dir.name

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
    out_p = subprocess.check_output('python pythonc.py %s -c test.py' % optimize, cwd=cwd, shell=True)

    # Run pythonc-compiled version
    start = time.time()
    try:
        out_p = subprocess.check_output('test', cwd=cwd, shell=True)
    except subprocess.CalledProcessError as e:
        # XXX assuming no output==fail
        out_p = repr(e).encode('utf-8')

    # Run CPython version
    mid = time.time()
    out_c = subprocess.check_output('python test.py', cwd=cwd, shell=True)
    end = time.time()

    # Compare
    if out_p != out_c:
        print('%s mismatched!\nPythonc:\n%s\nCPython:\n%s' % (i,
            out_p.decode(), out_c.decode()))
        # Make sure the contents are visible in case of failure
        shutil.rmtree('.out')
        shutil.copytree(cwd, '.out')
        for path, data in [['.out/pythonc', out_p], ['.out/cpython', out_c]]:
            with open(path, 'w') as f:
                f.write(data.decode())
        fails += 1
    else:
        passes += 1

    # Print timing info
    time_compile = start - comp_start
    time_p = mid - start
    time_c = end - mid
    print('%s: pythonc=%.3fs (compile=%.3fs) cpython=%.3fs (%.3fx)' % (i, time_p, time_compile, time_c, time_c / time_p))

print('%s/%s tests passed.' % (passes, passes + fails))
