# Verify str.rjust() works (positive)
if 'aaa'.rjust(6, '=') == '===aaa':
    print('PASSED')
else:
    print('FAILED')
