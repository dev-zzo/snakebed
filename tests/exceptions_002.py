# Test that `finally` executes.
print('Test case 2:')
try:
    try:
        raise KeyError
    finally:
        print('PASSED')
except:
    print('FAILED')
