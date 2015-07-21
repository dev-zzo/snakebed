"""Implements all classes necessary to handle unit tests."""

import sys

class UnitTestException(Exception):
    """Base for all unit test related exceptions."""
    pass
class AssertionError(UnitTestException):
    """Raised when an assertion fails."""
    def __init__(self, msg=None):
        UnitTestException.__init__(self, msg)
        self.msg = msg
class _SkipTest(UnitTestException):
    """Raise this exception in a test to skip it."""
    pass

class TestResult(object):
    """Contains the results of test case(s) being run."""
    def __init__(self):
        self.skipped = []
        self.passed = []
        self.failed = []
        self.errored = []
        self._current_test = None
    def __str__(self):
        text = []
        if len(self.passed) > 0:
            text.append("Passed:\r\n" + ", ".join(self.passed))
        if len(self.failed) > 0:
            fails = [x[0] + ': ' + str(x[1][1]) + "\r\n" for x in self.failed]
            text.append("Failed:\r\n" + "".join(fails))
        if len(self.errored) > 0:
            errors = [x[0] + ': ' + x[1][0].__name__ + ': ' + str(x[1][1]) + "\r\n" for x in self.errored]
            text.append("Encountered errors:\r\n" + "".join(errors))
        if len(self.skipped) > 0:
            text.append("Skipped:\r\n" + ", ".join([x[0] for x in self.skipped]))
        return "\r\n".join(text)

    def startTest(self, test_name):
        if self._current_test is not None:
            pass
        self._current_test = test_name
    def stopTest(self):
        self._current_test = None
    def addError(self, exinfo):
        self.errored.append((self._current_test, exinfo))
    def addFailure(self, exinfo):
        self.failed.append((self._current_test, exinfo))
    def addSuccess(self):
        self.passed.append(self._current_test)
    def addSkip(self, reason=None):
        self.skipped.append((self._current_test, reason))

class TestCase(object):
    """A collection of unit tests that verify something works."""
    
    def skipTest(self):
        """Call to skip the current test."""
        raise _SkipTest()

    def fail(self, msg=None):
        """Explicitly fail the test."""
        # print(msg)
        raise AssertionError, msg
    def assertTrue(self, expr):
        if not expr:
            self.fail("expression is not true")
    def assertFalse(self, expr):
        if expr:
            self.fail("expression is not false")
    def assertEqual(self, first, second):
        if not first == second:
            self.fail("objects are not equal")
    def assertNotEqual(self, first, second):
        if first == second:
            self.fail("objects are equal")
    def assertRaises(self, exc, callable, *args, **kwds):
        try:
            callable(*args, **kwds)
            self.fail("no exception raised")
        except:
            exinfo = sys.exc_info()
            if exinfo[0] is exc:
                return
            self.fail("incorrect exception type")

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
