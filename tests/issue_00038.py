def f(*args, **kwds):
    return True
def g(*args, **kwds):
    return f(*args, **kwds)
try:
    if g():
        print("PASSED")
    else:
        print("FAILED 1")
except:
    print("FAILED 2")
