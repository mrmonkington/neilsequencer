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
Contains all classes and functions needed to render the rack view.
"""

from gtkimport import gtk
import pango
import gobject
import common
player = common.get_player()
from common import MARGIN, MARGIN2, MARGIN3, MARGIN0

class RackPanel(gtk.VBox):
	"""
	Rack panel.
	
	Displays controls for individual plugins.
	"""
	def __init__(self, rootwindow):
		"""
		Initialization.
		"""
		self.rootwindow = rootwindow
		gtk.VBox.__init__(self)
