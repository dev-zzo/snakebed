# Verify classes can be instantiated
class A:
    pass
if type(A()) is A:
    print('PASSED')
else:
    print('FAILED')
