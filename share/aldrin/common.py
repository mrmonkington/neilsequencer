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
Provides information used by all ui sections.
"""

import zzub

MARGIN0 = 3
MARGIN = 6
MARGIN2 = 12
MARGIN3 = 18

class PluginInfo(object):
	"""
	Encapsulates data associated with a plugin.
	"""
	def __init__(self, plugin):
		self.plugin = plugin
		self.muted = False
		self.pattern_position = (0, 0, 0, 0, 0)
		self.plugingfx = None
		self.patterngfx = {}
		self.amp = -9999.0
		
	def reset_patterngfx(self):
		self.patterngfx = {}
		
	def reset_plugingfx(self):
		self.plugingfx = None
		self.amp = -9999.0
		
class PluginInfoCollection:
	"""
	Manages plugin infos.
	"""
	def __init__(self):
		self.plugin_info = {}
		self.update()
		
	def reset(self):
		self.plugin_dialogs = {}
		
	def __getitem__(self, k):
		return self.plugin_info.__getitem__(k)
		
	def __delitem__(self, k):
		return self.plugin_info.__delitem__(k)
		
	def get(self, k):
		if not k in self.plugin_info:
			self.add_plugin(k)
		return self.plugin_info.__getitem__(k)
		
	def iteritems(self):
		return self.plugin_info.iteritems()
		
	def reset_plugingfx(self):
		for k,v in self.plugin_info.iteritems():
			v.reset_plugingfx()
			
	def add_plugin(self, mp):
		self.plugin_info[mp] = PluginInfo(mp)

	def update(self):
		previous = dict(self.plugin_info)
		self.plugin_info.clear()
		for mp in player.get_plugin_list():
			if mp in previous:
				self.plugin_info[mp] = previous[mp]
			else:
				self.plugin_info[mp] = PluginInfo(mp)

collection = None
player = None

def get_player():
	global player # i like this
	if not player:
		player = zzub.Player()
	return player

def get_plugin_infos():
	global collection
	if not collection:
		collection = PluginInfoCollection()
	return collection

if __name__ == '__main__':
	get_player()
	col = PluginInfoCollection()
	del col[5]
