#encoding: latin-1

# Aldrin
# Modular Sequencer
# Copyright (C) 2006 The Aldrin Development Team
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

import aldrin.interface
from aldrin.interface import IExtension, IUIBuilder
from aldrin import interface
import zzub
import gtk
import gobject
import time
import os
from aldrin.utils import prepstr, new_listview, is_generator, message
from aldrin.patterns import key_to_note, get_str_from_param

class TickDoubler(gtk.Dialog):
	def __init__(self, exthost):
		gtk.Dialog.__init__(self, "double your tick size")
		self.player = exthost.get_player()
		self.plugin = 0
		self.resize(250, 10)

		self.double_button = gtk.Button("Double")
		self.double_button.connect("clicked", self.on_double, 2)
		self.halve_button= gtk.Button("Halve")
		self.halve_button.connect("clicked", self.on_double, 0.5)
		hbox = gtk.HBox(False, 6)
		hbox.pack_start(self.double_button, expand=True)
		hbox.pack_start(self.halve_button, expand=True)
		print self.vbox				
		self.vbox.pack_start(hbox, expand=False)
		self.connect('button-press-event', self.on_left_down)

	def double_pattern(self, plugin, pattern):
		"""
		Callback that doubles the length of the current pattern while
		keeping notes intact
		"""
		pattern_index=[]
		pattern_contents=[]
		pl = plugin.get_pluginloader()
		for r,g,t,i in self.pattern_range(plugin, pattern):
			pattern_index.append((r,g,t,i))
			pattern_contents.append(pattern.get_value(r,g,t,i))
			param = pl.get_parameter(g,i)
			pattern.set_value(r,g,t,i,param.get_value_none())
		item=0
		pattern.set_row_count(pattern.get_row_count()*2)
		for r,g,t,i in pattern_index:
			pattern.set_value(r*2,g,t,i,pattern_contents[item])
			item+=1
		
	def halve_pattern(self, plugin, pattern):
		"""
		Callback that halves the length of the current pattern while
		keeping notes intact
		"""
		if pattern.get_row_count()==1:
			return
		for r,g,t,i in self.pattern_range(plugin, pattern):
			if r%2:
				continue
			pattern.set_value(r/2,g,t,i,pattern.get_value(r,g,t,i))
		pattern.set_row_count(pattern.get_row_count()/2)

	def pattern_range(self, plugin, pattern):
		"""
		Iterator that moves through the entire pattern.
		
		@return: Tuple pair of the next position (row, group, track, index)
		@rtype: (int, int, int, int)
		"""		
		row_count = pattern.get_row_count()
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
		seq = self.player.get_current_sequencer()
		for track in seq.get_track_list():
			events = track.get_event_list()
			for row, pattern_index in events:
				track.remove_event_at(row)	
			for row, pattern_index in events:
				track.set_event(int(multiplier*row), pattern_index)
			
		# change pattern sizes
		for plugin in self.player.get_plugin_list():
			print plugin
			for id in range(plugin.get_pattern_count()):
				pattern = plugin.get_pattern(id)
				if multiplier == 2:
					self.double_pattern(plugin, pattern)
				else:
					self.halve_pattern(plugin, pattern)		
		message(self, "Resizing complete")
	

	def on_left_down(self, widget, event, data=None):
		self.grab_focus()
		print 'ping'


class Extension(IExtension, IUIBuilder):
        __uri__ = '@zzub.org/extension/tickdoubler;1'
        SERVICE_URI = '@zzub.org/extension/tickdoubler/uibuilder'
        
        def realize(self, extensionhost):
                # store the host reference
                self.exthost = extensionhost
                # get the extension manager
                extman = extensionhost.get_extension_manager()
                # register our service
                extman.register_service(self.SERVICE_URI, self)
                # add our service to the ui builder class list
                extman.add_service_class(aldrin.interface.CLASS_UI_BUILDER, self.SERVICE_URI)
                # create browser
                self.browser = TickDoubler(self.exthost)
                self.browser.connect('delete-event', self.browser.hide_on_delete)

        # IUIBuilder.extend_menu
        def extend_menu(self, menuuri, menu, **kargs):
                if menuuri == aldrin.interface.UIOBJECT_MAIN_MENU_TOOLS:
                        # create a menu item
                        item = gtk.MenuItem(label="Tickdoubler")
                        # connect the menu item to our handler
                        item.connect('activate', self.on_menuitem_activate)
                        # append the item to the menu
                        menu.append(item)
                        return True
                        
        def on_menuitem_activate(self, widget):
                self.browser.show_all()
        
        def finalize(self):
                # get rid of the host reference
                del self.exthost
                
