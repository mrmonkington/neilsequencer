import os, sys
import time
import unittest
import zzub
import ctypes

class TestPortAudio(unittest.TestCase):
    def setUp(self):
        self.player = zzub.zzub_player_create()
        self.driver = zzub.zzub_audiodriver_create_portaudio(self.player)

    def testGetCount(self):
        ndevices = zzub.zzub_audiodriver_get_count(self.driver)
        self.assertNotEqual(ndevices, 0)

    def testGetName(self):
        ndevices = zzub.zzub_audiodriver_get_count(self.driver)
        for i in range(ndevices):
            name = (ctypes.c_char * 1024)()
            zzub.zzub_audiodriver_get_name(self.driver, i, name, 1024)
            self.assertGreater(len(ctypes.string_at(name)), 0)

if __name__ == '__main__':
    unittest.main()
