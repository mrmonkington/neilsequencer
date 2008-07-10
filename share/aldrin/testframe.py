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

from gtkimport import gtk
import aldrincom
com = aldrincom.com

class TestDialog(gtk.Dialog):
	"""
	A test dialog for testing embedded views.
	"""
	__aldrin__ = dict(
		id = 'aldrin.test.dialog',
		singleton = False,
		categories = [
		]
	)
	
	def __init__(self, embed=None):
		gtk.Dialog.__init__(self)
		self.set_title("Test Dialog")
		self.set_default_size(500, 400)
		self.connect('destroy', self.on_destroy)
		if embed:
			self.vbox.add(embed)
		self.show_all()
		
	def on_destroy(self, event):
		gtk.main_quit()


__aldrin__ = dict(
	classes = [
		TestDialog,
	],
)

if __name__ == '__main__':
	aldrincom.init()
	view = com.get('aldrin.core.routerpanel')
	dlg = com.get('aldrin.test.dialog', view)
	gtk.main()


