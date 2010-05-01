#encoding: latin-1

# Neil
# Modular Sequencer
# Copyright (C) 2006,2007,2008 The Neil Development Team
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
A view that allows browsing available extension interfaces and documentation.

This module can also be executed standalone.
"""

import neil.com as com
import zzub
import gtk
import gobject
import time
import os
from neil.utils import prepstr, new_listview, is_generator, message

class TickDoublerDialog(gtk.Dialog):
	__neil__ = dict(
		id = 'neil.tickdoubler.dialog',
		singleton = True,
	)
	
	def __init__(self, hide_on_delete=True):
		gtk.Dialog.__init__(self, "Change resolution of song")
		
		# do not destroy dialog on close
		if hide_on_delete:
			self.connect('delete-event', self.hide_on_delete)
			
		self.plugin = 0
		self.resize(250, 10)

		self.double_button = gtk.Button("Double")
		self.double_button.connect("clicked", self.on_double, 2)
		self.halve_button= gtk.Button("Halve")
		self.halve_button.connect("clicked", self.on_double, 0.5)
		hbox = gtk.HBox(False, 6)
		hbox.pack_start(self.double_button, expand=True)
		hbox.pack_start(self.halve_button, expand=True)
		self.vbox.pack_start(hbox, expand=False)
		self.connect('button-press-event', self.on_left_down)

	def double_pattern(self, plugin, pattern_index):
		"""
		Callback that doubles the length of the current pattern while
		keeping notes intact
		"""
		pattern_indices=[]
		pattern_contents=[]
		pl = plugin.get_pluginloader()
		for r,g,t,i in self.pattern_range(plugin, pattern_index):
			pattern_indices.append((r,g,t,i))
			pattern_contents.append(plugin.get_pattern_value(pattern_index, g, t, i, r))
			param = pl.get_parameter(g,i)
			plugin.set_pattern_value(pattern_index, g, t, i, r, param.get_value_none())
		item=0
		plugin.set_pattern_length(pattern_index, plugin.get_pattern_length(pattern_index)*2)
		for r,g,t,i in pattern_indices:
			plugin.set_pattern_value(pattern_index, g, t, i, r*2, pattern_contents[item])	
			item+=1
		
	def halve_pattern(self, plugin, pattern_index):
		"""
		Callback that halves the length of the current pattern while
		keeping notes intact
		"""
		if  plugin.get_pattern_length(pattern_index)==1:
			return
		for r,g,t,i in self.pattern_range(plugin, pattern_index):
			if r%2:
				continue
			plugin.set_pattern_value(pattern_index, g, t, i, r/2, plugin.get_pattern_value(pattern_index, g, t, i, r))	
			
		plugin.set_pattern_length(pattern_index, plugin.get_pattern_length(pattern_index)/2)

	def pattern_range(self, plugin, pattern_index):
		"""
		Iterator that moves through the entire pattern.
		
		@return: Tuple pair of the next position (row, group, track, index)
		@rtype: (int, int, int, int)
		"""		
		row_count = plugin.get_pattern_length(pattern_index)
		group_track_count = [plugin.get_input_connection_count(),  1, plugin.get_track_count()]
		parameter_count = [plugin.get_pluginloader().get_parameter_count(group) for group in range(3)]
		for row in range(0, row_count):
			for group in range(3):
				tc = group_track_count[group]
				for track in range(0, tc):
					for index in range(0, parameter_count[group]):
						yield (row, group, track, index)
						
	def on_double(self, widget, multiplier):
		# multiply pattern positions
		player = com.get('neil.core.player')
		player.set_callback_state(False)
		seq = player.get_current_sequencer()
		for track in seq.get_track_list():
			events = list(track.get_event_list())
			
			for row, pattern_index in events:
				track.set_event(row, -1)
			for row, pattern_index in events:
				track.set_event(int(multiplier*row), pattern_index)
		# change pattern sizes
		for plugin in player.get_plugin_list():
			#print plugin
			for id in range(plugin.get_pattern_count()):
				pattern = plugin.get_pattern(id)
				if multiplier == 2:
					self.double_pattern(plugin, id)
				else:
					self.halve_pattern(plugin, id)		
		player.history_commit("tick resolution resizing")
		player.set_callback_state(True)
		eventbus = com.get('neil.core.eventbus')
		eventbus.document_loaded()	
		message(self, "Resizing complete")
	
	def on_left_down(self, widget, event, data=None):
		self.grab_focus()


class TickDoublerMenuItem:
	__neil__ = dict(
		id = 'neil.tickdoubler.menuitem',
		singleton = True,
		categories = [
			'menuitem.tool'
		],
	)
	
	def __init__(self, menu):
		# create a menu item
		item = gtk.MenuItem(label="Tick Doubler")
		# connect the menu item to our handler
		item.connect('activate', self.on_menuitem_activate)
		# append the item to the menu
		menu.append(item)
		
	def on_menuitem_activate(self, widget):
		browser = com.get('neil.tickdoubler.dialog')
		browser.show_all()

__neil__ = dict(
	classes = [
		TickDoublerDialog,
		TickDoublerMenuItem,
	],
)
                
