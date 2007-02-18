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
Provides a test player for testcases.
"""

from gtkimport import gtk
import config
from config import get_plugin_aliases, get_plugin_blacklist
import common

class TestWindow(gtk.Window):
	def __init__(self):
		gtk.Window.__init__(self)
		self.event_handlers = []
		self.resize(640,480)
		self.connect('destroy', lambda widget: gtk.main_quit())
		self.show_all()

def get_player():
	import zzub
	player = common.get_player()
	# load blacklist file and add blacklist entries
	for name in get_plugin_blacklist():
		player.blacklist_plugin(name)
	# load aliases file and add aliases
	for name,uri in get_plugin_aliases():
		player.add_plugin_alias(name, uri)
	pluginpaths = [
		'/usr/local/lib64/zzub',
		'/usr/local/lib/zzub',
		'/usr/lib64/zzub',
		'/usr/lib/zzub',
	]
	for pluginpath in pluginpaths:
		player.add_plugin_path(pluginpath + '/')
	inputname, outputname, samplerate, buffersize = config.get_config().get_audiodriver_config()
	player.initialize(samplerate)
	return player

if __name__ == '__main__':
	player = get_player()
