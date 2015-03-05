# Verify exceptions propagate between frames

def f():
    raise KeyError
def g():
    try:
        f()
        print('FAILED 1')
    except KeyError:
        print('PASSED')
    except:
        print('FAILED 2')
g()
