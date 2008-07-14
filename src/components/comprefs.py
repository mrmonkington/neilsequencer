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
Provides a preference tab to organize components.
"""

if __name__ == '__main__':
	import os
	os.system('../../bin/aldrin-combrowser aldrin.core.pref.components')
	raise SystemExit

import gtk
import gobject
import aldrin.com as com
from aldrin.common import MARGIN, MARGIN2, MARGIN3
from aldrin.utils import new_listview, add_scrollbars

#OPTIONS = [
#	'Module',
#	'Name',
#	'Description',
#	'Icon',
#	'Authors',
#	'Copyright',
#	'Website',

class ComponentPanel(gtk.VBox):
	"""
	Panel which allows changing of general settings.
	"""
	
	__aldrin__ = dict(
		id = 'aldrin.core.pref.components',
		categories = [
			'aldrin.prefpanel',
		]
	)
	
	__prefpanel__ = dict(
		label = "Components",
	)
	
	def __init__(self):
		"""
		Initializing.
		"""
		gtk.VBox.__init__(self)
		self.set_border_width(MARGIN)
		frame1 = gtk.Frame("Components")
		fssizer = gtk.VBox(False, MARGIN)
		fssizer.set_border_width(MARGIN)
		frame1.add(fssizer)
		self.compolist, store, columns = new_listview([
			('Use', bool),
			('Name', str, dict(markup=True,wrap=True)),
			(None, gobject.TYPE_PYOBJECT),
		])
		def cmp_package(a,b):
			return cmp(a.name.lower(), b.name.lower())
		packages = sorted(com.get_packages(), cmp_package)
		for package in packages:
			text = '<b><big>' + package.name + '</big></b>\n' + package.description
			store.append([True, text, package])
		fssizer.pack_start(add_scrollbars(self.compolist))
		self.add(frame1)
		
	def apply(self):
		"""
		Writes general config settings to file.
		"""
		pass

__aldrin__ = dict(
	classes = [
		ComponentPanel,
	],
)

