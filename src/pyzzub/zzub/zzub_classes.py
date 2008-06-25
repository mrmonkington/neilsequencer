# generated with ctypes2classes.py, do not touch

from ctypes import cast, c_void_p
from zzub_flat import *

class _Archive(object):
	_handle = None
	_hash = 0
	
	def __init__(self):
		self._handle = zzub_archive_create_memory()
		self._hash = cast(self._handle, c_void_p).value
	def __hash__(self):
		return self._hash

	def __eq__(self,other):
		return self._hash == hash(other)

	def __ne__(self,other):
		return self._hash != hash(other)

	def destroy(self):
		assert self._handle
		zzub_archive_destroy(self._handle)
	
	def get_input(self, path):
		assert self._handle
		return Input(zzub_archive_get_input(self._handle,path))
	
	def get_output(self, path):
		assert self._handle
		return Output(zzub_archive_get_output(self._handle,path))
	
class _Attribute(object):
	_handle = None
	_hash = 0
	
	def __init__(self,handle):
		self._handle = handle
		self._hash = cast(self._handle, c_void_p).value

	def __hash__(self):
		return self._hash

	def __eq__(self,other):
		return self._hash == hash(other)

	def __ne__(self,other):
		return self._hash != hash(other)

	def get_name(self):
		assert self._handle
		return zzub_attribute_get_name(self._handle)
	
	def get_value_default(self):
		assert self._handle
		return zzub_attribute_get_value_default(self._handle)
	
	def get_value_max(self):
		assert self._handle
		return zzub_attribute_get_value_max(self._handle)
	
	def get_value_min(self):
		assert self._handle
		return zzub_attribute_get_value_min(self._handle)
	
class _Audiodriver(object):
	_handle = None
	_hash = 0
	
	def __init__(self,handle):
		self._handle = handle
		self._hash = cast(self._handle, c_void_p).value

	def __hash__(self):
		return self._hash

	def __eq__(self,other):
		return self._hash == hash(other)

	def __ne__(self,other):
		return self._hash != hash(other)

	def create_device(self, input_index, output_index):
		assert self._handle
		return zzub_audiodriver_create_device(self._handle,input_index,output_index)
	
	def destroy(self):
		assert self._handle
		zzub_audiodriver_destroy(self._handle)
	
	def destroy_device(self):
		assert self._handle
		zzub_audiodriver_destroy_device(self._handle)
	
	def enable(self, state):
		assert self._handle
		zzub_audiodriver_enable(self._handle,state)
	
	def get_buffersize(self):
		assert self._handle
		return zzub_audiodriver_get_buffersize(self._handle)
	
	def get_count(self):
		assert self._handle
		return zzub_audiodriver_get_count(self._handle)
	
	def get_cpu_load(self):
		assert self._handle
		return zzub_audiodriver_get_cpu_load(self._handle)
	
	def get_enabled(self):
		assert self._handle
		return zzub_audiodriver_get_enabled(self._handle)
	
	def get_master_channel(self):
		assert self._handle
		return zzub_audiodriver_get_master_channel(self._handle)
	
	def get_name(self, index, name, max_len):
		assert self._handle
		return zzub_audiodriver_get_name(self._handle,index,name,max_len)
	
	def get_samplerate(self):
		assert self._handle
		return zzub_audiodriver_get_samplerate(self._handle)
	
	def get_supported_input_channels(self, index):
		assert self._handle
		return zzub_audiodriver_get_supported_input_channels(self._handle,index)
	
	def get_supported_output_channels(self, index):
		assert self._handle
		return zzub_audiodriver_get_supported_output_channels(self._handle,index)
	
	def get_supported_samplerates(self, index, result, maxrates):
		assert self._handle
		return zzub_audiodriver_get_supported_samplerates(self._handle,index,result,maxrates)
	
	def is_input(self, index):
		assert self._handle
		return zzub_audiodriver_is_input(self._handle,index)
	
	def is_output(self, index):
		assert self._handle
		return zzub_audiodriver_is_output(self._handle,index)
	
	def set_buffersize(self, buffersize):
		assert self._handle
		zzub_audiodriver_set_buffersize(self._handle,buffersize)
	
	def set_master_channel(self, index):
		assert self._handle
		zzub_audiodriver_set_master_channel(self._handle,index)
	
	def set_samplerate(self, samplerate):
		assert self._handle
		zzub_audiodriver_set_samplerate(self._handle,samplerate)
	
