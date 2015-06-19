# Verify str.ljust() works (positive 1)
if 'aaa'.ljust(6, '=') == 'aaa===':
    print('PASSED')
else:
    print('FAILED')
