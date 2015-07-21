"""Verify base class __init__() can be called properly."""

class A:
    def __init__(self):
        print("A's __init__ called")
class B(A):
    def __init__(self):
        print("B's __init__ called")
        A.__init__(self)
class C(B):
    def __init__(self):
        print("C's __init__ called")
        B.__init__(self)
x = C()