class _Envelope(object):
	_handle = None
	_hash = 0
	
	def __init__(self,handle):
		self._handle = handle
		self._hash = cast(self._handle, c_void_p).value

	def __hash__(self):
		return self._hash

	def __eq__(self,other):
		return self._hash == hash(other)

	def __ne__(self,other):
		return self._hash != hash(other)

	def delete_point(self, index):
		assert self._handle
		zzub_envelope_delete_point(self._handle,index)
	
	def enable(self, enable):
		assert self._handle
		zzub_envelope_enable(self._handle,enable)
	
	def get_attack(self):
		assert self._handle
		return zzub_envelope_get_attack(self._handle)
	
	def get_decay(self):
		assert self._handle
		return zzub_envelope_get_decay(self._handle)
	
	def get_flags(self):
		assert self._handle
		return zzub_envelope_get_flags(self._handle)
	
	def get_point(self, index, x, y, flags):
		assert self._handle
		zzub_envelope_get_point(self._handle,index,x,y,flags)
	
	def get_point_count(self):
		assert self._handle
		return zzub_envelope_get_point_count(self._handle)
	
	def get_release(self):
		assert self._handle
		return zzub_envelope_get_release(self._handle)
	
	def get_subdivision(self):
		assert self._handle
		return zzub_envelope_get_subdivision(self._handle)
	
	def get_sustain(self):
		assert self._handle
		return zzub_envelope_get_sustain(self._handle)
	
	def insert_point(self, index):
		assert self._handle
		zzub_envelope_insert_point(self._handle,index)
	
	def is_enabled(self):
		assert self._handle
		return zzub_envelope_is_enabled(self._handle)
	
	def set_attack(self, attack):
		assert self._handle
		zzub_envelope_set_attack(self._handle,attack)
	
	def set_decay(self, decay):
		assert self._handle
		zzub_envelope_set_decay(self._handle,decay)
	
	def set_flags(self, flags):
		assert self._handle
		zzub_envelope_set_flags(self._handle,flags)
	
	def set_point(self, index, x, y, flags):
		assert self._handle
		zzub_envelope_set_point(self._handle,index,x,y,flags)
	
	def set_release(self, release):
		assert self._handle
		zzub_envelope_set_release(self._handle,release)
	
	def set_subdivision(self, subdiv):
		assert self._handle
		zzub_envelope_set_subdivision(self._handle,subdiv)
	
	def set_sustain(self, sustain):
		assert self._handle
		zzub_envelope_set_sustain(self._handle,sustain)
	
class _Input(object):
	_handle = None
	_hash = 0
	
	def __init__(self, filename):
		self._handle = zzub_input_open_file(filename)
		self._hash = cast(self._handle, c_void_p).value
	def __hash__(self):
		return self._hash

	def __eq__(self,other):
		return self._hash == hash(other)

	def __ne__(self,other):
		return self._hash != hash(other)

	def destroy(self):
		assert self._handle
		zzub_input_destroy(self._handle)
	
	def position(self):
		assert self._handle
		return zzub_input_position(self._handle)
	
	def read(self, buffer, bytes):
		assert self._handle
		zzub_input_read(self._handle,buffer,bytes)
	
	def seek(self, pos, mode):
		assert self._handle
		zzub_input_seek(self._handle,pos,mode)
	
	def size(self):
		assert self._handle
		return zzub_input_size(self._handle)
	
class _Mididriver(object):
	_handle = None
	_hash = 0
	
	def __init__(self,handle):
		self._handle = handle
		self._hash = cast(self._handle, c_void_p).value

	def __hash__(self):
		return self._hash

	def __eq__(self,other):
		return self._hash == hash(other)

	def __ne__(self,other):
		return self._hash != hash(other)

class _Midimapping(object):
	_handle = None
	_hash = 0
	
	def __init__(self,handle):
		self._handle = handle
		self._hash = cast(self._handle, c_void_p).value

	def __hash__(self):
		return self._hash

	def __eq__(self,other):
		return self._hash == hash(other)

	def __ne__(self,other):
		return self._hash != hash(other)

	def get_channel(self):
		assert self._handle
		return zzub_midimapping_get_channel(self._handle)
	
	def get_column(self):
		assert self._handle
		return zzub_midimapping_get_column(self._handle)
	
	def get_controller(self):
		assert self._handle
		return zzub_midimapping_get_controller(self._handle)
	
	def get_group(self):
		assert self._handle
		return zzub_midimapping_get_group(self._handle)
	
	def get_plugin(self):
		assert self._handle
		return zzub_midimapping_get_plugin(self._handle)
	
	def get_track(self):
		assert self._handle
		return zzub_midimapping_get_track(self._handle)
	
class _Output(object):
	_handle = None
	_hash = 0
	
	def __init__(self, filename):
		self._handle = zzub_output_create_file(filename)
		self._hash = cast(self._handle, c_void_p).value
	def __hash__(self):
		return self._hash

	def __eq__(self,other):
		return self._hash == hash(other)

	def __ne__(self,other):
		return self._hash != hash(other)

	def destroy(self):
		assert self._handle
		zzub_output_destroy(self._handle)
	
	def position(self):
		assert self._handle
		return zzub_output_position(self._handle)
	
	def seek(self, pos, mode):
		assert self._handle
		zzub_output_seek(self._handle,pos,mode)
	
	def write(self, buffer, bytes):
		assert self._handle
		zzub_output_write(self._handle,buffer,bytes)
	
class _Parameter(object):
	_handle = None
	_hash = 0
	
	def __init__(self,handle):
		self._handle = handle
		self._hash = cast(self._handle, c_void_p).value

	def __hash__(self):
		return self._hash

	def __eq__(self,other):
		return self._hash == hash(other)

	def __ne__(self,other):
		return self._hash != hash(other)

	def get_description(self):
		assert self._handle
		return zzub_parameter_get_description(self._handle)
	
	def get_flags(self):
		assert self._handle
		return zzub_parameter_get_flags(self._handle)
	
	def get_name(self):
		assert self._handle
		return zzub_parameter_get_name(self._handle)
	
	def get_type(self):
		assert self._handle
		return zzub_parameter_get_type(self._handle)
	
	def get_value_default(self):
		assert self._handle
		return zzub_parameter_get_value_default(self._handle)
	
	def get_value_max(self):
		assert self._handle
		return zzub_parameter_get_value_max(self._handle)
	
	def get_value_min(self):
		assert self._handle
		return zzub_parameter_get_value_min(self._handle)
	
	def get_value_none(self):
		assert self._handle
		return zzub_parameter_get_value_none(self._handle)
	
