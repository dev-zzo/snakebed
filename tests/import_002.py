# Verify that importing built-in modules works
# This excercises the ImportFrom opcode
from sys import exit, modules

if exit:
    print('PASSED')
else:
    print('FAILED')
