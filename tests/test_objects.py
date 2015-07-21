"""
This is a test suite for various object-related things.
"""

import unittest

__del_called = False

class Tests(unittest.TestCase):
    def test_del(self):
        "Verify __del__ is called when defined"
        class C:
            def __del__(self):
                global __del_called
                __del_called = True
        x = C()
        __del_called = False
        del x
        self.assertTrue(__del_called)
    pass
#

if __name__ == "__main__":
    r = Tests().run()
    print(str(r))
    print()