class _Pattern(object):
	_handle = None
	_hash = 0
	
	def __init__(self,handle):
		self._handle = handle
		self._hash = cast(self._handle, c_void_p).value

	def __hash__(self):
		return self._hash

	def __eq__(self,other):
		return self._hash == hash(other)

	def __ne__(self,other):
		return self._hash != hash(other)

	def destroy(self):
		assert self._handle
		zzub_pattern_destroy(self._handle)
	
	def get_column_count(self, group, track):
		assert self._handle
		return zzub_pattern_get_column_count(self._handle,group,track)
	
	def get_group_count(self):
		assert self._handle
		return zzub_pattern_get_group_count(self._handle)
	
	def get_name(self, name, maxLen):
		assert self._handle
		zzub_pattern_get_name(self._handle,name,maxLen)
	
	def get_row_count(self):
		assert self._handle
		return zzub_pattern_get_row_count(self._handle)
	
	def get_track_count(self, group):
		assert self._handle
		return zzub_pattern_get_track_count(self._handle,group)
	
	def get_value(self, row, group, track, column):
		assert self._handle
		return zzub_pattern_get_value(self._handle,row,group,track,column)
	
	def interpolate(self):
		assert self._handle
		zzub_pattern_interpolate(self._handle)
	
	def set_name(self, name):
		assert self._handle
		zzub_pattern_set_name(self._handle,name)
	
	def set_value(self, row, group, track, column, value):
		assert self._handle
		zzub_pattern_set_value(self._handle,row,group,track,column,value)
	
