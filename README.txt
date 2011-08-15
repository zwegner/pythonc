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
while translating. A speed somewhere close to C++ is expected. Sound ridiculous?
Of course it does. But we'll just have to wait and see.

Some near-term plans:
Improve the memory management
Restructure the front end so most logic is in the syntax.py domain
Do lots of simple but effective optimizations during translation
Fill in enough of the Python builtins/simple modules to not care anymore
Clean up the back end code

Some long-term plans:
Remove all use of the STL
Use C instead of C++

Python support
==============
Pythonc has only been tested to work with Python 3. It could quite possibly
work for Python 2 though. The subset of Python implemented is hardly distinguishable
as Python 2 or 3, except that "print" is a function.

It uses the Python ast module to parse the Python code.

There are just a few places with incompatibilities:
All integers are signed 64-bit ints
Str/bytes are the same thing
Tuples/lists are the same thing
Genexps/list comprehensions are the same thing
Probably a bunch of other little things

Some things it doesn't support as of now:
Classes with inheritance
Any dynamic features (introspection/runtime code generation)
Exceptions
Floating point
Imports/the entire standard library
Useful error messages
Several cases of complex compound expressions (e.g. assigning to slices)
Generators
Operator overloading
