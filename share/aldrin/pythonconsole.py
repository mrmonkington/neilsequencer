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
A view that allows browsing available extension interfaces and documentation.

This module can also be executed standalone.
"""

from gtkimport import gtk
import gobject
gobject.threads_init()
import os
import inspect

from aldrincom import com
import utils

import contextlog

import pango

MARGIN = 6

VIEW_CLASS = gtk.TextView
BUFFER_CLASS = gtk.TextBuffer

import thread
import time

class PythonConsoleDialog(gtk.Dialog):
	__aldrin__ = dict(
		id = 'aldrin.pythonconsole.dialog',
		singleton = True,
	)
	
	def __init__(self):
		gtk.Dialog.__init__(self,
			"Python Console")
		self.resize(600,500)
		
		buffer = BUFFER_CLASS()
		
		view = VIEW_CLASS(buffer)
		cfg = com.get('aldrin.core.config')
		# "ProFontWindows 9"
		view.modify_font(pango.FontDescription(cfg.get_pattern_font('Monospace')))
		self.consoleview = view
		self.buffer = buffer

		scrollwin = gtk.ScrolledWindow()
		scrollwin.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
		scrollwin.set_shadow_type(gtk.SHADOW_IN)
		scrollwin.add(self.consoleview)
		self.vbox.add(scrollwin)
		
		gobject.timeout_add(50, self.update_output)
		self.log_buffer_pos = 0
		
	def update_output(self):
		while self.log_buffer_pos != len(contextlog.LOG_BUFFER):
			target, filename, lineno, text = contextlog.LOG_BUFFER[self.log_buffer_pos]
			self.buffer.insert(self.buffer.get_end_iter(), text) 
			self.log_buffer_pos += 1
		return True

class PythonConsoleMenuItem:
	__aldrin__ = dict(
		id = 'aldrin.pythonconsole.menuitem',
		singleton = True,
		categories = [
			'menuitem.tool'
		],
	)
	
	def __init__(self, menu):
		# create a menu item
		item = gtk.MenuItem(label="Show _Python Console")
		# connect the menu item to our handler
		item.connect('activate', self.on_menuitem_activate)
		# append the item to the menu
		menu.append(item)
		
	def on_menuitem_activate(self, widget):
		browser = com.get('aldrin.pythonconsole.dialog')
		browser.connect('delete-event', browser.hide_on_delete)
		browser.show_all()

__aldrin__ = dict(
	classes = [
		PythonConsoleDialog,
		PythonConsoleMenuItem,
	],
)

if __name__ == '__main__': # extension mode
	import contextlog
	contextlog.init()
	com.load_packages()
	# running standalone
	browser = com.get('aldrin.pythonconsole.dialog')
	browser.connect('destroy', lambda widget: gtk.main_quit())
	browser.show_all()
	gtk.main()
