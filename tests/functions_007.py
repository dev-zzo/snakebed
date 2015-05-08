# Verify varargs work
def f(x, y, *args):
    if x == 42 and y == 777 and type(args) is tuple and args[0] == 73 and args[1] == 11:
        print('PASSED')
    else:
        print('FAILED')
f(42, 777, 73, 11)
