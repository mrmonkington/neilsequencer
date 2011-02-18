
"""
Test Suite for PyZZUB
"""

import os, sys
CWD = os.path.abspath(os.path.join(os.path.dirname(__file__)))
module_path = os.path.normpath(os.path.join(CWD, '../src'))
sys.path = [module_path] + sys.path

import time
from unittest import TestCase, main

class Test(TestCase):
	def setUp(self):
		import neil.com as com
		com.init()
		
	def tearDown(self):
		import neil.com as com
		com.clear()
	
	def test_pref_dialog(self):
		import neil.com as com
		p = com.get('neil.core.player')
		self.assertTrue(p)

	def test_part2(self):
		import neil.com as com
		p = com.get('neil.core.window.root')
		self.assertTrue(p)

if __name__ == '__main__':
    main()


