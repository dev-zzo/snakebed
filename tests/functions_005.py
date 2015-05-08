# Verify simple keyword args can be passed.
def f(x, y):
    if x == 42 and y == 777:
        print('PASSED')
    else:
        print('FAILED')
f(y=777, x=42)
