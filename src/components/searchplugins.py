#encoding: latin-1

# Neil
# Modular Sequencer
# Copyright (C) 2006,2007,2008,2009,2010 The Neil Development Team
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, 
# Boston, MA  02110-1301, USA.

import gtk
import gobject
import aldrin.utils as utils, os, stat
from aldrin.utils import new_stock_image_toggle_button, ObjectHandlerGroup
import aldrin.common as common
import aldrin.com as com
import zzub

class SearchPluginsDialog(gtk.Dialog):

    __aldrin__ = dict(
        id = 'aldrin.core.searchplugins',
        singleton = True,
        categories = [
            'viewdialog',
            'view',
            ]
	)
	
    __view__ = dict(
        label = "Search Plugins",
        order = 0,
        toggle = True,
	)
	

__all__ = [
    'SearchPluginsDialog',
]

__aldrin__ = dict(
    classes = [
        SearchPluginsDialog,
	],
)

if __name__ == '__main__':
    pass