class _Player(object):
	_handle = None
	_hash = 0
	
	def __init__(self):
		self._handle = zzub_player_create()
		self._hash = cast(self._handle, c_void_p).value
	def __hash__(self):
		return self._hash

	def __eq__(self,other):
		return self._hash == hash(other)

	def __ne__(self,other):
		return self._hash != hash(other)

	def audiodriver_create(self):
		assert self._handle
		return Audiodriver(zzub_audiodriver_create(self._handle))
	
	def audiodriver_create_portaudio(self):
		assert self._handle
		return Audiodriver(zzub_audiodriver_create_portaudio(self._handle))
	
	def audiodriver_create_rtaudio(self):
		assert self._handle
		return Audiodriver(zzub_audiodriver_create_rtaudio(self._handle))
	
	def audiodriver_create_silent(self, name, out_channels, in_channels, supported_rates, num_rates):
		assert self._handle
		return Audiodriver(zzub_audiodriver_create_silent(self._handle,name,out_channels,in_channels,supported_rates,num_rates))
	
	def mididriver_close_all(self):
		assert self._handle
		return zzub_mididriver_close_all(self._handle)
	
	def mididriver_get_count(self):
		assert self._handle
		return zzub_mididriver_get_count(self._handle)
	
	def mididriver_get_name(self, index):
		assert self._handle
		return zzub_mididriver_get_name(self._handle,index)
	
	def mididriver_is_input(self, index):
		assert self._handle
		return zzub_mididriver_is_input(self._handle,index)
	
	def mididriver_is_output(self, index):
		assert self._handle
		return zzub_mididriver_is_output(self._handle,index)
	
	def mididriver_open(self, index):
		assert self._handle
		return zzub_mididriver_open(self._handle,index)
	
	def add_plugin_path(self, path):
		assert self._handle
		zzub_player_add_plugin_path(self._handle,path)
	
	def blacklist_plugin(self, uri):
		assert self._handle
		zzub_player_blacklist_plugin(self._handle,uri)
	
	def clear(self):
		assert self._handle
		zzub_player_clear(self._handle)
	
	def create_plugin(self, input, dataSize, instanceName, loader):
		assert self._handle
		return Plugin(zzub_player_create_plugin(self._handle,input._handle,dataSize,instanceName,loader._handle))
	
	def create_sequence(self, plugin):
		assert self._handle
		return Sequence(zzub_player_create_sequence(self._handle,plugin._handle))
	
	def destroy(self):
		assert self._handle
		zzub_player_destroy(self._handle)
	
	def flush(self, redo_event, undo_event):
		assert self._handle
		zzub_player_flush(self._handle,redo_event,undo_event)
	
	def get_automation(self):
		assert self._handle
		return zzub_player_get_automation(self._handle)
	
	def get_bpm(self):
		assert self._handle
		return zzub_player_get_bpm(self._handle)
	
	def get_infotext(self):
		assert self._handle
		return zzub_player_get_infotext(self._handle)
	
	def get_loop_enabled(self):
		assert self._handle
		return zzub_player_get_loop_enabled(self._handle)
	
	def get_loop_end(self):
		assert self._handle
		return zzub_player_get_loop_end(self._handle)
	
	def get_loop_start(self):
		assert self._handle
		return zzub_player_get_loop_start(self._handle)
	
	def get_midi_plugin(self):
		assert self._handle
		return Plugin(zzub_player_get_midi_plugin(self._handle))
	
	def get_midi_transport(self):
		assert self._handle
		return zzub_player_get_midi_transport(self._handle)
	
	def get_midimapping(self, index):
		assert self._handle
		return Midimapping(zzub_player_get_midimapping(self._handle,index))
	
	def get_midimapping_count(self):
		assert self._handle
		return zzub_player_get_midimapping_count(self._handle)
	
	def get_new_plugin_name(self, uri, name, maxLen):
		assert self._handle
		zzub_player_get_new_plugin_name(self._handle,uri,name,maxLen)
	
	def get_plugin(self, index):
		assert self._handle
		return Plugin(zzub_player_get_plugin(self._handle,index))
	
	def get_plugin_by_id(self, id):
		assert self._handle
		return Plugin(zzub_player_get_plugin_by_id(self._handle,id))
	
	def get_plugin_by_name(self, name):
		assert self._handle
		return Plugin(zzub_player_get_plugin_by_name(self._handle,name))
	
	def get_plugin_count(self):
		assert self._handle
		return zzub_player_get_plugin_count(self._handle)
	
	def get_pluginloader(self, index):
		assert self._handle
		return Pluginloader(zzub_player_get_pluginloader(self._handle,index))
	
	def get_pluginloader_by_name(self, name):
		assert self._handle
		return Pluginloader(zzub_player_get_pluginloader_by_name(self._handle,name))
	
	def get_pluginloader_count(self):
		assert self._handle
		return zzub_player_get_pluginloader_count(self._handle)
	
	def get_position(self):
		assert self._handle
		return zzub_player_get_position(self._handle)
	
	def get_sequence(self, index):
		assert self._handle
		return Sequence(zzub_player_get_sequence(self._handle,index))
	
	def get_sequence_track_count(self):
		assert self._handle
		return zzub_player_get_sequence_track_count(self._handle)
	
	def get_song_end(self):
		assert self._handle
		return zzub_player_get_song_end(self._handle)
	
	def get_song_start(self):
		assert self._handle
		return zzub_player_get_song_start(self._handle)
	
	def get_state(self):
		assert self._handle
		return zzub_player_get_state(self._handle)
	
	def get_tpb(self):
		assert self._handle
		return zzub_player_get_tpb(self._handle)
	
	def get_wave(self, index):
		assert self._handle
		return Wave(zzub_player_get_wave(self._handle,index))
	
	def get_wave_count(self):
		assert self._handle
		return zzub_player_get_wave_count(self._handle)
	
	def handle_events(self):
		assert self._handle
		zzub_player_handle_events(self._handle)
	
	def history_commit(self, description):
		assert self._handle
		zzub_player_history_commit(self._handle,description)
	
	def history_flush(self):
		assert self._handle
		zzub_player_history_flush(self._handle)
	
	def history_flush_last(self):
		assert self._handle
		zzub_player_history_flush_last(self._handle)
	
	def history_get_description(self, position):
		assert self._handle
		return zzub_player_history_get_description(self._handle,position)
	
	def history_get_position(self):
		assert self._handle
		return zzub_player_history_get_position(self._handle)
	
	def history_get_size(self):
		assert self._handle
		return zzub_player_history_get_size(self._handle)
	
	def initialize(self, samplesPerSecond):
		assert self._handle
		return zzub_player_initialize(self._handle,samplesPerSecond)
	
	def load_bmx(self, datastream, messages, maxLen):
		assert self._handle
		return zzub_player_load_bmx(self._handle,datastream._handle,messages,maxLen)
	
	def load_ccm(self, fileName):
		assert self._handle
		return zzub_player_load_ccm(self._handle,fileName)
	
	def redo(self):
		assert self._handle
		zzub_player_redo(self._handle)
	
	def reset_keyjazz(self):
		assert self._handle
		zzub_player_reset_keyjazz(self._handle)
	
	def save_bmx(self, plugins, num_plugins, save_waves, datastream):
		assert self._handle
		return zzub_player_save_bmx(self._handle,plugins,num_plugins,save_waves,datastream._handle)
	
	def save_ccm(self, fileName):
		assert self._handle
		return zzub_player_save_ccm(self._handle,fileName)
	
	def set_automation(self, enable):
		assert self._handle
		zzub_player_set_automation(self._handle,enable)
	
	def set_bpm(self, bpm):
		assert self._handle
		zzub_player_set_bpm(self._handle,bpm)
	
	def set_callback(self, callback, tag):
		assert self._handle
		zzub_player_set_callback(self._handle,callback,tag)
	
	def set_infotext(self, text):
		assert self._handle
		zzub_player_set_infotext(self._handle,text)
	
	def set_loop_enabled(self, enable):
		assert self._handle
		zzub_player_set_loop_enabled(self._handle,enable)
	
	def set_loop_end(self, v):
		assert self._handle
		zzub_player_set_loop_end(self._handle,v)
	
	def set_loop_start(self, v):
		assert self._handle
		zzub_player_set_loop_start(self._handle,v)
	
	def set_midi_plugin(self, plugin):
		assert self._handle
		zzub_player_set_midi_plugin(self._handle,plugin._handle)
	
	def set_midi_transport(self, enable):
		assert self._handle
		zzub_player_set_midi_transport(self._handle,enable)
	
	def set_position(self, tick):
		assert self._handle
		zzub_player_set_position(self._handle,tick)
	
	def set_song_end(self, v):
		assert self._handle
		zzub_player_set_song_end(self._handle,v)
	
	def set_song_start(self, v):
		assert self._handle
		zzub_player_set_song_start(self._handle,v)
	
	def set_state(self, state):
		assert self._handle
		zzub_player_set_state(self._handle,state)
	
	def set_tpb(self, tpb):
		assert self._handle
		zzub_player_set_tpb(self._handle,tpb)
	
	def undo(self):
		assert self._handle
		zzub_player_undo(self._handle)
	
	def work_stereo(self, numSamples):
		assert self._handle
		return zzub_player_work_stereo(self._handle,numSamples)
	
	def plugin_create_range_pattern(self, columns, rows):
		assert self._handle
		return Pattern(zzub_plugin_create_range_pattern(self._handle,columns,rows))
	
	def plugincollection_get_by_uri(self, uri):
		assert self._handle
		return Plugincollection(zzub_plugincollection_get_by_uri(self._handle,uri))
	
