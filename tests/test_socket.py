"""
This is a test suite for the _socket built-in C module.
"""

import unittest
import socket

class ProtocolIndependentTests(unittest.TestCase):
    def test_constants(self):
        "Verify certain critical constants are available"
        socket.AF_INET
        socket.SOCK_STREAM
        socket.SOCK_DGRAM

    def test_attributes(self):
        "Verify the socket object exposes expected attributes correctly"
        sock = socket.socket()
        try:
            self.assertEqual(sock.family, socket.AF_INET)
            self.assertEqual(sock.type, socket.SOCK_STREAM)
            self.assertEqual(sock.proto, 0)
        finally:
            sock.close()

    def test_settimeout(self):
        "Verify timeout can be set and read back"
        sock = socket.socket()
        try:
            sock.settimeout(5)
            self.assertEqual(sock.gettimeout(), 5)
        finally:
            sock.close()

    def test_connect_timeout(self):
        "Verify the connect() method times out as set"
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            sock.settimeout(1)
            # NOTE: Currently, no HTTP service is on Google's DNS service...
            self.assertRaises(socket.SocketTimeoutError, sock.connect, ("8.8.8.8", 80))
        finally:
            sock.close()
#

if __name__ == "__main__":
    r = ProtocolIndependentTests().run()
    print(str(r))
    print()
