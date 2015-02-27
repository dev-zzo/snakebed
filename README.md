# SnakeBed

A toy implementation of Python 2.7 subset designed for compilation freedom and size minimisation (you don't build what you don't want).

# C Conventions

*All* functions that can possibly fail *must* return an indication so the caller can tell whether the call has failed.

## Functions returning int

* 0 indicates success
* -1 indicates failure (exception set unless noted otherwise)

## Functions returning a pointer

* Non-NULL indicates success
* NULL indicates failure (exception set unless noted otherwise)

# TODO List

* Slots implementation
* Unify lists and tuples by as much as possible without hurting performance
* Factor out common code into some more generic sequence implementation?
