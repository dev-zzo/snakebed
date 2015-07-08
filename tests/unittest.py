"""Implements all classes necessary to handle unit tests."""

import sys

class UnitTestException(Exception):
    """Base for all unit test related exceptions."""
    pass
class AssertionError(UnitTestException):
    """Raised when an assertion fails."""
    def __init__(self, msg=None):
        self.msg = msg
class _SkipTest(UnitTestException):
    """Raise this exception in a test to skip it."""
    pass

class TestResult(object):
    """Contains the results of test case(s) being run."""
    def __init__(self):
        self.results = []
        self._current_test = None

    def startTest(self, test_name):
        self._current_test = {
                'name': test_name,
                'result': 'unknown',
                'error': None,
            }
        self.results.append(self._current_test)
    def stopTest(self):
        ct = self._current_test
        self._current_test = None
    def addError(self, exinfo):
        ct = self._current_test
        ct['result'] = 'error'
        ct['error'] = exinfo
    def addFailure(self, exinfo):
        ct = self._current_test
        ct['result'] = 'failed'
        ct['error'] = exinfo
    def addSuccess(self):
        self._current_test['result'] = 'passed'
    def addSkip(self, reason=None):
        self._current_test['result'] = 'skipped'

class TestCase(object):
    """A collection of unit tests that verify something works."""
    
    def skipTest(self):
        """Call to skip the current test."""
        raise _SkipTest()

    def fail(self, msg=None):
        """Explicitly fail the test."""
        print(msg)
        raise AssertionError(msg)
    def assertTrue(self, expr, msg=None):
        if not expr:
            fail("expression is not true")
    def assertFalse(self, expr, msg=None):
        if expr:
            fail("expression is not false")
    def assertEqual(self, first, second, msg=None):
        if not first == second:
            fail("objects are not equal")
    def assertNotEqual(self, first, second, msg=None):
        if first == second:
            fail("objects are equal")
    def assertRaises(self, exc, callable, *args, **kwds):
        try:
            callable(*args, **kwds)
            fail("no exception raised")
        except BaseException as e:
            if e.__class__ is exc:
                return
            fail("incorrect exception type")

    def setUp(self):
        """Hook method for setting up the test fixture before exercising it."""
        pass
    
    def tearDown(self):
        """Hook method for deconstructing the test fixture after testing it."""
        pass

    def __call__(self, *args, **kwds):
        return self.run(*args, **kwds)
 
    def _run_single(self, test_name, result):
        """Execute a single test method and record the results."""
        result.startTest(test_name)
        try:
            success = False
            try:
                self.setUp()
            except _SkipTest:
                result.addSkip()
            except KeyboardInterrupt:
                print('KeyboardInterrupt!')
                raise
            except:
                result.addError(test_name, sys.exc_info())
            else:
                try:
                    m = getattr(self, test_name)
                    m()
                except KeyboardInterrupt:
                    print('KeyboardInterrupt!')
                    raise
                except AssertionError:
                    result.addFailure(sys.exc_info())
#                except _ExpectedFailure:
#                    result.addSuccess()
#                except _UnexpectedSuccess:
#                    result.addFailure(sys.exc_info())
                except _SkipTest:
                    result.addSkip()
                except:
                    result.addError(sys.exc_info())
                else:
                    success = True

                try:
                    self.tearDown()
                except KeyboardInterrupt:
                    print('KeyboardInterrupt!')
                    raise
                except:
                    result.addError(sys.exc_info())
                    success = False

            if success:
                result.addSuccess()
        finally:
            result.stopTest()

    def run(self, result=None):
        if not result:
            result = TestResult()
        for name in self.__class__.__dict__.iterkeys():
            if name.startswith("test_"):
                # print(name)
                self._run_single(name, result)
        return result
