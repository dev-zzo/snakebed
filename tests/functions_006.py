# Verify dict args can be unpacked
def f(x, y, z):
    if x == 42 and y == 777 and z == 31:
        print('PASSED')
    else:
        print('FAILED')
kwargs = { 'x': 42, 'y': 777, 'z': 31 }
f(**kwargs)
