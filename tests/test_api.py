import os, sys
import time
import unittest
import zzub
import ctypes

######################################
# This tests PortAudio playback driver
######################################

class TestPortAudio(unittest.TestCase):
    def setUp(self):
        self.player = zzub.zzub_player_create()
        self.driver = zzub.zzub_audiodriver_create_portaudio(self.player)

    def testGetCount(self):
        """Check if the number of devices is not zero.
        """
        ndevices = zzub.zzub_audiodriver_get_count(self.driver)
        self.assertNotEqual(ndevices, 0)

    def testGetName(self):
        """Check if zzub_audiodriver_get_name returns a proper string.
        """
        ndevices = zzub.zzub_audiodriver_get_count(self.driver)
        for i in range(ndevices):
            name = (ctypes.c_char * 1024)()
            zzub.zzub_audiodriver_get_name(self.driver, i, name, 1024)
            self.assertGreater(len(ctypes.string_at(name)), 0)

    def testGetSampleRates(self):
        """Check if the device sample rate list is not empty.
        """
        ndevices = zzub.zzub_audiodriver_get_count(self.driver)
        maxrates = 20
        for i in range(ndevices):
            result = (ctypes.c_int * maxrates)()
            zzub.zzub_audiodriver_get_supported_samplerates(self.driver, i, result, maxrates)
            result = [rate for rate in result]
            self.assertNotEqual(result[0], 0)

if __name__ == '__main__':
    unittest.main()
