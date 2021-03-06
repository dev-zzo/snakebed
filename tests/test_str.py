import unittest

class Test(unittest.TestCase):
    def test_ctor(self):
        self.assertEqual(str('aaa'), 'aaa')
    def test_join(self):
        self.assertEqual('XXX'.join(['a','b','c']), 'aXXXbXXXc')
    def test_justify(self):
        self.assertEqual('aaa'.ljust(6, '='), 'aaa===')
        self.assertEqual('aaa'.ljust(0, '='), 'aaa')
        self.assertEqual('aaa'.rjust(6, '='), '===aaa')
        self.assertEqual('aaa'.center(7, '='), '==aaa==')
    def test_find(self):
        self.assertEqual("abcdefg".find("def"), 3)
        self.assertEqual("abcdefg".find("def", 4), -1)
        self.assertEqual("abcdefgabc".find("abc", 3), 7)
        self.assertEqual("".find(""), 0)
        self.assertEqual("".find("", 1), -1)
        self.assertEqual("".find("abc"), -1)
        self.assertEqual("abc".find("xxx", 2800000, 1), -1)
    def test_rfind(self):
        self.assertEqual("abcdefg".rfind("def"), 3)
        self.assertEqual("abcdefg".rfind("def", 4), -1)
        self.assertEqual("abcdefgabc".rfind("abc", 0, 9), 0)
        self.assertEqual("".rfind(""), 0)
        self.assertEqual("".rfind("", 1), -1)
        self.assertEqual("".rfind("abc"), -1)
        self.assertEqual("abc".rfind("xxx", 2800000, 1), -1)
    def test_format_str(self):
        self.assertEqual("meh{0}teh".format("or"), "mehorteh")
        self.assertEqual("{0:<16}".format("abc"), "abc             ")
        self.assertEqual("{0:>16}".format("abc"), "             abc")
        self.assertEqual("{0:^16}".format("abc"), "      abc       ")
        self.assertEqual("{0:>7.4}".format("abcdefg"), "   abcd")
    def test_format_int(self):
        self.assertEqual("left{0}right".format(-1024), "left-1024right")
        self.assertEqual("{0:<16}".format(-1024), "-1024           ")
        self.assertEqual("{0:>16}".format(-1024), "           -1024")
        self.assertEqual("{0:^16}".format(-1024), "     -1024      ")
        self.assertEqual("{0:A=+16}".format(-1024), "-AAAAAAAAAAA1024")
        self.assertEqual("{0:A=+16}".format(1024), "+AAAAAAAAAAA1024")
#

if __name__ == "__main__":
    r = Test().run()
    print(str(r))
    print()
