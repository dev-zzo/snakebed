# Verify simple positional args can be passed.
def f(x, y):
    if x == 42 and y == 777:
        print('PASSED')
    else:
        print('FAILED')
f(42, 777)
