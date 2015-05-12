# Verify the UnboundLocalError is raised
def f():
    a = 1
    del a
    b = a
try:
    f()
    print('FAILED 1')
except UnboundLocalError:
    print('PASSED')
except:
    print('FAILED 2')
