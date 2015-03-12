# Verify `class` works (simple)
class A:
    classprop = 42
if A.classprop == 42:
    print('PASSED')
else:
    print('FAILED')