class _Plugin(object):
	_handle = None
	_hash = 0
	
	def __init__(self,handle):
		self._handle = handle
		self._hash = cast(self._handle, c_void_p).value

	def __hash__(self):
		return self._hash

	def __eq__(self,other):
		return self._hash == hash(other)

	def __ne__(self,other):
		return self._hash != hash(other)

	def player_add_midimapping(self, group, track, param, channel, controller):
		assert self._handle
		return Midimapping(zzub_player_add_midimapping(self._handle,group,track,param,channel,controller))
	
	def player_get_currently_playing_pattern(self, pattern, row):
		assert self._handle
		return zzub_player_get_currently_playing_pattern(self._handle,pattern,row)
	
	def player_get_currently_playing_pattern_row(self, pattern, row):
		assert self._handle
		return zzub_player_get_currently_playing_pattern_row(self._handle,pattern,row)
	
	def player_remove_midimapping(self, group, track, param):
		assert self._handle
		return zzub_player_remove_midimapping(self._handle,group,track,param)
	
	def add_event_connection_binding(self, from_plugin, sourceparam, targetgroup, targettrack, targetparam):
		assert self._handle
		zzub_plugin_add_event_connection_binding(self._handle,from_plugin._handle,sourceparam,targetgroup,targettrack,targetparam)
	
	def add_input(self, from_plugin, type):
		assert self._handle
		return zzub_plugin_add_input(self._handle,from_plugin._handle,type)
	
	def add_pattern(self, pattern):
		assert self._handle
		zzub_plugin_add_pattern(self._handle,pattern._handle)
	
	def command(self, i):
		assert self._handle
		zzub_plugin_command(self._handle,i)
	
	def create_pattern(self, rows):
		assert self._handle
		return Pattern(zzub_plugin_create_pattern(self._handle,rows))
	
	def delete_input(self, from_plugin, type):
		assert self._handle
		zzub_plugin_delete_input(self._handle,from_plugin._handle,type)
	
	def describe_value(self, group, column, value, name, maxlen):
		assert self._handle
		return zzub_plugin_describe_value(self._handle,group,column,value,name,maxlen)
	
	def destroy(self):
		assert self._handle
		return zzub_plugin_destroy(self._handle)
	
	def get_attribute_value(self, index):
		assert self._handle
		return zzub_plugin_get_attribute_value(self._handle,index)
	
	def get_bypass(self):
		assert self._handle
		return zzub_plugin_get_bypass(self._handle)
	
	def get_commands(self, commands, maxlen):
		assert self._handle
		return zzub_plugin_get_commands(self._handle,commands,maxlen)
	
	def get_envelope_count(self):
		assert self._handle
		return zzub_plugin_get_envelope_count(self._handle)
	
	def get_envelope_flags(self, index):
		assert self._handle
		return zzub_plugin_get_envelope_flags(self._handle,index)
	
	def get_envelope_name(self, index):
		assert self._handle
		return zzub_plugin_get_envelope_name(self._handle,index)
	
	def get_flags(self):
		assert self._handle
		return zzub_plugin_get_flags(self._handle)
	
	def get_id(self):
		assert self._handle
		return zzub_plugin_get_id(self._handle)
	
	def get_input_connection_by_type(self, from_plugin, type):
		assert self._handle
		return zzub_plugin_get_input_connection_by_type(self._handle,from_plugin._handle,type)
	
	def get_input_connection_count(self):
		assert self._handle
		return zzub_plugin_get_input_connection_count(self._handle)
	
	def get_input_connection_plugin(self, index):
		assert self._handle
		return Plugin(zzub_plugin_get_input_connection_plugin(self._handle,index))
	
	def get_input_connection_type(self, index):
		assert self._handle
		return zzub_plugin_get_input_connection_type(self._handle,index)
	
	def get_last_audio_result(self):
		assert self._handle
		return zzub_plugin_get_last_audio_result(self._handle)
	
	def get_last_cpu_load(self):
		assert self._handle
		return zzub_plugin_get_last_cpu_load(self._handle)
	
	def get_last_midi_result(self):
		assert self._handle
		return zzub_plugin_get_last_midi_result(self._handle)
	
	def get_last_peak(self, maxL, maxR):
		assert self._handle
		zzub_plugin_get_last_peak(self._handle,maxL,maxR)
	
	def get_last_worktime(self):
		assert self._handle
		return zzub_plugin_get_last_worktime(self._handle)
	
	def get_midi_output_device(self, index):
		assert self._handle
		return zzub_plugin_get_midi_output_device(self._handle,index)
	
	def get_midi_output_device_count(self):
		assert self._handle
		return zzub_plugin_get_midi_output_device_count(self._handle)
	
	def get_mixbuffer(self, leftbuffer, rightbuffer, size, samplepos):
		assert self._handle
		return zzub_plugin_get_mixbuffer(self._handle,leftbuffer,rightbuffer,size,samplepos)
	
	def get_mute(self):
		assert self._handle
		return zzub_plugin_get_mute(self._handle)
	
	def get_name(self, name, maxlen):
		assert self._handle
		return zzub_plugin_get_name(self._handle,name,maxlen)
	
	def get_new_pattern_name(self, name, maxLen):
		assert self._handle
		zzub_plugin_get_new_pattern_name(self._handle,name,maxLen)
	
	def get_output_connection_by_type(self, from_plugin, type):
		assert self._handle
		return zzub_plugin_get_output_connection_by_type(self._handle,from_plugin._handle,type)
	
	def get_output_connection_count(self):
		assert self._handle
		return zzub_plugin_get_output_connection_count(self._handle)
	
	def get_output_connection_plugin(self, index):
		assert self._handle
		return Plugin(zzub_plugin_get_output_connection_plugin(self._handle,index))
	
	def get_output_connection_type(self, index):
		assert self._handle
		return zzub_plugin_get_output_connection_type(self._handle,index)
	
	def get_parameter(self, group, track, column):
		assert self._handle
		return Parameter(zzub_plugin_get_parameter(self._handle,group,track,column))
	
	def get_parameter_count(self, group, track):
		assert self._handle
		return zzub_plugin_get_parameter_count(self._handle,group,track)
	
	def get_parameter_value(self, group, track, column):
		assert self._handle
		return zzub_plugin_get_parameter_value(self._handle,group,track,column)
	
	def get_pattern(self, index):
		assert self._handle
		return Pattern(zzub_plugin_get_pattern(self._handle,index))
	
	def get_pattern_by_name(self, name):
		assert self._handle
		return zzub_plugin_get_pattern_by_name(self._handle,name)
	
	def get_pattern_column_count(self):
		assert self._handle
		return zzub_plugin_get_pattern_column_count(self._handle)
	
	def get_pattern_count(self):
		assert self._handle
		return zzub_plugin_get_pattern_count(self._handle)
	
	def get_pattern_length(self, index):
		assert self._handle
		return zzub_plugin_get_pattern_length(self._handle,index)
	
	def get_pattern_name(self, index):
		assert self._handle
		return zzub_plugin_get_pattern_name(self._handle,index)
	
	def get_pattern_value(self, pattern, group, track, column, row):
		assert self._handle
		return zzub_plugin_get_pattern_value(self._handle,pattern,group,track,column,row)
	
	def get_pluginloader(self):
		assert self._handle
		return Pluginloader(zzub_plugin_get_pluginloader(self._handle))
	
	def get_position(self, x, y):
		assert self._handle
		zzub_plugin_get_position(self._handle,x,y)
	
	def get_sub_commands(self, i, commands, maxlen):
		assert self._handle
		return zzub_plugin_get_sub_commands(self._handle,i,commands,maxlen)
	
	def get_track_count(self):
		assert self._handle
		return zzub_plugin_get_track_count(self._handle)
	
	def insert_pattern_rows(self, pattern, column_indices, num_indices, start, rows):
		assert self._handle
		zzub_plugin_insert_pattern_rows(self._handle,pattern,column_indices,num_indices,start,rows)
	
	def invoke_event(self, data, immediate):
		assert self._handle
		return zzub_plugin_invoke_event(self._handle,data,immediate)
	
	def linear_to_pattern(self, index, group, track, column):
		assert self._handle
		return zzub_plugin_linear_to_pattern(self._handle,index,group,track,column)
	
	def load(self, input):
		assert self._handle
		return zzub_plugin_load(self._handle,input._handle)
	
	def move_pattern(self, index, newIndex):
		assert self._handle
		zzub_plugin_move_pattern(self._handle,index,newIndex)
	
	def pattern_to_linear(self, group, track, column, index):
		assert self._handle
		return zzub_plugin_pattern_to_linear(self._handle,group,track,column,index)
	
	def play_midi_note(self, note, prevNote, velocity):
		assert self._handle
		zzub_plugin_play_midi_note(self._handle,note,prevNote,velocity)
	
	def play_pattern_row(self, pattern, row):
		assert self._handle
		zzub_plugin_play_pattern_row(self._handle,pattern._handle,row)
	
	def play_pattern_row_ref(self, pattern, row):
		assert self._handle
		zzub_plugin_play_pattern_row_ref(self._handle,pattern,row)
	
	def remove_pattern(self, pattern):
		assert self._handle
		zzub_plugin_remove_pattern(self._handle,pattern)
	
	def remove_pattern_rows(self, pattern, column_indices, num_indices, start, rows):
		assert self._handle
		zzub_plugin_remove_pattern_rows(self._handle,pattern,column_indices,num_indices,start,rows)
	
	def save(self, ouput):
		assert self._handle
		return zzub_plugin_save(self._handle,ouput._handle)
	
	def set_attribute_value(self, index, value):
		assert self._handle
		zzub_plugin_set_attribute_value(self._handle,index,value)
	
	def set_bypass(self, muted):
		assert self._handle
		zzub_plugin_set_bypass(self._handle,muted)
	
	def set_instrument(self, name):
		assert self._handle
		return zzub_plugin_set_instrument(self._handle,name)
	
	def set_midi_connection_device(self, from_plugin, name):
		assert self._handle
		return zzub_plugin_set_midi_connection_device(self._handle,from_plugin._handle,name)
	
	def set_mute(self, muted):
		assert self._handle
		zzub_plugin_set_mute(self._handle,muted)
	
	def set_name(self, name):
		assert self._handle
		return zzub_plugin_set_name(self._handle,name)
	
	def set_parameter_value(self, group, track, column, value, record):
		assert self._handle
		zzub_plugin_set_parameter_value(self._handle,group,track,column,value,record)
	
	def set_parameter_value_direct(self, group, track, column, value, record):
		assert self._handle
		zzub_plugin_set_parameter_value_direct(self._handle,group,track,column,value,record)
	
	def set_pattern_length(self, index, rows):
		assert self._handle
		zzub_plugin_set_pattern_length(self._handle,index,rows)
	
	def set_pattern_name(self, index, name):
		assert self._handle
		zzub_plugin_set_pattern_name(self._handle,index,name)
	
	def set_pattern_value(self, pattern, group, track, column, row, value):
		assert self._handle
		zzub_plugin_set_pattern_value(self._handle,pattern,group,track,column,row,value)
	
	def set_position(self, x, y):
		assert self._handle
		zzub_plugin_set_position(self._handle,x,y)
	
	def set_position_direct(self, x, y):
		assert self._handle
		zzub_plugin_set_position_direct(self._handle,x,y)
	
	def set_stream_source(self, resource):
		assert self._handle
		zzub_plugin_set_stream_source(self._handle,resource)
	
	def set_track_count(self, count):
		assert self._handle
		zzub_plugin_set_track_count(self._handle,count)
	
	def tick(self):
		assert self._handle
		zzub_plugin_tick(self._handle)
	
	def update_pattern(self, index, pattern):
		assert self._handle
		zzub_plugin_update_pattern(self._handle,index,pattern._handle)
	
