#!/usr/bin/python3
import os
import shutil
import subprocess
import time
import sys

tests = set('%s.py' % s for s in sys.argv[1:])

if not os.path.exists('.temp'):
    os.mkdir('.temp')
for i in ['pythonc.py', 'syntax.py', 'transform.py', 'backend.cpp', 'alloc.py']:
    shutil.copy(i, '.temp')

passes = 0
fails = 0

for i in os.listdir('tests'):
    if i.endswith('.py') and (not tests or i in tests):
        shutil.copy('tests/%s' % i, '.temp/test.py')
        comp_start = time.time()
        out_p = subprocess.check_output('./pythonc.py -O -q -c test.py', cwd='.temp', shell=True)
        start = time.time()
        out_p = subprocess.check_output('./test', cwd='.temp', shell=True)
        mid = time.time()
        out_c = subprocess.check_output('python3 test.py', cwd='.temp', shell=True)
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
