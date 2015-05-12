# Verify the exception is being raised if more args passed than required.
def f(x, y):
    pass
try:
    f(0, 1, 2, 3)
    print('FAILED 1')
except TypeError:
    print('PASSED')
except:
    print('FAILED 2')
