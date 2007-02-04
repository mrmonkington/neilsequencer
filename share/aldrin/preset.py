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
Provides a class to read and write preset files.
"""

#~ ------------
#~ head
#~ ------------

#~ 4 bytes: version number
#~ 4 bytes: size of machine name
#~ machine name characters
#~ 4 bytes: number of parameter sets

#~ --------------
#~ single preset block:
#~ --------------

#~ 4 byte: number of characters in name
#~ name characters (no terminating zero)
#~ 4 byte: number of tracks
#~ 4 byte: number of parameters (global & track*tracks, only the mpf_state parameters)
#~ then parameters as 32 bit int
#~ 4 byte: size of comment string 
#~ comment characters

import zzub
from utils import read_int, read_string, write_int, write_string

def sort_preset(a,b):
	if a.name.lower() < b.name.lower():
		return -1
	elif a.name.lower() > b.name.lower():
		return 1
	return 0

class Preset:
	"""
	A class to hold preset data.
	"""
	def __init__(self, f=None):
		"""
		Loads the preset from a file.
		
		@param f: handle to open preset file.
		@type f: file
		"""
		self.name = ''
		self.trackcount = 0
		self.params = []
		self.comment = ''
		if f:
			self.name = read_string(f)
			self.trackcount = read_int(f)
			paramcount = read_int(f)
			for i in range(paramcount):
				self.params.append(read_int(f))
			self.comment = read_string(f)
			
	def save(self, filehandle):
		"""
		Writes the preset to an open filehandle.
		
		@param filehandle: Handle to an open preset file.
		@type filehandle: file
		"""
		write_string(filehandle, self.name)
		write_int(filehandle, self.trackcount)
		write_int(filehandle, len(self.params))
		for param in self.params:
			write_int(filehandle, param)
		write_string(filehandle, self.comment)
			
	def apply(self, plugin):
		"""
		Apply the preset to a currently loaded plugin.
		
		@param plugin: The plugin to which to apply the preset.
		@type plugin: zzub.Plugin
		"""
		pl = plugin.get_pluginloader()
		params = list(reversed(self.params[:]))
		for g in range(1,3):
			if g == 1:
				trackcount = 1
			else:
				trackcount = self.trackcount
			for t in range(trackcount):
					for i in range(pl.get_parameter_count(g)):
						p = pl.get_parameter(g,i)
						if p.get_flags() & zzub.zzub_parameter_flag_state:
							v = params.pop()
							assert v >= p.get_value_min() and v <= p.get_value_max()
							if t < plugin.get_group_track_count(g):
								plugin.set_parameter_value(g,t,i,v,0)
								
	def pickup(self, plugin):
		"""
		Pickup the preset from a currently loaded plugin.
		
		@param plugin: The plugin from which to pick up parameters.
		@type plugin: zzub.Plugin
		"""
		pl = plugin.get_pluginloader()
		self.params = []
		self.trackcount = plugin.get_group_track_count(2)
		for g in range(1,3):
			if g == 1:
				trackcount = 1
			else:
				trackcount = self.trackcount
			for t in range(trackcount):
					for i in range(pl.get_parameter_count(g)):
						p = pl.get_parameter(g,i)
						if p.get_flags() & zzub.zzub_parameter_flag_state:
							self.params.append(plugin.get_parameter_value(g,t,i))

class PresetCollection:
	"""
	A collection of plugin parameter presets.
	"""
	def __init__(self, filepath=None):
		"""
		Loads the preset collection from a file.
		
		@param filepath: Path to preset file.
		@type filepath: str
		"""
		self.filepath = filepath
		self.presets = []
		self.version = 1
		self.name = ''
		if filepath:
			f = file(filepath,'rb')
			self.version = read_int(f)
			self.name = read_string(f)
			setcount = read_int(f)
			for index in range(setcount):
				self.presets.append(Preset(f))
			self.sort()
			
	def save(self, filepath):
		"""
		Saves the preset collection to a file.
		
		@param filepath: Path to file.
		@type filepath: str
		"""
		f = file(filepath, 'wb')
		write_int(f, self.version)
		write_string(f, self.name)
		write_int(f, len(self.presets))
		for preset in self.presets:
			preset.save(f)
		f.close()
			
	def sort(self):
		"""
		Sorts presets by filenames.
		"""
		self.presets = sorted(self.presets,sort_preset)

__all__ = [
	'PresetCollection',
	'Preset',
]

if __name__ == '__main__':
	from config import get_plugin_aliases, get_plugin_blacklist
	import utils, zzub, os
	aliases = {}
	player = zzub.Player()
	# load blacklist file and add blacklist entries
	for name in get_plugin_blacklist():
		player.blacklist_plugin(name)
	# load aliases file and add aliases
	for name,uri in get_plugin_aliases():
		aliases[name]=uri
		player.add_plugin_alias(name, uri)
	pluginpath = utils.filepath('../../lib/zzub') + os.sep
	print "pluginpath is '%s'" % pluginpath
	player.add_plugin_path(pluginpath)
	player.initialize(44100)
	prs = PresetCollection(utils.filepath('presets/makk_m4.prs'))
	print 'collection version',prs.version
	print 'collection name',prs.name	
	pl = player.get_pluginloader_by_name(aliases.get(prs.name,prs.name))
	if pl._handle:
		for preset in prs.presets:
			print '---'
			print 'preset name', preset.name
