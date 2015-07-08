# sys.exc_info() does not provide exception information within a handler.

import sys

try:
    raise KeyError
except:
    i = sys.exc_info()
    if i[0] is KeyError:
        print("PASSED")
    else:
        print("FAILED")
