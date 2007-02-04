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
Example script to demonstrate UI extensions for Aldrin.
"""

from aldrin.interface import IExtension, \
						MAINFRAME_SERVICE

import wx
import os

class Extension(IExtension):
	__uri__ = '@zzub.org/extension/testbutton;1'
	
	def realize(self, extensionhost):
		# store the host reference
		self.exthost = extensionhost
		# get the extension manager
		extman = extensionhost.get_extension_manager()
		# get the mainframe
		mainframe = extman.get_service(MAINFRAME_SERVICE)		
		# get our toolbar image location
		imagepath = extensionhost.resolve_path('demo.png')
		# check it exists
		assert os.path.isfile(imagepath), "%s not found." % imagepath
		# add a new button to the toolbar
		toolid = mainframe.add_tool_button(
			"Demo", 	# label
			wx.Bitmap(imagepath, wx.BITMAP_TYPE_ANY),	# image
			wx.NullBitmap, 	# disabled image
			wx.ITEM_NORMAL, # style
			"Demo Button", 	# tooltip
			"This is a button added by an extension.") # description
		# associate a handler
		mainframe.add_click_handler(toolid, self.on_button_click)
		
	def on_button_click(self, event):
		# get the extension manager
		extman = self.exthost.get_extension_manager()
		# get the mainframe service
		mainframe = extman.get_service(MAINFRAME_SERVICE)
		# get the mainframe window
		parent = mainframe.get_window()
		wx.MessageDialog(parent,
						message="You clicked the demo button.",
						caption = "Demo Button", style = wx.OK|wx.CENTER).ShowModal()
	
	def finalize(self):
		# get rid of the host reference
		del self.exthost
