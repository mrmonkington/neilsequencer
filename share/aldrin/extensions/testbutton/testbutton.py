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

import aldrin.interface
from aldrin.interface import IExtension, IUIBuilder

import gtk
import os

class Extension(IExtension, IUIBuilder):
	__uri__ = '@zzub.org/extension/testbutton;1'
	SERVICE_URI = '@zzub.org/extension/testbutton/uibuilder'
	
	# IUIBuilder.extend_toolbar
	def extend_toolbar(self, toolbaruri, toolbar):
		if toolbaruri == aldrin.interface.UIOBJECT_MAIN_TOOLBAR:
			# get our toolbar image location
			imagepath = self.exthost.resolve_path('demo.png')
			# check it exists
			assert os.path.isfile(imagepath), "%s not found." % imagepath
			# load an image
			image = gtk.Image()
			image.set_from_file(imagepath)
			# create new tool button with image
			item = gtk.ToolButton(image)
			# append tool button to toolbar
			toolbar.insert(item, -1)
			# connect our handler
			item.connect('clicked', self.on_toolbutton_clicked)
			return True
	
	# IExtension.realize
	def realize(self, extensionhost):
		# store the host reference
		self.exthost = extensionhost
		# get the extension manager
		extman = extensionhost.get_extension_manager()
		# register our service
		extman.register_service(self.SERVICE_URI, self)
		# add our service to the ui builder class list
		extman.add_service_class(aldrin.interface.CLASS_UI_BUILDER, self.SERVICE_URI)
		# store the message service for later
		self.msgsvc = extman.get_service(aldrin.interface.SERVICE_MESSAGE)
		
	def on_toolbutton_clicked(self, widget):
		self.msgsvc.message("You clicked the demo button.")
	
	# IExtension.finalize
	def finalize(self):
		# get rid of the host reference
		del self.exthost
