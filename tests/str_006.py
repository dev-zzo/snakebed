# Verify str.find() is working

if "abcdefg".find("def") != 3:
    print("FAILED 1")
elif "abcdefg".find("def", 4) != -1:
    print("FAILED 2")
elif "abcdefgabc".find("abc", 3) != 7:
    print("FAILED 3")
elif "".find("") != 0:
    print("FAILED 4")
elif "".find("", 1) != -1:
    print("FAILED 5")
elif "".find("abc") != -1:
    print("FAILED 5")
elif "abc".find("xxx", 2800000, 1) != -1:
    print("FAILED 6")
else:
    print("PASSED")
