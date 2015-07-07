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
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.assertEqual(sock.family, socket.AF_INET)
        self.assertEqual(sock.type, socket.SOCK_STREAM)
        self.assertEqual(sock.proto, 0)
        sock.close()
#

if __name__ == "__main__":
    r = ProtocolIndependentTests().run()
    for i in r.results:
        print(i['name'])
        print(i['result'])
