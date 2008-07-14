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
Provides a preference tab to organize components.
"""

import gtk
import aldrin.com as com
from aldrin.common import MARGIN, MARGIN2, MARGIN3

class ComponentPanel(gtk.VBox):
	"""
	Panel which allows changing of general settings.
	"""
	
	__aldrin__ = dict(
		id = 'aldrin.core.pref.components',
		categories = [
			'aldrin.prefpanel',
		]
	)
	
	__prefpanel__ = dict(
		label = "Components",
	)
	
	def __init__(self):
		"""
		Initializing.
		"""
		gtk.VBox.__init__(self)
		self.set_border_width(MARGIN)
		frame1 = gtk.Frame("General Settings")
		fssizer = gtk.VBox(False, MARGIN)
		fssizer.set_border_width(MARGIN)
		frame1.add(fssizer)
#		audioeditor = config.get_config().get_audioeditor_command()
#		incsave = config.get_config().get_incremental_saving()
#		leddraw = config.get_config().get_led_draw()
#		patnoteoff = config.get_config().get_pattern_noteoff()
#		self.patternfont = gtk.FontButton(config.get_config().get_pattern_font())
#		self.patternfont.set_use_font(True)
#		self.patternfont.set_use_size(True)
#		self.patternfont.set_show_style(True)
#		self.patternfont.set_show_size(True)
#		self.audioeditor = gtk.Entry()
#		self.incsave = gtk.CheckButton()
#		self.leddraw = gtk.CheckButton()
#		self.patnoteoff = gtk.CheckButton()
#		self.rackpanel = gtk.CheckButton()
#		self.audioeditor.set_text(audioeditor)
#		self.incsave.set_active(int(incsave))
#		self.leddraw.set_active(int(leddraw))
#		self.patnoteoff.set_active(int(patnoteoff))
#		sg1 = gtk.SizeGroup(gtk.SIZE_GROUP_HORIZONTAL)
#		sg2 = gtk.SizeGroup(gtk.SIZE_GROUP_HORIZONTAL)
#		def add_row(c1, c2):
#			row = gtk.HBox(False, MARGIN)
#			c1.set_alignment(1, 0.5)
#			sg1.add_widget(c1)
#			sg2.add_widget(c2)
#			row.pack_start(c1, expand=False)
#			row.pack_end(c2)
#			fssizer.pack_start(row, expand=False)
#		add_row(gtk.Label("External Sample Editor"), self.audioeditor)
#		add_row(gtk.Label("Incremental Saves"), self.incsave)
#		add_row(gtk.Label("Draw Amp LEDs in Router"), self.leddraw)
#		add_row(gtk.Label("Auto Note-Off in Pattern Editor"), self.patnoteoff)
#		add_row(gtk.Label("Pattern Font"), self.patternfont)
		self.add(frame1)
		
	def apply(self):
		"""
		Writes general config settings to file.
		"""
		pass

__aldrin__ = dict(
	classes = [
		ComponentPanel,
	],
)

