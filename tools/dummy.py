
class A:
    def __hash__(self):
        return 1
        
    def x(self):
        return hash(self)

a = A()
a.x(1, 2, 3, a=4, b=5)
b = A.x
