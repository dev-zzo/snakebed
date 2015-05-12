# Verify the exception is being raised if there is not enough args passed.
def f(x, y):
    pass
try:
    f(0)
    print('FAILED 1')
except TypeError:
    print('PASSED')
except:
    print('FAILED 2')
