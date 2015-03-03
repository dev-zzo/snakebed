# Test `return` within `finally`.

def f(x):
    try:
        pass
    finally:
        return x

if f(42) == 42:
    print('PASSED')
else:
    print('FAILED')
