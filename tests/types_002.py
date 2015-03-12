# Verify a type can be constructed.
# Kind of over-simplistic, but verifies all the type calling machinery.
x = int()
if x == 0:
    print('PASSED')
else:
    print('FAILED')
