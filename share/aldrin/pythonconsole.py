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
import code

class PythonConsoleDialog(gtk.Dialog):
	__aldrin__ = dict(
		id = 'aldrin.pythonconsole.dialog',
		singleton = True,
	)
	
	def __init__(self):
		gtk.Dialog.__init__(self,
			"Python Console")
		self.resize(600,500)
		self.locals = dict(
			__name__ = "__console__",
			__doc__ = None,
			com = com,
			embed = self.embed,
			gtk = gtk,
		)
		self.compiler = code.InteractiveConsole(self.locals)
		
		buffer = BUFFER_CLASS()
		
		view = VIEW_CLASS(buffer)
		cfg = com.get('aldrin.core.config')
		# "ProFontWindows 9"
		view.modify_font(pango.FontDescription(cfg.get_pattern_font('Monospace')))
		view.set_editable(False)
		view.set_wrap_mode(gtk.WRAP_WORD)
		self.consoleview = view
		self.buffer = buffer
		self.entry = gtk.Entry()
		self.entry.modify_font(pango.FontDescription(cfg.get_pattern_font('Monospace')))
		self.entry.connect('activate', self.on_entry_activate)
		self.textmark = self.buffer.create_mark(None, self.buffer.get_end_iter(), False)

		scrollwin = gtk.ScrolledWindow()
		scrollwin.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
		scrollwin.set_shadow_type(gtk.SHADOW_IN)
		scrollwin.add(self.consoleview)
		
		vpack = gtk.VBox()
		vpack.pack_start(scrollwin)
		vpack.pack_end(self.entry, False)
		self.vbox.add(vpack)
		
		gobject.timeout_add(50, self.update_output)
		self.log_buffer_pos = 0
		self.entry.grab_focus()
		
	def embed(self, widget):
		anchor = self.buffer.create_child_anchor(self.buffer.get_end_iter())
		self.consoleview.add_child_at_anchor(widget, anchor)
		widget.show_all()
		print
		
	def push_text(self, text):
		self.compiler.push(text)
		
	def on_entry_activate(self, widget):
		text = self.entry.get_text()
		self.entry.set_text("")
		print '>>> ' + text
		gobject.timeout_add(50, self.push_text, text)
		
	def update_output(self):
		while self.log_buffer_pos != len(contextlog.LOG_BUFFER):
			target, filename, lineno, text = contextlog.LOG_BUFFER[self.log_buffer_pos]
			self.buffer.insert(self.buffer.get_end_iter(), text) 
			self.log_buffer_pos += 1
			self.consoleview.scroll_mark_onscreen(self.textmark)
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
