Pythonc--Python to C++ translator

Copyright 2011 Zach Wegner

Pythonc is pronounced like "Pie Thonk", for obvious reasons.

It currently translates some nontrivial subset of Python code into C++. No
claims are made about the correctness or efficiency of the generated code.
You have been warned.

The translator is released under the GPLv3.

The C++ backend, used by the generated code, is released under the Apache license.

Details
=======
Pythonc is intended to be a translator of a very simple subset of Python,
suitable for use in embedded applications.

Eventually, the translator will be able to optimize the code substantially
while translating. A speed somewhere close to C++ is expected.

Python support
==============
Pythonc has only been tested to work with Python 3. It could quite possibly
work for Python 2 though. The subset of Python implemented is hardly distinguishable
as Python 2 or 3, except that "print" is a function.

It uses the Python ast module to parse the Python code.

The only place where Pythonc is explicitly Python incompatible is that all
integers are 64-bit signed ints.

Some things it doesn't support as of now:
Classes with inheritance
Any dynamic features (introspection/runtime code generation)
Exceptions
Floating point
Imports/the entire standard library
Useful error messages
Several cases of complex compound expressions (e.g. assigning to slices)
