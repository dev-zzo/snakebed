# Print out a trace back

def f():
    raise KeyError, "abcd"
def g():
    f()
def h():
    a = 1
    b = a
    g()
h()