class _Plugincollection(object):
	_handle = None
	_hash = 0
	
	def __init__(self,handle):
		self._handle = handle
		self._hash = cast(self._handle, c_void_p).value

	def __hash__(self):
		return self._hash

	def __eq__(self,other):
		return self._hash == hash(other)

	def __ne__(self,other):
		return self._hash != hash(other)

	def configure(self, key, value):
		assert self._handle
		zzub_plugincollection_configure(self._handle,key,value)
	
class _Pluginloader(object):
	_handle = None
	_hash = 0
	
	def __init__(self,handle):
		self._handle = handle
		self._hash = cast(self._handle, c_void_p).value

	def __hash__(self):
		return self._hash

	def __eq__(self,other):
		return self._hash == hash(other)

	def __ne__(self,other):
		return self._hash != hash(other)

	def get_attribute(self, index):
		assert self._handle
		return Attribute(zzub_pluginloader_get_attribute(self._handle,index))
	
	def get_attribute_count(self):
		assert self._handle
		return zzub_pluginloader_get_attribute_count(self._handle)
	
	def get_author(self):
		assert self._handle
		return zzub_pluginloader_get_author(self._handle)
	
	def get_flags(self):
		assert self._handle
		return zzub_pluginloader_get_flags(self._handle)
	
	def get_instrument_list(self, result, maxbytes):
		assert self._handle
		return zzub_pluginloader_get_instrument_list(self._handle,result,maxbytes)
	
	def get_loader_name(self):
		assert self._handle
		return zzub_pluginloader_get_loader_name(self._handle)
	
	def get_name(self):
		assert self._handle
		return zzub_pluginloader_get_name(self._handle)
	
	def get_parameter(self, group, index):
		assert self._handle
		return Parameter(zzub_pluginloader_get_parameter(self._handle,group,index))
	
	def get_parameter_count(self, group):
		assert self._handle
		return zzub_pluginloader_get_parameter_count(self._handle,group)
	
	def get_short_name(self):
		assert self._handle
		return zzub_pluginloader_get_short_name(self._handle)
	
	def get_stream_format_count(self):
		assert self._handle
		return zzub_pluginloader_get_stream_format_count(self._handle)
	
	def get_stream_format_ext(self, index):
		assert self._handle
		return zzub_pluginloader_get_stream_format_ext(self._handle,index)
	
	def get_tracks_max(self):
		assert self._handle
		return zzub_pluginloader_get_tracks_max(self._handle)
	
	def get_tracks_min(self):
		assert self._handle
		return zzub_pluginloader_get_tracks_min(self._handle)
	
	def get_uri(self):
		assert self._handle
		return zzub_pluginloader_get_uri(self._handle)
	
