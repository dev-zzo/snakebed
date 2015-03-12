# Verify instance methods work (i.e. self is passed)
class A:
    def f(self):
        if type(self) is A:
            print('PASSED')
        else:
            print('FAILED')
A().f()
