
class A:
    def __hash__(self):
        return 1
        
    def x(self):
        return hash(self)

a = A()
a.x()
