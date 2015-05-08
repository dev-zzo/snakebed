# Verify tuple args can be unpacked
def f(x, y, z):
    if x == 42 and y == 777 and z == 31:
        print('PASSED')
    else:
        print('FAILED')
args = (42, 777, 31)
f(*args)
