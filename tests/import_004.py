# Verify importing a nonexistent module fails.
try:
    import IDontExist
    print('FAILED 1')
except ImportError:
    print('PASSED')
except:
    print('FAILED 2')
