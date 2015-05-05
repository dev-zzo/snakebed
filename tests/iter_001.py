a = [11,22,33]
i = iter(a)
if i.next() == 11:
	if i.next() == 22:
		if i.next() == 33:
			try:
				i.next()
				print("FAILED 5")
			except StopIteration:
				print("PASSED")
			except:
				print("FAILED 4")
		else:
			print("FAILED 3")
	else:
		print("FAILED 2")
else:
	print("FAILED 1")
