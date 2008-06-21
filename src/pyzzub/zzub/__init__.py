#encoding: latin-1

# pyzzub
# Python bindings for libzzub
# Copyright (C) 2006 The libzzub Development Team
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

import os, sys
from zzub_flat import *
import zzub_classes as libzzub
import array
import ctypes

class Audiodriver(libzzub._Audiodriver):
	def get_name(self, index):
		name = (ctypes.c_char * 1024)()
		libzzub._Audiodriver.get_name(self, index, name, 1024)
		return name.value

libzzub.Audiodriver = Audiodriver

class Pluginloader(libzzub._Pluginloader):
	def get_parameter_list(self, group):
		return [self.get_parameter(group,index) for index in range(self.get_parameter_count(group))]

	def get_attribute_list(self):
		return [self.get_attribute(index) for index in range(self.get_attribute_count())]

libzzub.Pluginloader = Pluginloader

class Connection:
	def __init__(self, in_plugin, out_plugin, index):
		self.in_plugin = in_plugin
		self.out_plugin = out_plugin
		if self.in_plugin:
			self.type = self.in_plugin.get_input_connection_type(index)
			self.out_plugin = self.in_plugin.get_input_connection_plugin(index)
		elif self.out_plugin:
			self.type = self.out_plugin.get_output_connection_type(index)
			self.in_plugin = self.out_plugin.get_output_connection_plugin(index)
		
	def get_type(self):
		return self.type
	
	def get_input(self):
		return self.out_plugin
	
	def get_output(self):
		return self.in_plugin

class Pattern(libzzub._Pattern):
	def get_name(self):
		name = (ctypes.c_char * 1024)()
		libzzub._Pattern.get_name(self, name, 1024)
		return name.value
		
	def get_new_name(self):
		name = (ctypes.c_char * 1024)()
		libzzub._Pattern.get_new_name(self, name, 1024)
		return name.value
		
	def get_bandwidth_digest(self, size):
		digest = (ctypes.c_float * size)()
		libzzub._Pattern.get_bandwidth_digest(self, digest, size)
		return digest

libzzub.Pattern = Pattern

class Plugin(libzzub._Plugin):
	def create_pattern(self, row):
		return Pattern(libzzub.zzub_plugin_create_pattern(self._handle,row))
	
	def add_post_process(self, mixcallback, tag):	
		_cb = libzzub.ZzubMixCallback(mixcallback)
		pp = libzzub._Plugin.add_post_process(self, _cb, None)
		pp._cb = _cb
		return pp

	def get_name(self):
		name = (ctypes.c_char * 1024)()
		libzzub._Plugin.get_name(self, name, 1024)
		return name.value
	
	def describe_value(self, group, column, value):
		name = (ctypes.c_char * 1024)()
		libzzub._Plugin.describe_value(self,group,column,value,name,1024)
		return name.value

	def get_commands(self):
		commands = (ctypes.c_char * 32768)()
		libzzub._Plugin.get_commands(self,commands,32768)
		if commands.value:
			return commands.value.split('\n')
		return []

	def get_sub_commands(self, i):
		commands = (ctypes.c_char * 32768)()
		libzzub._Plugin.get_sub_commands(self,i,commands,32768)
		if commands.value:
			return commands.value.split('\n')
		return []

	def get_last_peak(self):
		maxL, maxR = ctypes.c_float(), ctypes.c_float()
		libzzub._Plugin.get_last_peak(self,ctypes.byref(maxL),ctypes.byref(maxR))
		return maxL.value, maxR.value
	
	def get_pluginloader(self):
		return Pluginloader(libzzub.zzub_plugin_get_pluginloader(self._handle))
	
	def get_pattern(self, index):
		return Pattern(libzzub.zzub_plugin_get_pattern(self._handle,index))
		
	def get_pattern_list(self):
		return [self.get_pattern(index) for index in range(self.get_pattern_count())]

	def get_position(self):
		x = ctypes.c_float()
		y = ctypes.c_float()
		libzzub._Plugin.get_position(self,ctypes.byref(x),ctypes.byref(y))
		return x.value,y.value
		
	def get_input_connection_list(self):
		return [self.get_input_connection(index) for index in range(self.get_input_connection_count())]

	def get_output_connection_list(self):
		return [self.get_output_connection(index) for index in range(self.get_output_connection_count())]

	def get_input_connection(self, index):
		return Connection(self, None, index)
	
	def get_output_connection(self, index):
		return Connection(None, self, index)
		
	def get_group_track_count(self, group):
		return [self.get_input_connection_count, lambda: 1, self.get_track_count][group]()

	def invoke_event(self, data, immediate):
		return libzzub._Plugin.invoke_event(self,ctypes.byref(data),immediate)

