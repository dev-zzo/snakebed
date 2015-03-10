# How CPython handles exceptions

Exceptions are handled using the concept of *blocks*, with two kinds of those existing: 
*try blocks* and *finally blocks*. This seems to stem from opcodes having only one possible argument, 
so code having both `try` and `finally` cannot be set up with one opcode anyway.

Let's see how things are handled on bytecode level.

```python
try:
    a = 1
except:
    pass

# 1           0 SETUP_EXCEPT            10 (to 13)
# 
# 2           3 LOAD_CONST               0 (1)
#             6 STORE_NAME               0 (a)
#             9 POP_BLOCK
#            10 JUMP_FORWARD             7 (to 20)
# 
# 3     >>   13 POP_TOP
#            14 POP_TOP
#            15 POP_TOP
# 
# 4          16 JUMP_FORWARD             1 (to 20)
#            19 END_FINALLY
#       >>   20 LOAD_CONST               1 (None)
#            23 RETURN_VALUE
```

Note when execution flows outside the `except` clause, `END_FINALLY` is not executed.

```python
try:
    a = 1
finally:
    pass

# 1           0 SETUP_FINALLY           10 (to 13)
# 
# 2           3 LOAD_CONST               0 (1)
#             6 STORE_NAME               0 (a)
#             9 POP_BLOCK
#            10 LOAD_CONST               1 (None)
# 
# 4     >>   13 END_FINALLY
#            14 LOAD_CONST               1 (None)
#            17 RETURN_VALUE
```

And with both `except` and `finally` clauses:

```python
try:
    a = 1
except:
    a = 2
finally:
    a = 3

# 1           0 SETUP_FINALLY           30 (to 33)
#             3 SETUP_EXCEPT            10 (to 16)
# 
# 2           6 LOAD_CONST               0 (1)
#             9 STORE_NAME               0 (a)
#            12 POP_BLOCK
#            13 JUMP_FORWARD            13 (to 29)
# 
# 3     >>   16 POP_TOP
#            17 POP_TOP
#            18 POP_TOP
# 
# 4          19 LOAD_CONST               1 (2)
#            22 STORE_NAME               0 (a)
#            25 JUMP_FORWARD             1 (to 29)
#            28 END_FINALLY
#       >>   29 POP_BLOCK
#            30 LOAD_CONST               2 (None)
# 
# 6     >>   33 LOAD_CONST               3 (3)
#            36 STORE_NAME               0 (a)
#            39 END_FINALLY
#            40 LOAD_CONST               2 (None)
#            43 RETURN_VALUE
```

Code at 28 is efficiently dead -- due to the all-catching `except`. Modifying the example a bit reveals:

```python
try:
    a = 1
except KeyError:
    a = 2
finally:
    a = 3
a = 4

# 1           0 SETUP_FINALLY           40 (to 43)
#             3 SETUP_EXCEPT            10 (to 16)
# 
# 2           6 LOAD_CONST               0 (1)
#             9 STORE_NAME               0 (a)
#            12 POP_BLOCK
#            13 JUMP_FORWARD            23 (to 39)
# 
# 3     >>   16 DUP_TOP
#            17 LOAD_NAME                1 (KeyError)
#            20 COMPARE_OP              10 (exception match)
#            23 POP_JUMP_IF_FALSE       38
#            26 POP_TOP
#            27 POP_TOP
#            28 POP_TOP
# 
# 4          29 LOAD_CONST               1 (2)
#            32 STORE_NAME               0 (a)
#            35 JUMP_FORWARD             1 (to 39)
#       >>   38 END_FINALLY
#       >>   39 POP_BLOCK
#            40 LOAD_CONST               2 (None)
# 
# 6     >>   43 LOAD_CONST               3 (3)
#            46 STORE_NAME               0 (a)
#            49 END_FINALLY
# 
# 7          50 LOAD_CONST               4 (4)
#            53 STORE_NAME               0 (a)
#            56 LOAD_CONST               2 (None)
#            59 RETURN_VALUE
```

So, let's see what happens.

If no exception is raised, the following insn sequence is executed:

```
# 1           0 SETUP_FINALLY           40 (to 43)
#             3 SETUP_EXCEPT            10 (to 16)
# 2           6 LOAD_CONST               0 (1)
#             9 STORE_NAME               0 (a)
#            12 POP_BLOCK
#       >>   39 POP_BLOCK
#            40 LOAD_CONST               2 (None)
# 6     >>   43 LOAD_CONST               3 (3)
#            46 STORE_NAME               0 (a)
#            49 END_FINALLY
# 7          50 LOAD_CONST               4 (4)
#            53 STORE_NAME               0 (a)
```

If an exception is raised and handled:

```
# 1           0 SETUP_FINALLY           40 (to 43)
#             3 SETUP_EXCEPT            10 (to 16)
# 2           6 LOAD_CONST               0 (1)
#             9 STORE_NAME               0 (a)
# 3     >>   16 DUP_TOP
#            17 LOAD_NAME                1 (KeyError)
#            20 COMPARE_OP              10 (exception match)
#            23 POP_JUMP_IF_FALSE       38
#            26 POP_TOP
#            27 POP_TOP
#            28 POP_TOP
# 4          29 LOAD_CONST               1 (2)
#            32 STORE_NAME               0 (a)
#       >>   39 POP_BLOCK
#            40 LOAD_CONST               2 (None)
# 6     >>   43 LOAD_CONST               3 (3)
#            46 STORE_NAME               0 (a)
#            49 END_FINALLY
# 7          50 LOAD_CONST               4 (4)
#            53 STORE_NAME               0 (a)
```

If an exception is raised and not handled:

```
# 1           0 SETUP_FINALLY           40 (to 43)
#             3 SETUP_EXCEPT            10 (to 16)
# 2           6 LOAD_CONST               0 (1)
#             9 STORE_NAME               0 (a)
# 3     >>   16 DUP_TOP
#            17 LOAD_NAME                1 (KeyError)
#            20 COMPARE_OP              10 (exception match)
#            23 POP_JUMP_IF_FALSE       38
#       >>   38 END_FINALLY
#       >>   39 POP_BLOCK
#            40 LOAD_CONST               2 (None)
# 6     >>   43 LOAD_CONST               3 (3)
#            46 STORE_NAME               0 (a)
#            49 END_FINALLY
# 7          50 LOAD_CONST               4 (4)
#            53 STORE_NAME               0 (a)
```

