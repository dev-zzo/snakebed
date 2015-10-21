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

    def test_shl_native_simple(self):
        a = 0x03200
        self.assertEqual(a << 1, 0x06400)
    def test_shl_native_conv(self):
        a = 0x40000000
        self.assertEqual(a << 1, 0x80000000)
    def test_shl_long_simple1(self):
        a = 0x100000000L
        self.assertEqual(a << 1, 0x200000000L)
    def test_shl_long_simple2(self):
        a = -0x100000000L
        b = a << 1
        self.assertEqual(b, -0x200000000L)
        self.assertTrue(b < 0)
    def test_shl_long_into_sign(self):
        a = 0x400000000000L
        b = a << 1
        self.assertEqual(b, 0x800000000000L)
        self.assertTrue(b > 0)
        
    def test_shr_native_simple(self):
        a = 0x03200
        self.assertEqual(a >> 4, 0x00320)
    def test_shl_long_simple1(self):
        a = 0x123400000L
        self.assertEqual(a >> 4, 0x12340000L)
    def test_shl_long_simple2(self):
        a = -0x123400000L
        self.assertEqual(a >> 4, -0x12340000L)
#

if __name__ == "__main__":
    r = Test().run()
    print(str(r))
    print()
