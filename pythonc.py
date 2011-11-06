#!/usr/bin/python3

import os
import subprocess
import sys
import time

def usage():
    print('usage: %s [-O] <input.py> [args...]' % sys.argv[0])
    exit(1)

args = sys.argv[1:]

gcc_flags = ['-g', '-Wall', '-std=c++0x']
quiet = False
compile_only = False
while args:
    arg = args.pop(0)
    if arg == '-O':
        gcc_flags += ['-O3']
    elif arg == '-c':
        compile_only = True
    elif arg == '-q':
        quiet = True
    else:
        args = [arg] + args
        break

if not args:
    usage()

path = args[0]
base = os.path.splitext(path)[0]

start = time.time()
subprocess.check_call(['python3', 'transform.py', path, '%s.cpp' % base])
elapsed = time.time() - start
if not quiet:
    print('Transform time: %.4fs' % elapsed)

start = time.time()
subprocess.check_call(['c++'] + gcc_flags + ['%s.cpp' % base, '-o', base])
elapsed = time.time() - start
if not quiet:
    print('Compile time: %.4fs' % elapsed)

if not compile_only:
    start = time.time()
    subprocess.check_call(['./%s' % base] + args[1:])
    elapsed = time.time() - start
    if not quiet:
        print('Run time: %.4fs' % elapsed)
