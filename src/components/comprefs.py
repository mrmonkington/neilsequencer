#encoding: latin-1

# Neil
# Modular Sequencer
# Copyright (C) 2006,2007,2008 The Neil Development Team
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
	os.system('../../bin/neil-combrowser neil.core.pref.components')
	raise SystemExit

import gtk
import gobject
import neil.com as com
from neil.common import MARGIN, MARGIN2, MARGIN3
from neil.utils import new_listview, add_scrollbars

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
	
	__neil__ = dict(
		id = 'neil.core.pref.components',
		categories = [
			'neil.prefpanel',
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
			('Icon', str, dict(icon=True)),
			('Name', str, dict(markup=True)),
			(None, gobject.TYPE_PYOBJECT),
		])
		self.compolist.set_headers_visible(False)
		def cmp_package(a,b):
			return cmp(a.name.lower(), b.name.lower())
		packages = sorted(com.get_packages(), cmp_package)
		for package in packages:
			text = '<b>' + package.name + '</b>' + '\n'
			text += package.description
			store.append([True, package.icon, text, package])
		fssizer.pack_start(add_scrollbars(self.compolist))
		self.add(frame1)
		
	def apply(self):
		"""
		Writes general config settings to file.
		"""
		pass

__neil__ = dict(
	classes = [
		ComponentPanel,
	],
)

