# Verify str.find() is working

if "abcdefg".rfind("def") != 3:
    print("FAILED 1")
elif "abcdefg".rfind("def", 4) != -1:
    print("FAILED 2")
elif "abcdefgabc".rfind("abc", 0, 9) != 0:
    print("FAILED 3")
elif "".rfind("") != 0:
    print("FAILED 4")
elif "".rfind("", 1) != -1:
    print("FAILED 5")
elif "".rfind("abc") != -1:
    print("FAILED 5")
elif "abc".rfind("xxx", 2800000, 1) != -1:
    print("FAILED 6")
else:
    print("PASSED")
