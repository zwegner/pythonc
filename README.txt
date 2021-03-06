Pythonc--Python to C++ translator

Copyright 2013 Zach Wegner, Matt Craighead

Pythonc is pronounced like "Pie Thonk", for obvious reasons.

It currently translates some nontrivial subset of Python code into C++. No
claims are made about the correctness or efficiency of the generated code,
though it should be much faster than CPython (and will get even faster in
the near future). You have been warned.

The translator is released under the GPLv3.

The C++ backend, used by the generated code, is released under the Apache license.

Details
=======
Pythonc is intended to be a translator of a very simple subset of Python,
suitable for use in embedded applications.

Eventually, the translator will be able to optimize the code substantially
while translating. A speed somewhere close to C++ is expected. Sound ridiculous?
Of course it does. But we'll just have to wait and see.

Python support
==============
Pythonc only officially supports Python 3, and it will probably stay that way.
It might not be too hard to get it to work with Python 2, but we haven't tried
and we don't care.

It uses the Python ast module to parse the Python code. At some point we
will want to write our own parser, for various reasons (making Pythonc
self-hosting, eval(), not having to do all the transform stuff, ...).

There are just a few places with incompatibilities:
All integers are signed 64-bit ints
Genexps/list comprehensions are the same thing
Probably a bunch of other little things

Some things it doesn't support as of now:
Classes with inheritance
Modules don't allow setattr()
locals()/globals()
Any dynamic features (introspection/runtime code generation)
Exceptions
Floating point
Dynamic imports/most of the standard library
Useful error messages
Several cases of complex compound expressions (e.g. assigning to slices)
Generators
Operator overloading