class _Recorder(object):
	_handle = None
	_hash = 0
	
	def __init__(self,handle):
		self._handle = handle
		self._hash = cast(self._handle, c_void_p).value

	def __hash__(self):
		return self._hash

	def __eq__(self,other):
		return self._hash == hash(other)

	def __ne__(self,other):
		return self._hash != hash(other)

class _Sequence(object):
	_handle = None
	_hash = 0
	
	def __init__(self,handle):
		self._handle = handle
		self._hash = cast(self._handle, c_void_p).value

	def __hash__(self):
		return self._hash

	def __eq__(self,other):
		return self._hash == hash(other)

	def __ne__(self,other):
		return self._hash != hash(other)

	def destroy(self):
		assert self._handle
		zzub_sequence_destroy(self._handle)
	
	def get_event(self, index, pos, value):
		assert self._handle
		return zzub_sequence_get_event(self._handle,index,pos,value)
	
	def get_event_at(self, pos):
		assert self._handle
		return zzub_sequence_get_event_at(self._handle,pos)
	
	def get_event_count(self):
		assert self._handle
		return zzub_sequence_get_event_count(self._handle)
	
	def get_plugin(self):
		assert self._handle
		return Plugin(zzub_sequence_get_plugin(self._handle))
	
	def insert_events(self, start, ticks):
		assert self._handle
		return zzub_sequence_insert_events(self._handle,start,ticks)
	
	def move(self, newIndex):
		assert self._handle
		zzub_sequence_move(self._handle,newIndex)
	
	def remove_events(self, start, ticks):
		assert self._handle
		return zzub_sequence_remove_events(self._handle,start,ticks)
	
	def set_event(self, pos, value):
		assert self._handle
		zzub_sequence_set_event(self._handle,pos,value)
	
