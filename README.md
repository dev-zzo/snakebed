# SnakeBed

A toy implementation of Python 2.7 subset designed for compilation freedom and 
size minimisation (you don't build what you don't want). The aim is to stay below 64K
with only essential modules compiled in.

Although every effort is being made to make code as fast as possible, 
speed is *not* the primary implementation objective.

# How it works

The main deliverable is an executable similar to non-interactive `python`, 
which allows running precompiled Python bytecode files (currently `*.sb`).

Since the core is compiled as a library, it should pose no problem to embed it into other applications.

## Compiling Python code

A tool (`tools/sbcompile.py`) is provided, which uses Python 2.7 installed on the host to compile
the Python source into a bytecode format that the runner can understand.

The format is quite similar to what Python uses to store compilation results, 
albeit modified to conserve space as much as is reasonable.

# Implementation notes

TBD, honestly. There is a lot to note.

## Bytecode

Currently, we are piggy-backing on CPython's bytecode format, which significantly 
influences our interpreter implementation. Unfortunately, there is no easy way 
to get away from it, but making interpreters interchangeable seems to be a good
design goal.

## Supporting __slots__

At the moment, there is no clear way to add support for `__slots__` without significantly 
bloating the code, at least, none that I am aware of. Dictionaries seem to do their job well.

# Implementation status

Presently, the interpreter is able to run a decent subset of what Python 2.7 can. 
Still, this is very much incomplete and buggy.

## Differences from CPython

* No support for threads, whatsoever. The implementation is single-threaded, not thread-safe.
* Types:
  * "New style" classes only.
  * No descriptor support (yet).
  * `__getslice__()`, `__setslice__()`, and `__delslice__()` are not supported. Slice objects are supported as parameters to `__getitem__()`, `__setitem__()`, and `__delitem__()` magic methods.
  * `__slots__` is not supported.
* Exceptions:
  * Raising and handling an exception within `except` will overwrite the last exception being handled.
  * Only raising types (e.g. `raise KeyError`) is supported.
* Packages are currently not supported at all. Not sure whether supporting these is required.
* Generators are currently not supported.
* Unicode is not supported at all.
* Dynamic code execution (`compile`, `eval`, `exec`, `execfile`) is obviously not supported.

# C Conventions

*All* functions that can possibly fail *must* return an indication so the caller can tell whether the call has failed.

## Functions returning int

* 0 indicates success
* -1 indicates failure (exception set unless noted otherwise)

## Functions returning a pointer

* Non-NULL indicates success
* NULL indicates failure (exception set unless noted otherwise)

# TODO List

* Implement `long`
* Unify lists and tuples by as much as possible without hurting performance
* Factor out common code into some more generic sequence implementation?
