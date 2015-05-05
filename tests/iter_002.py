# Verify iteration via `for` works
a = [11,22,33]
sum = 0
try:
	for x in a:
		sum = sum + x
	if sum == 66:
		print("PASSED")
	else:
		print("FAILED 1")
except:
	print("FAILED 2")
