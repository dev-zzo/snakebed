"""
This is a test suite for exception objects and their handling.
"""

import unittest

class Tests(unittest.TestCase):
    def test_properties(self):
        def f():
            Exception().nonexistentattribute
        self.assertRaises(AttributeError, f)

    def test_args(self):
        "Verify exception args can be accessed"
        x = Exception(10, "abc", False, "irrelevant")
        args = x.args
        self.assertTrue(type(args) is tuple)
        self.assertEqual(args[0], 10)
        self.assertEqual(args[1], "abc")
        self.assertEqual(args[2], False)
        self.assertEqual(args[3], "irrelevant")

if __name__ == "__main__":
    r = Tests().run()
    print(str(r))
    print()
