# Test exceptions being raised when handling exceptions internally
try:
    try:
        raise KeyError
    except BogusError:
        print('FAILED 1')
    except:
        print('FAILED 2')
except KeyError:
    print('FAILED 3')
except NameError:
    print('PASSED')
