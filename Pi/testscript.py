import unittest
import STIS

class testSTIS(unittest.TestCase):
    
    def test_underscore(self):
        print("test1")
        t1 = STIS.internetConnectionPresent()
        self.assertEqual(t1, True)
        