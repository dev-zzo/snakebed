# Verify that `finally` executes properly.
try:
    try:
        raise KeyError
        print('FAILED 1')
    finally:
        print('PASSED')
except KeyError:
    pass # expected
except:
    print('FAILED 2')
