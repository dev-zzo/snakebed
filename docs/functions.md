# Python Functions

Studying this spiece of test code:

```python
def f_noargs():
    x = 42
def f_posargs(a, b, c):
    x = 42
def f_posargs_defaults(a, b=100, c='zzz'):
    x = 42
def f_aargs(a, b, *args):
    x = 42
def f_kwargs(a, b, **kwargs):
    x = 42
def f_aargs_kwargs(a, b, *args, **kwargs):
    x = 42
```

Generates:

```
Code object: <module>.f_noargs
  co_name:        f_noargs
  co_flags:       10043 (CO_NEWLOCALS, CO_NOFREE)
  co_stacksize:   1
  co_argcount:    0
  co_nlocals:     1
  co_varnames:    1
    0: x
  co_names:       0
  co_freevars:    0
  co_cellvars:    0
  co_consts:      2
    0: None
    1: 42
  co_code:        10
  2           0 LOAD_CONST               1 (42)
              3 STORE_FAST               0 (x)
              6 LOAD_CONST               0 (None)
              9 RETURN_VALUE


Code object: <module>.f_posargs
  co_name:        f_posargs
  co_flags:       10043 (CO_NEWLOCALS, CO_NOFREE)
  co_stacksize:   1
  co_argcount:    3
  co_nlocals:     4
  co_varnames:    4
    0: a
    1: b
    2: c
    3: x
  co_names:       0
  co_freevars:    0
  co_cellvars:    0
  co_consts:      2
    0: None
    1: 42
  co_code:        10
  4           0 LOAD_CONST               1 (42)
              3 STORE_FAST               3 (x)
              6 LOAD_CONST               0 (None)
              9 RETURN_VALUE


Code object: <module>.f_posargs_defaults
  co_name:        f_posargs_defaults
  co_flags:       10043 (CO_NEWLOCALS, CO_NOFREE)
  co_stacksize:   1
  co_argcount:    3
  co_nlocals:     4
  co_varnames:    4
    0: a
    1: b
    2: c
    3: x
  co_names:       0
  co_freevars:    0
  co_cellvars:    0
  co_consts:      2
    0: None
    1: 42
  co_code:        10
  6           0 LOAD_CONST               1 (42)
              3 STORE_FAST               3 (x)
              6 LOAD_CONST               0 (None)
              9 RETURN_VALUE


Code object: <module>.f_aargs
  co_name:        f_aargs
  co_flags:       10047 (CO_NEWLOCALS, CO_VARARGS, CO_NOFREE)
  co_stacksize:   1
  co_argcount:    2
  co_nlocals:     4
  co_varnames:    4
    0: a
    1: b
    2: args
    3: x
  co_names:       0
  co_freevars:    0
  co_cellvars:    0
  co_consts:      2
    0: None
    1: 42
  co_code:        10
  8           0 LOAD_CONST               1 (42)
              3 STORE_FAST               3 (x)
              6 LOAD_CONST               0 (None)
              9 RETURN_VALUE


Code object: <module>.f_kwargs
  co_name:        f_kwargs
  co_flags:       1004B (CO_NEWLOCALS, CO_VARKEYWORDS, CO_NOFREE)
  co_stacksize:   1
  co_argcount:    2
  co_nlocals:     4
  co_varnames:    4
    0: a
    1: b
    2: kwargs
    3: x
  co_names:       0
  co_freevars:    0
  co_cellvars:    0
  co_consts:      2
    0: None
    1: 42
  co_code:        10
 10           0 LOAD_CONST               1 (42)
              3 STORE_FAST               3 (x)
              6 LOAD_CONST               0 (None)
              9 RETURN_VALUE


Code object: <module>.f_aargs_kwargs
  co_name:        f_aargs_kwargs
  co_flags:       1004F (CO_NEWLOCALS, CO_VARARGS, CO_VARKEYWORDS, CO_NOFREE)
  co_stacksize:   1
  co_argcount:    2
  co_nlocals:     5
  co_varnames:    5
    0: a
    1: b
    2: args
    3: kwargs
    4: x
  co_names:       0
  co_freevars:    0
  co_cellvars:    0
  co_consts:      2
    0: None
    1: 42
  co_code:        10
 12           0 LOAD_CONST               1 (42)
              3 STORE_FAST               4 (x)
              6 LOAD_CONST               0 (None)
              9 RETURN_VALUE
```

Observations:

* Positional args come first in the name list, in the order they are specified
* `co_argcount` corresponds to the number of positinal args
* Neither `*args` nor `*kwargs` are included in `co_argcount`
* Presence of `*args` is noted by the `CO_VARARGS` flag; comes in declaration order
* Presence of `*kwargs` is noted by the `CO_VARKEYWORDS` flag; comes in declaration order
* Presence of default arguments changes nothing in code object definition
