# Test that an exception can be raised and caught.
print('Test case 1:')
try:
    raise KeyError, 1234
except KeyError as e:
    if e.args[0] == 1234:
        print('PASSED')
    else:
        print('FAILED 1')
else:
    print('FAILED 2')
