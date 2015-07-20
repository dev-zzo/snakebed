"""Issue 42: calling type methods causes a crash."""

print(str(type("str")))
