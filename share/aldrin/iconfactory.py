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

from aldrincom import com
from gtkimport import gtk

import glob, os
from utils import filepath, get_root_folder_path

ICON_SEARCHPATH = [
	'icons/16x16',
	'icons/22x22',
	'icons/24x24',
	'icons/32x32',
	'icons/48x48',
	'icons/scalable',
	'icons',
	'res',
]

ICON_EXTENSIONS = [
	'.svg',
	'.png',
]

class IconLibrary:
	"""
	registers icons to gtk and the rest of the application in a simple,
	unified way.
	"""
	
	__aldrin__ = dict(
		id = 'aldrin.core.icons',
		singleton = True,
	)	
	
	def __init__(self):
		self.iconfactory = gtk.IconFactory()
		self.iconfactory.add_default()
		self.iconsets = {}
		for searchpath in ICON_SEARCHPATH:
			for ext in ICON_EXTENSIONS:
				mask = filepath(searchpath) + '/*' + ext
				for filename in glob.glob(mask):
					key = os.path.splitext(os.path.basename(filename))[0]
					if not key in self.iconsets:
						self.iconsets[key] = gtk.IconSet()
					iconset = self.iconsets[key]
					iconsource = gtk.IconSource()
					iconsource.set_filename(filename)
					iconset.add_source(iconsource)
					
	def get_icon(self, iconset):
		return self.iconsets.get(iconset,None)
	
	def register_single(self, stockid, label, key='', iconset=None):
		if iconset:
			self.iconfactory.add(stockid, self.iconsets[iconset])
		if key:
			key = gtk.gdk.keyval_from_name(key)
		else:
			key = 0
		gtk.stock_add((
			(stockid, label, 0, key, 'aldrin'),
		))

__aldrin__ = dict(
	classes = [
		IconLibrary,
	],
)

if __name__ == '__main__':
	pass
