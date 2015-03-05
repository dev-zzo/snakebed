# Test `return` within `finally`.

def f(x):
    try:
        pass
    finally:
        return x
    print('FAILED 1')
if f(42) == 42:
    print('PASSED')
else:
    print('FAILED 2')