libzzub.Plugin = Plugin

class Sequence(libzzub._Sequence):
	def get_plugin(self):
		return Plugin(libzzub.zzub_sequence_get_plugin(self._handle))
		
	def get_event(self, index):
		pos = ctypes.c_long()
		value = ctypes.c_long()
		libzzub._Sequence.get_event(self,index,ctypes.byref(pos),ctypes.byref(value))
		return pos.value, value.value
		
	def get_event_list(self):
		return [self.get_event(index) for index in range(self.get_event_count())]

libzzub.Sequence = Sequence

#~ class Sequencer(libzzub._Sequencer):
	#~ def get_track_list(self):
		#~ return [self.get_track(index) for index in range(self.get_track_count())]

	#~ def get_track(self, index):
		#~ return Sequence(libzzub.zzub_sequencer_get_track(self._handle,index))

	#~ def create_track(self, machine):
		#~ return Sequence(libzzub.zzub_sequencer_create_track(self._handle,machine._handle))

#~ libzzub.Sequencer = Sequencer

class Player(libzzub._Player):
	callback = None
	
	def get_current_sequencer(self):
		return self #Sequencer(libzzub.zzub_player_get_current_sequencer(self._handle))
		
	def create_plugin(self, input, dataSize, instanceName, loader):
		#~ if not input:
			#~ input = libzzub._Input(ctypes.POINTER(libzzub.zzub_input_t)())
		return Plugin(libzzub.zzub_player_create_plugin(self._handle,input and input._handle,dataSize,instanceName,loader._handle))
		
	def get_song_loop(self):
		return self.get_loop_start(), self.get_loop_end()
		
	def play(self):
		self.set_state(libzzub.zzub_player_state_playing)
		
	def stop(self):
		self.set_state(libzzub.zzub_player_state_stopped)

	def get_plugin(self, index):
		return Plugin(libzzub.zzub_player_get_plugin(self._handle,index))

	def get_plugin_list(self):
		return [self.get_plugin(index) for index in range(self.get_plugin_count())]
		
	def get_midimapping_list(self):
		return [self.get_midimapping(index) for index in range(self.get_midimapping_count())]

	def get_pluginloader_list(self):
		return [self.get_pluginloader(index) for index in range(self.get_pluginloader_count())]

	def get_pluginloader(self, index):
		return Pluginloader(libzzub.zzub_player_get_pluginloader(self._handle,index))

	def get_wave(self, index):
		return Wave(libzzub.zzub_player_get_wave(self._handle,index))
		
	def pre_callback(self, player, plugin, data, tag):
		if self.callback:
			player = self
			plugin = Plugin(plugin)
			if self.callback(player, plugin, data.contents):
				return 0
		return -1
		
	def set_callback(self, cb):
		self.callback = cb
		_cb = libzzub.ZzubCallback(self.pre_callback)
		libzzub._Player.set_callback(self, _cb, None)
		self._callback = _cb

libzzub.Player = Player

class Wave(libzzub._Wave):
	def get_envelope(self, index):
		return Envelope(libzzub.zzub_wave_get_envelope(self._handle,index))
		
	def get_envelope_list(self):
		return [self.get_envelope(index) for index in range(self.get_envelope_count())]

libzzub.Wave = Wave

class Wavelevel(libzzub._Wavelevel):
	def get_samples_digest(self, channel, start, end, digestsize):
		mindigest = (ctypes.c_float * digestsize)()
		maxdigest = (ctypes.c_float * digestsize)()
		ampdigest = (ctypes.c_float * digestsize)()
		libzzub._Wavelevel.get_samples_digest(self,channel,start,end,mindigest,maxdigest,ampdigest,digestsize)
		return mindigest,maxdigest,ampdigest
		
	def get_slices(self):
		return [self.get_slice_value(index) for index in range(self.get_slice_count())]

libzzub.Wavelevel = Wavelevel

class Envelope(libzzub._Envelope):
	def get_point(self, index):
		x = ctypes.c_ushort(0)
		y = ctypes.c_ushort(0)
		flags = ctypes.c_ubyte(0)
		libzzub._Envelope.get_point(self,index,ctypes.byref(x),ctypes.byref(y),ctypes.byref(flags))
		return x.value, y.value, flags.value
		
	def get_point_list(self):
		return [self.get_point(index) for index in range(self.get_point_count())]

libzzub.Envelope = Envelope


__all__ = [
	'Audiodriver',
	'Pluginloader',
	#~ 'Connection',
	'Pattern',
	'Plugin',
	'Sequence',
	#~ 'Sequencer',
	'Player',
	'Wave',
	'Envelope',
]



