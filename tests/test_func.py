"""
This is a test suite for handling function arguments.
"""

import unittest

class Tests(unittest.TestCase):
    def test_defaults(self):
        def f(a, b, c=100, d=200):
            return c == 100 and d == 200
        self.assertTrue(f(0, 0))
    def test_args_unpack(self):
        def f(x, y, z):
            return x == 42 and y == 777 and z == 31
        args = (42, 777, 31)
        self.assertTrue(f(*args))
    def test_kwds_simple(self):
        def f(x, y):
            return x == 42 and y == 777
        self.assertTrue(f(y=777, x=42))
    def test_kwds_unpack(self):
        def f(x, y, z):
            return x == 42 and y == 777 and z == 31
        kwds = { 'x': 42, 'y': 777, 'z': 31 }
        self.assertTrue(f(**kwds))
    def test_vargs(self):
        def f(x, y, *args):
            return x == 42 and y == 777 and type(args) is tuple and args[0] == 73 and args[1] == 11
        self.assertTrue(f(42, 777, 73, 11))
    def test_argcount_typeerror(self):
        def f(x, y):
            pass
        self.assertRaises(TypeError, f)
        self.assertRaises(TypeError, f, 0, 1, 2, 3)
        self.assertRaises(TypeError, f, x=1, y=2, z=3)
#

if __name__ == "__main__":
    r = Tests().run()
    print(str(r))
    print()
