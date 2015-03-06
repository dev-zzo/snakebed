# Verify that tuple's __getitem__() works.
x = (100, 200, 300)
if x[1] == 200:
    print('PASSED')
else:
    print('FAILED')
