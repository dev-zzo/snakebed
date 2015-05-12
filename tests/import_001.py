# Verify that importing built-in modules works
# This excercises the ImportName opcode
# NOTE: `import a, b, c` is just a syntactic sugar.
import sys

if sys.exit:
    print('PASSED')
else:
    print('FAILED')
