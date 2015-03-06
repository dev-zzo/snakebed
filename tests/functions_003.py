# Verify defaults are applied.
def f(a, b, c=100, d=200):
    if c == 100 and d == 200:
        print('PASSED')
    else:
        print('FAILED')
f(1, 2)
