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

    def testGetSampleRates(self):
        ndevices = zzub.zzub_audiodriver_get_count(self.driver)
        maxrates = 20
        for i in range(ndevices):
            result = (ctypes.c_int * maxrates)()
            zzub.zzub_audiodriver_get_supported_samplerates(self.driver, i, result, maxrates)
            result = [rate for rate in result]
            self.assertNotEqual(result[0], 0)

if __name__ == '__main__':
    unittest.main()
