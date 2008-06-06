#encoding: latin-1

# Aldrin
# Modular Sequencer
# Copyright (C) 2006,2007,2008 The Aldrin Development Team
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

"""
Provides application class and controls used in the aldrin main window.
"""

import gobject
gobject.threads_init()

import sys, os
# add the directory that the main module resides in to the python path,
# if its not already contained
modulepath = os.path.dirname(__file__)
if not modulepath in sys.path:
	sys.path = [modulepath] + sys.path

import contextlog
contextlog.init()

import errordlg
errordlg.install()

from eventbus import *

from gtkimport import gtk
import time

from utils import format_time, ticks_to_time, prepstr, linear2db, db2linear, filepath, \
	is_debug, question, error, add_scrollbars, file_filter, new_stock_image_toggle_button, \
	new_stock_image_button, message
import utils

from zzub import Player
import zzub
player = None
playstarttime = None

import sequencer, router, patterns, wavetable, preferences, hdrecorder, cpumonitor, info, common, rack
from sequencer import SequencerPanel
from router import RoutePanel
from patterns import PatternPanel
from wavetable import WavetablePanel
from rack import RackPanel
from info import InfoPanel
from preferences import show_preferences
import config
import about
import envelope
import driver
import cairo
from common import MARGIN, MARGIN2, MARGIN3, MARGIN0

import os, sys

import audiogui

from aldrincom import com

app = None



# end of class AldrinFrame
import random

import Queue, time

class EventPlayer:
	"""
	replays timed events.
	"""
	def __init__(self):
		self.reset()
		
	def reset(self):
		self.offset = None
		self.starttime = None
		self.queue = []
		
	def put(self, v, t):
		if (not self.offset) or (self.offset > t):
			self.offset = t #+ driver.get_audiodriver().get_latency()
			self.starttime = time.time()
			self.lasttime = t
		t -= self.offset
		ts = self.starttime + t
		if len(self.queue) > 100:
			self.reset()
			return
		self.queue.insert(0, (ts,v))
	
	def get_next(self):
		if not len(self.queue):
			self.reset()
			return
		r = 0.0
		while len(self.queue):
			ct = time.time()
			t,v = self.queue[-1]
			if t >= ct:
				break
			r = max(r,self.queue.pop()[1])
		return r


class AldrinApplication:
	"""
	Application class. This one will be instantiated as a singleton.
	"""
	def main(self):
		"""
		Called when the main loop initializes.
		"""
		rootwindows = com.get_from_category('rootwindow')
		gtk.main()
		return 1

def main():
	global app
	app = AldrinApplication()
	app.main()
	if player:
		driver.get_mididriver().destroy()
		driver.get_audiodriver().destroy()

def run(argv):
	"""
	Starts the application and runs the mainloop.
	
	@param argv: command line arguments as passed by sys.argv.
	@type argv: str list
	"""
	global app
	com.load_packages()
	options = com.get('aldrin.core.options')
	options.parse_args(argv)
	app_options, app_args = options.get_options_args()
	if app_options.profile:
		import cProfile
		cProfile.runctx('main()', globals(), locals(), app_options.profile)
	else:
		main()

__all__ = [
'CancelException',
'AboutDialog',
'AldrinFrame',
'AmpView',
'MasterPanel',
'TimePanel',
'AldrinApplication',
'main',
'run',
]

if __name__ == '__main__':
	#~ import profile
	#~ profile.run('run(sys.argv)', 'aldrinprofile')
	run(sys.argv)
