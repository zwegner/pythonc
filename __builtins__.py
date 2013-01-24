################################################################################
##
## Pythonc--Python to C++ translator
##
## Copyright 2012 Zach Wegner
##
## This file is part of Pythonc.
##
## Pythonc is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## Pythonc is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with Pythonc.  If not, see <http://www.gnu.org/licenses/>.
##
################################################################################

import sys

@builtin
def any(iterable):
    for element in iterable:
        if element:
            return True
    return False

@builtin
def all(iterable):
    for element in iterable:
        if not element:
            return False
    return True

# XXX support multiple iterables
# XXX needs to be a generator
#def map(func, iterable):
#    r = []
#    for element in iterable:
#        r.append(func(element))
#    return r

@builtin
def print(*args, sep=None, end=None, file=None):
    if sep is None:
        sep = ' '
    if end is None:
        end = '\n'
    if file is None:
        file = sys.stdout
    file.write(sep.join([str(s) for s in args]) + end)
