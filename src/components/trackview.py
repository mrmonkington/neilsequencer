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
Contains all classes and functions needed to render the sequence
editor and its associated components.
"""

if __name__ == '__main__':
	import os
	os.system('../../bin/aldrin-combrowser aldrin.core.trackviewpanel')
	raise SystemExit
	
import gtk
import pango
import gobject
from aldrin.utils import prepstr, from_hsb, to_hsb, get_item_count, \
	get_clipboard_text, set_clipboard_text, add_scrollbars
from aldrin.utils import is_effect,is_generator,is_controller,is_root, \
	get_new_pattern_name, filepath
import random
import config
import aldrin.common as common
MARGIN = common.MARGIN
MARGIN2 = common.MARGIN2
MARGIN3 = common.MARGIN3
MARGIN0 = common.MARGIN0
import aldrin.com as com

class SequencerPanel(gtk.VBox):
	"""
	Sequencer pattern panel.
	
	Displays all the patterns available for the current track.
	"""
	__aldrin__ = dict(
		id = 'aldrin.core.trackviewpanel',
		singleton = True,
		categories = [
			'aldrin.viewpanel',
			'view',
		]
	)	
	
	__view__ = dict(
			label = "Tracks",
			stockid = "aldrin_sequencer",
			shortcut = '<Shift>F4',
			order = 4,
	)
	
	def __init__(self):
		"""
		Initialization.
		"""
		gtk.VBox.__init__(self)
		self.splitter = gtk.HPaned()
		self.tracklabels = gtk.VBox()
		self.trackview = gtk.VBox()
		
		self.splitter.pack1(self.tracklabels, False, False)
		self.splitter.pack2(self.trackview, True, True)

		self.pack_start(self.splitter)

__aldrin__ = dict(
	classes = [
		SequencerPanel,
	],
)

