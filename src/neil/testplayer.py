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
Provides a test player for testcases.
"""

from gtkimport import gtk
import gobject
import config
from config import get_plugin_aliases, get_plugin_blacklist
import common

_player = None

event_handlers = []

def player_callback(player, plugin, data):
	"""
	Default callback for ui events sent by zzub.
	"""
	result = False
	for handler in event_handlers:
		result = handler(player,plugin,data) or result
	return result

class TestWindow(gtk.Window):
	def __init__(self):
		gtk.Window.__init__(self)
		self.event_handlers = event_handlers
		self.resize(640,480)
		self.connect('destroy', lambda widget: gtk.main_quit())
		self.show_all()
		get_player()

def get_player():
	global _player
	if _player:
		return _player
	import zzub, driver
	player = common.get_player()
	_player = player
	player.set_callback(player_callback)
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
	try:
		driver.get_audiodriver().init()
	except:
		import traceback
		traceback.print_exc()
	try:
		driver.get_mididriver().init()
	except:
		import traceback
		traceback.print_exc()
	def handle_events(player):
		player.handle_events()
		return True
	gobject.timeout_add(1000/25, handle_events, player)
	return player

if __name__ == '__main__':
	player = get_player()
