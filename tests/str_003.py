# Verify str.ljust() works (positive 2)
if 'aaa'.ljust(0, '=') == 'aaa':
    print('PASSED')
else:
    print('FAILED')
