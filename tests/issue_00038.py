def f(*args, **kwds):
    return True
def g(*args, **kwds):
    return f(*args, **kwds)
g()
