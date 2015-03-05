# Test that an exception can be raised and caught.
try:
    raise KeyError, 1234
    print('FAILED 1')
except KeyError as e:
    if e.args[0] == 1234:
        print('PASSED')
    else:
        print('FAILED 2')
else:
    print('FAILED 3')
