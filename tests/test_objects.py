"""
This is a test suite for various object-related things.
"""

import unittest

del_called = False

class Tests(unittest.TestCase):
    def test_del(self):
        "Verify __del__ is called when defined"
        global del_called
        class C:
            def __del__(self):
                global del_called
                del_called = True
        x = C()
        del_called = False
        del x
        self.assertTrue(del_called)
    pass
#

if __name__ == "__main__":
    r = Tests().run()
    print(str(r))
    print()
