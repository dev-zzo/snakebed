# Verify that del `name` works properly

a = 1
del a
try:
	b = a
	print("FAILED 1")
except NameError:
	print("PASSED")
except:
	print("FAILED 2")
