import unittest

class Test(unittest.TestCase):
    def test_comparisons_native(self):
        a = 20
        b = 42
        self.assertTrue(a < b)
        self.assertFalse(b < a)

        self.assertTrue(a <= b)
        self.assertFalse(b <= a)

        self.assertTrue(a != b)
        self.assertTrue(b != a)

        self.assertTrue(a == a)
    def test_comparisons_long_samesize(self):
        a = 20000000000
        b = 42000000000

        self.assertTrue(a == a)

        self.assertTrue(a < b)
        self.assertFalse(b < a)

        self.assertTrue(a <= b)
        self.assertFalse(b <= a)

        self.assertTrue(a != b)
        self.assertTrue(b != a)
    def test_comparisons_long_diffsize(self):
        a = 2000000
        b = 42000000000

        self.assertTrue(a == a)

        self.assertTrue(a < b)
        self.assertFalse(b < a)

        self.assertTrue(a <= b)
        self.assertFalse(b <= a)

        self.assertTrue(a != b)
        self.assertTrue(b != a)
    def test_comparisons_long_diffsize2(self):
        a = -42000000000
        b = 4200000

        self.assertTrue(a == a)

        self.assertTrue(a < b)
        self.assertFalse(b < a)

        self.assertTrue(a <= b)
        self.assertFalse(b <= a)

        self.assertTrue(a != b)
        self.assertTrue(b != a)
    def test_native_overflow_add(self):
        a = 2147483647
        b = 1
        self.assertEqual(a + b, 2147483648)
    def test_native_underflow_add(self):
        a = -2147483648
        b = -1
        self.assertEqual(a + b, -2147483649)
    def test_native_overflow_sub(self):
        a = -2147483648
        b = 1
        self.assertEqual(a - b, -2147483649)
    def test_native_mul1(self):
        a = 20
        b = 32
        self.assertEqual(a * b, 640)
    def test_native_overflow_mul1(self):
        a = 0x7FFFFFFF
        b = 2
        self.assertEqual(a * b, 0xFFFFFFFE)
    def test_native_overflow_mul2(self):
        a = -0x7FFFFFFF
        b = 2
        self.assertEqual(a * b, -0xFFFFFFFE)
#

if __name__ == "__main__":
    r = Test().run()
    print(str(r))
    print()
