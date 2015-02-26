
def z(x, y=2, *args, **kwds):
    """docstring"""
    return kwds['aaa']
    
z(1, 4, 55, kw=42)
