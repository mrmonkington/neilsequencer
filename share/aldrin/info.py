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

from wximport import wx

class InfoPanel(wx.Panel):
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
		wx.Panel.__init__(self, *args, **kwds)
		self.view = InfoView(rootwindow, self, -1)
		sizer_2 = wx.BoxSizer(wx.HORIZONTAL)
		sizer_2.Add(self.view, 1, wx.EXPAND, 0)
		self.SetAutoLayout(True)
		self.SetSizer(sizer_2)
		self.Layout()
		
	def reset(self):
		"""
		Resets the router view. Used when
		a new song is being loaded.
		"""
		self.view.reset()
		
	def update_all(self):		
		self.view.update()

class InfoView(wx.TextCtrl):
	"""
	Allows to enter and view text saved with the module.
	"""	
	
	def __init__(self, rootwindow, *args, **kwds):
		"""
		Initializer.
		
		@param rootwindow: Main window.
		@type rootwindow: wx.Frame
		"""
		kwds['style'] = wx.SUNKEN_BORDER | wx.TE_MULTILINE | wx.TE_RICH2 | wx.TE_BESTWRAP
		wx.TextCtrl.__init__(self, *args, **kwds)
		wx.EVT_TEXT(args[0], self.GetId(), self.on_edit)
		
	def on_edit(self, event):
		"""
		Handler for text changes.
		
		@param event: Event
		@type event: wx.Event
		"""
		player.set_infotext(self.GetValue())
		
	def reset(self):
		"""
		Resets the view.
		"""
		self.Clear()
		
	def update(self):
		"""
		Updates the view.
		"""
		text = player.get_infotext()
		#~ if not text:
			#~ text = "Composed with Aldrin.\n\nThe revolution will not be televised."
		self.SetValue(text)


_all__ = [
	'InfoPanel',
	'InfoView',
]

if __name__ == '__main__':
	import sys
	from main import run
	#sys.argv.append(filepath('demosongs/test.bmx'))
	run(sys.argv)