class _Wave(object):
	_handle = None
	_hash = 0
	
	def __init__(self,handle):
		self._handle = handle
		self._hash = cast(self._handle, c_void_p).value

	def __hash__(self):
		return self._hash

	def __eq__(self,other):
		return self._hash == hash(other)

	def __ne__(self,other):
		return self._hash != hash(other)

	def clear(self):
		assert self._handle
		return zzub_wave_clear(self._handle)
	
	def get_envelope(self, index):
		assert self._handle
		return Envelope(zzub_wave_get_envelope(self._handle,index))
	
	def get_envelope_count(self):
		assert self._handle
		return zzub_wave_get_envelope_count(self._handle)
	
	def get_flags(self):
		assert self._handle
		return zzub_wave_get_flags(self._handle)
	
	def get_index(self):
		assert self._handle
		return zzub_wave_get_index(self._handle)
	
	def get_level(self, index):
		assert self._handle
		return Wavelevel(zzub_wave_get_level(self._handle,index))
	
	def get_level_count(self):
		assert self._handle
		return zzub_wave_get_level_count(self._handle)
	
	def get_name(self):
		assert self._handle
		return zzub_wave_get_name(self._handle)
	
	def get_path(self):
		assert self._handle
		return zzub_wave_get_path(self._handle)
	
	def get_volume(self):
		assert self._handle
		return zzub_wave_get_volume(self._handle)
	
	def load_sample(self, level, offset, clear, path, datastream):
		assert self._handle
		return zzub_wave_load_sample(self._handle,level,offset,clear,path,datastream._handle)
	
	def save_sample(self, level, datastream):
		assert self._handle
		return zzub_wave_save_sample(self._handle,level,datastream._handle)
	
	def save_sample_range(self, level, datastream, start, end):
		assert self._handle
		return zzub_wave_save_sample_range(self._handle,level,datastream._handle,start,end)
	
	def set_envelope(self, index, env):
		assert self._handle
		zzub_wave_set_envelope(self._handle,index,env._handle)
	
	def set_envelope_count(self, count):
		assert self._handle
		zzub_wave_set_envelope_count(self._handle,count)
	
	def set_flags(self, flags):
		assert self._handle
		zzub_wave_set_flags(self._handle,flags)
	
	def set_name(self, name):
		assert self._handle
		zzub_wave_set_name(self._handle,name)
	
	def set_path(self, path):
		assert self._handle
		zzub_wave_set_path(self._handle,path)
	
	def set_volume(self, volume):
		assert self._handle
		zzub_wave_set_volume(self._handle,volume)
	
class _Wavelevel(object):
	_handle = None
	_hash = 0
	
	def __init__(self,handle):
		self._handle = handle
		self._hash = cast(self._handle, c_void_p).value

	def __hash__(self):
		return self._hash

	def __eq__(self,other):
		return self._hash == hash(other)

	def __ne__(self,other):
		return self._hash != hash(other)

	def clear(self):
		assert self._handle
		return zzub_wavelevel_clear(self._handle)
	
	def get_format(self):
		assert self._handle
		return zzub_wavelevel_get_format(self._handle)
	
	def get_loop_end(self):
		assert self._handle
		return zzub_wavelevel_get_loop_end(self._handle)
	
	def get_loop_start(self):
		assert self._handle
		return zzub_wavelevel_get_loop_start(self._handle)
	
	def get_root_note(self):
		assert self._handle
		return zzub_wavelevel_get_root_note(self._handle)
	
	def get_sample_count(self):
		assert self._handle
		return zzub_wavelevel_get_sample_count(self._handle)
	
	def get_samples_digest(self, channel, start, end, mindigest, maxdigest, ampdigest, digestsize):
		assert self._handle
		zzub_wavelevel_get_samples_digest(self._handle,channel,start,end,mindigest,maxdigest,ampdigest,digestsize)
	
	def get_samples_per_second(self):
		assert self._handle
		return zzub_wavelevel_get_samples_per_second(self._handle)
	
	def get_wave(self):
		assert self._handle
		return Wave(zzub_wavelevel_get_wave(self._handle))
	
	def remove_sample_range(self, start, end):
		assert self._handle
		zzub_wavelevel_remove_sample_range(self._handle,start,end)
	
	def set_loop_end(self, pos):
		assert self._handle
		zzub_wavelevel_set_loop_end(self._handle,pos)
	
	def set_loop_start(self, pos):
		assert self._handle
		zzub_wavelevel_set_loop_start(self._handle,pos)
	
	def set_root_note(self, note):
		assert self._handle
		zzub_wavelevel_set_root_note(self._handle,note)
	
	def set_sample_count(self, count):
		assert self._handle
		zzub_wavelevel_set_sample_count(self._handle,count)
	
	def set_samples_per_second(self, sps):
		assert self._handle
		zzub_wavelevel_set_samples_per_second(self._handle,sps)
	

# you can override these
Input = _Input
Recorder = _Recorder
Midimapping = _Midimapping
Envelope = _Envelope
Plugin = _Plugin
Wave = _Wave
Mididriver = _Mididriver
Sequence = _Sequence
Plugincollection = _Plugincollection
Player = _Player
Wavelevel = _Wavelevel
Attribute = _Attribute
Archive = _Archive
Parameter = _Parameter
Audiodriver = _Audiodriver
Pluginloader = _Pluginloader
Pattern = _Pattern
Output = _Output
