# Test that `finally` executes.
try:
    try:
        raise KeyError
    finally:
        print('PASSED')
except:
    print('FAILED')
