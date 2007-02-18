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
Provides an info view which allows to enter text.
"""

from gtkimport import gtk
import common
player = common.get_player()


class InfoPanel(gtk.VBox):
	"""
	Contains the info view.
	"""
	def __init__(self, rootwindow, *args, **kwds):
		"""
		Initializer.
		
		@param rootwindow: Main window.
		@type rootwindow: wx.Frame
		"""
		self.rootwindow = rootwindow
		gtk.VBox.__init__(self)
		self.view = InfoView(rootwindow)
		sizer_2 = gtk.HBox()
		sizer_2.add(self.view)
		self.add(sizer_2)
		
	def reset(self):
		"""
		Resets the router view. Used when
		a new song is being loaded.
		"""
		self.view.reset()
		
	def update_all(self):		
		self.view.update()

class InfoView(gtk.TextView):
	"""
	Allows to enter and view text saved with the module.
	"""	
	
	def __init__(self, rootwindow):
		"""
		Initializer.
		
		@param rootwindow: Main window.
		@type rootwindow: wx.Frame
		"""
		gtk.TextView.__init__(self)
		self.set_wrap_mode(gtk.WRAP_WORD)
		self.connect('insert-at-cursor', self.on_edit)
		self.connect('delete-from-cursor', self.on_edit)
		
	def on_edit(self, event):
		"""
		Handler for text changes.
		
		@param event: Event
		@type event: wx.Event
		"""
		player.set_infotext(self.get_buffer().get_property('text'))
		
	def reset(self):
		"""
		Resets the view.
		"""
		self.get_buffer().set_property('text', '')
		
	def update(self):
		"""
		Updates the view.
		"""
		text = player.get_infotext()
		#~ if not text:
			#~ text = "Composed with Aldrin.\n\nThe revolution will not be televised."
		self.get_buffer().set_property('text', text)


_all__ = [
	'InfoPanel',
	'InfoView',
]

if __name__ == '__main__':
	import sys
	from main import run
	#sys.argv.append(filepath('demosongs/test.bmx'))
	run(sys.argv)
