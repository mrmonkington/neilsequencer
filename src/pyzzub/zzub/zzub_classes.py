# generated with ctypes2classes.py, do not touch

from ctypes import cast, c_void_p
from zzub_flat import *

class Archive(object):
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

class Attribute(object):
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
	
class AudioConnection(object):
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

	def get_amplitude(self):
		assert self._handle
		return zzub_audio_connection_get_amplitude(self._handle)
	
	def get_panning(self):
		assert self._handle
		return zzub_audio_connection_get_panning(self._handle)
	
	def set_amplitude(self, amp):
		assert self._handle
		zzub_audio_connection_set_amplitude(self._handle,amp)
	
	def set_panning(self, pan):
		assert self._handle
		zzub_audio_connection_set_panning(self._handle,pan)
	
class Connection(object):
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

	def get_audio_connection(self):
		assert self._handle
		return AudioConnection(zzub_connection_get_audio_connection(self._handle))
	
	def get_event_connection(self):
		assert self._handle
		return EventConnection(zzub_connection_get_event_connection(self._handle))
	
	def get_input(self):
		assert self._handle
		return Plugin(zzub_connection_get_input(self._handle))
	
	def get_output(self):
		assert self._handle
		return Plugin(zzub_connection_get_output(self._handle))
	
	def get_type(self):
		assert self._handle
		return zzub_connection_get_type(self._handle)
	
class Envelope(object):
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
	
class EventConnection(object):
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

	def add_binding(self, sourceparam, targetgroup, targettrack, targetparam):
		assert self._handle
		return zzub_event_connection_add_binding(self._handle,sourceparam,targetgroup,targettrack,targetparam)
	
	def get_binding(self, index):
		assert self._handle
		return zzub_event_connection_get_binding(self._handle,index)
	
	def get_binding_count(self):
		assert self._handle
		return zzub_event_connection_get_binding_count(self._handle)
	
	def remove_binding(self, index):
		assert self._handle
		return zzub_event_connection_remove_binding(self._handle,index)
	
class Input(object):
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

class Midimapping(object):
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
		return Plugin(zzub_midimapping_get_plugin(self._handle))
	
	def get_track(self):
		assert self._handle
		return zzub_midimapping_get_track(self._handle)
	
class Output(object):
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

class Parameter(object):
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
	
class Pattern(object):
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

	def delete_row(self, group, track, column, row):
		assert self._handle
		zzub_pattern_delete_row(self._handle,group,track,column,row)
	
	def destroy(self):
		assert self._handle
		zzub_pattern_destroy(self._handle)
	
	def get_name(self, name, maxLen):
		assert self._handle
		zzub_pattern_get_name(self._handle,name,maxLen)
	
	def get_row_count(self):
		assert self._handle
		return zzub_pattern_get_row_count(self._handle)
	
	def get_track(self, index):
		assert self._handle
		return Patterntrack(zzub_pattern_get_track(self._handle,index))
	
	def get_track_count(self):
		assert self._handle
		return zzub_pattern_get_track_count(self._handle)
	
	def get_value(self, row, group, track, column):
		assert self._handle
		return zzub_pattern_get_value(self._handle,row,group,track,column)
	
	def insert_row(self, group, track, column, row):
		assert self._handle
		zzub_pattern_insert_row(self._handle,group,track,column,row)
	
	def set_name(self, name):
		assert self._handle
		zzub_pattern_set_name(self._handle,name)
	
	def set_row_count(self, rows):
		assert self._handle
		zzub_pattern_set_row_count(self._handle,rows)
	
	def set_value(self, row, group, track, column, value):
		assert self._handle
		zzub_pattern_set_value(self._handle,row,group,track,column,value)
	
class Patterntrack(object):
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

class Player(object):
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

	def audiodriver_create(self, input_index, output_index):
		assert self._handle
		return zzub_audiodriver_create(self._handle,input_index,output_index)
	
	def audiodriver_destroy(self):
		assert self._handle
		zzub_audiodriver_destroy(self._handle)
	
	def audiodriver_enable(self, state):
		assert self._handle
		zzub_audiodriver_enable(self._handle,state)
	
	def audiodriver_get_buffersize(self):
		assert self._handle
		return zzub_audiodriver_get_buffersize(self._handle)
	
	def audiodriver_get_count(self):
		assert self._handle
		return zzub_audiodriver_get_count(self._handle)
	
	def audiodriver_get_cpu_load(self):
		assert self._handle
		return zzub_audiodriver_get_cpu_load(self._handle)
	
	def audiodriver_get_name(self, index, name, maxLen):
		assert self._handle
		return zzub_audiodriver_get_name(self._handle,index,name,maxLen)
	
	def audiodriver_get_samplerate(self):
		assert self._handle
		return zzub_audiodriver_get_samplerate(self._handle)
	
	def audiodriver_is_input(self, index):
		assert self._handle
		return zzub_audiodriver_is_input(self._handle,index)
	
	def audiodriver_is_output(self, index):
		assert self._handle
		return zzub_audiodriver_is_output(self._handle,index)
	
	def audiodriver_set_buffersize(self, buffersize):
		assert self._handle
		zzub_audiodriver_set_buffersize(self._handle,buffersize)
	
	def audiodriver_set_samplerate(self, samplerate):
		assert self._handle
		zzub_audiodriver_set_samplerate(self._handle,samplerate)
	
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
	
	def add_midimapping(self, plugin, group, track, param, channel, controller):
		assert self._handle
		return Midimapping(zzub_player_add_midimapping(self._handle,plugin._handle,group,track,param,channel,controller))
	
	def add_plugin_alias(self, name, uri):
		assert self._handle
		zzub_player_add_plugin_alias(self._handle,name,uri)
	
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
	
	def destroy(self):
		assert self._handle
		zzub_player_destroy(self._handle)
	
	def get_automation(self):
		assert self._handle
		return zzub_player_get_automation(self._handle)
	
	def get_bpm(self):
		assert self._handle
		return zzub_player_get_bpm(self._handle)
	
	def get_current_sequencer(self):
		assert self._handle
		return Sequencer(zzub_player_get_current_sequencer(self._handle))
	
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
	
	def get_midimapping(self, index):
		assert self._handle
		return Midimapping(zzub_player_get_midimapping(self._handle,index))
	
	def get_midimapping_count(self):
		assert self._handle
		return zzub_player_get_midimapping_count(self._handle)
	
	def get_plugin(self, index):
		assert self._handle
		return Plugin(zzub_player_get_plugin(self._handle,index))
	
	def get_plugin_by_name(self, name):
		assert self._handle
		return Plugin(zzub_player_get_plugin_by_name(self._handle,name))
	
	def get_plugin_count(self):
		assert self._handle
		return zzub_player_get_plugin_count(self._handle)
	
	def get_plugincollection_by_uri(self, uri):
		assert self._handle
		return Plugincollection(zzub_player_get_plugincollection_by_uri(self._handle,uri))
	
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
	
	def get_sequence_count(self):
		assert self._handle
		return zzub_player_get_sequence_count(self._handle)
	
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
	
	def get_wave_amp(self):
		assert self._handle
		return zzub_player_get_wave_amp(self._handle)
	
	def get_wave_count(self):
		assert self._handle
		return zzub_player_get_wave_count(self._handle)
	
	def handle_events(self):
		assert self._handle
		zzub_player_handle_events(self._handle)
	
	def initialize(self, samplesPerSecond):
		assert self._handle
		return zzub_player_initialize(self._handle,samplesPerSecond)
	
	def load_bmx(self, fileName):
		assert self._handle
		return zzub_player_load_bmx(self._handle,fileName)
	
	def load_ccm(self, fileName):
		assert self._handle
		return zzub_player_load_ccm(self._handle,fileName)
	
	def lock(self):
		assert self._handle
		zzub_player_lock(self._handle)
	
	def lock_tick(self):
		assert self._handle
		zzub_player_lock_tick(self._handle)
	
	def play_wave(self, wave, level, note):
		assert self._handle
		zzub_player_play_wave(self._handle,wave._handle,level,note)
	
	def remove_midimapping(self, plugin, group, track, param):
		assert self._handle
		return zzub_player_remove_midimapping(self._handle,plugin._handle,group,track,param)
	
	def save_bmx(self, fileName):
		assert self._handle
		return zzub_player_save_bmx(self._handle,fileName)
	
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
	
	def set_current_sequencer(self, sequencer):
		assert self._handle
		zzub_player_set_current_sequencer(self._handle,sequencer._handle)
	
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
	
	def set_wave_amp(self, amp):
		assert self._handle
		zzub_player_set_wave_amp(self._handle,amp)
	
	def stop_wave(self):
		assert self._handle
		zzub_player_stop_wave(self._handle)
	
	def unlock(self):
		assert self._handle
		zzub_player_unlock(self._handle)
	
	def unlock_tick(self):
		assert self._handle
		zzub_player_unlock_tick(self._handle)
	
	def work_stereo(self, numSamples):
		assert self._handle
		return zzub_player_work_stereo(self._handle,numSamples)
	
class Plugin(object):
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

	def add_audio_input(self, fromMachine, amp, pan):
		assert self._handle
		return Connection(zzub_plugin_add_audio_input(self._handle,fromMachine._handle,amp,pan))
	
	def add_event_input(self, fromMachine):
		assert self._handle
		return Connection(zzub_plugin_add_event_input(self._handle,fromMachine._handle))
	
	def add_pattern(self, pattern):
		assert self._handle
		zzub_plugin_add_pattern(self._handle,pattern._handle)
	
	def add_post_process(self, mixcallback, tag):
		assert self._handle
		return Postprocess(zzub_plugin_add_post_process(self._handle,mixcallback,tag))
	
	def command(self, i):
		assert self._handle
		zzub_plugin_command(self._handle,i)
	
	def create_pattern(self, row):
		assert self._handle
		return Pattern(zzub_plugin_create_pattern(self._handle,row))
	
	def delete_input(self, fromMachine):
		assert self._handle
		zzub_plugin_delete_input(self._handle,fromMachine._handle)
	
	def describe_value(self, group, column, value, name, maxlen):
		assert self._handle
		return zzub_plugin_describe_value(self._handle,group,column,value,name,maxlen)
	
	def destroy(self):
		assert self._handle
		return zzub_plugin_destroy(self._handle)
	
	def get_attribute_value(self, index):
		assert self._handle
		return zzub_plugin_get_attribute_value(self._handle,index)
	
	def get_auto_write(self):
		assert self._handle
		return zzub_plugin_get_auto_write(self._handle)
	
	def get_commands(self, commands, maxlen):
		assert self._handle
		return zzub_plugin_get_commands(self._handle,commands,maxlen)
	
	def get_end_write_position(self):
		assert self._handle
		return zzub_plugin_get_end_write_position(self._handle)
	
	def get_input_connection(self, index):
		assert self._handle
		return Connection(zzub_plugin_get_input_connection(self._handle,index))
	
	def get_input_connection_count(self):
		assert self._handle
		return zzub_plugin_get_input_connection_count(self._handle)
	
	def get_last_peak(self, maxL, maxR):
		assert self._handle
		zzub_plugin_get_last_peak(self._handle,maxL,maxR)
	
	def get_last_worktime(self):
		assert self._handle
		return zzub_plugin_get_last_worktime(self._handle)
	
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
	
	def get_output_channels(self):
		assert self._handle
		return zzub_plugin_get_output_channels(self._handle)
	
	def get_output_connection(self, index):
		assert self._handle
		return Connection(zzub_plugin_get_output_connection(self._handle,index))
	
	def get_output_connection_count(self):
		assert self._handle
		return zzub_plugin_get_output_connection_count(self._handle)
	
	def get_parameter_value(self, group, track, column):
		assert self._handle
		return zzub_plugin_get_parameter_value(self._handle,group,track,column)
	
	def get_pattern(self, index):
		assert self._handle
		return Pattern(zzub_plugin_get_pattern(self._handle,index))
	
	def get_pattern_by_name(self, name):
		assert self._handle
		return Pattern(zzub_plugin_get_pattern_by_name(self._handle,name))
	
	def get_pattern_count(self):
		assert self._handle
		return zzub_plugin_get_pattern_count(self._handle)
	
	def get_pattern_index(self, pattern):
		assert self._handle
		return zzub_plugin_get_pattern_index(self._handle,pattern._handle)
	
	def get_pluginloader(self):
		assert self._handle
		return Pluginloader(zzub_plugin_get_pluginloader(self._handle))
	
	def get_position(self, x, y):
		assert self._handle
		zzub_plugin_get_position(self._handle,x,y)
	
	def get_start_write_position(self):
		assert self._handle
		return zzub_plugin_get_start_write_position(self._handle)
	
	def get_sub_commands(self, i, commands, maxlen):
		assert self._handle
		return zzub_plugin_get_sub_commands(self._handle,i,commands,maxlen)
	
	def get_ticks_written(self):
		assert self._handle
		return zzub_plugin_get_ticks_written(self._handle)
	
	def get_track_count(self):
		assert self._handle
		return zzub_plugin_get_track_count(self._handle)
	
	def get_type(self):
		assert self._handle
		return zzub_plugin_get_type(self._handle)
	
	def get_wave_file_path(self):
		assert self._handle
		return zzub_plugin_get_wave_file_path(self._handle)
	
	def get_write_wave(self):
		assert self._handle
		return zzub_plugin_get_write_wave(self._handle)
	
	def invoke_event(self, data, immediate):
		assert self._handle
		return zzub_plugin_invoke_event(self._handle,data,immediate)
	
	def move_pattern(self, index, newIndex):
		assert self._handle
		zzub_plugin_move_pattern(self._handle,index,newIndex)
	
	def remove_pattern(self, pattern):
		assert self._handle
		zzub_plugin_remove_pattern(self._handle,pattern._handle)
	
	def remove_post_process(self, pp):
		assert self._handle
		zzub_plugin_remove_post_process(self._handle,pp._handle)
	
	def reset_ticks_written(self):
		assert self._handle
		zzub_plugin_reset_ticks_written(self._handle)
	
	def set_attribute_value(self, index, value):
		assert self._handle
		zzub_plugin_set_attribute_value(self._handle,index,value)
	
	def set_auto_write(self, enable):
		assert self._handle
		zzub_plugin_set_auto_write(self._handle,enable)
	
	def set_end_write_position(self, position):
		assert self._handle
		zzub_plugin_set_end_write_position(self._handle,position)
	
	def set_input_channels(self, fromMachine, channels):
		assert self._handle
		zzub_plugin_set_input_channels(self._handle,fromMachine._handle,channels)
	
	def set_mute(self, muted):
		assert self._handle
		zzub_plugin_set_mute(self._handle,muted)
	
	def set_name(self, name):
		assert self._handle
		return zzub_plugin_set_name(self._handle,name)
	
	def set_parameter_value(self, group, track, column, value, record):
		assert self._handle
		zzub_plugin_set_parameter_value(self._handle,group,track,column,value,record)
	
	def set_position(self, x, y):
		assert self._handle
		zzub_plugin_set_position(self._handle,x,y)
	
	def set_start_write_position(self, position):
		assert self._handle
		zzub_plugin_set_start_write_position(self._handle,position)
	
	def set_track_count(self, count):
		assert self._handle
		zzub_plugin_set_track_count(self._handle,count)
	
	def set_wave_file_path(self, path):
		assert self._handle
		return zzub_plugin_set_wave_file_path(self._handle,path)
	
	def set_write_wave(self, enable):
		assert self._handle
		zzub_plugin_set_write_wave(self._handle,enable)
	
	def tick(self):
		assert self._handle
		zzub_plugin_tick(self._handle)
	
class Plugincollection(object):
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
	
class Pluginloader(object):
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
	
	def get_type(self):
		assert self._handle
		return zzub_pluginloader_get_type(self._handle)
	
	def get_uri(self):
		assert self._handle
		return zzub_pluginloader_get_uri(self._handle)
	
class Postprocess(object):
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

class Sequence(object):
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

	def get_event(self, index, pos, value):
		assert self._handle
		return zzub_sequence_get_event(self._handle,index,pos,value)
	
	def get_event_count(self):
		assert self._handle
		return zzub_sequence_get_event_count(self._handle)
	
	def get_plugin(self):
		assert self._handle
		return Plugin(zzub_sequence_get_plugin(self._handle))
	
	def get_value_at(self, pos, exists):
		assert self._handle
		return zzub_sequence_get_value_at(self._handle,pos,exists)
	
	def move_events(self, fromRow, delta):
		assert self._handle
		zzub_sequence_move_events(self._handle,fromRow,delta)
	
	def remove_event_at(self, pos):
		assert self._handle
		zzub_sequence_remove_event_at(self._handle,pos)
	
	def remove_event_range(self, from_pos, to_pos):
		assert self._handle
		zzub_sequence_remove_event_range(self._handle,from_pos,to_pos)
	
	def remove_event_value(self, value):
		assert self._handle
		zzub_sequence_remove_event_value(self._handle,value)
	
	def set_event(self, pos, value):
		assert self._handle
		zzub_sequence_set_event(self._handle,pos,value)
	
class Sequencer(object):
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

	def create_range(self, fromRow, fromTrack, toRow, toTrack):
		assert self._handle
		return Sequencer(zzub_sequencer_create_range(self._handle,fromRow,fromTrack,toRow,toTrack))
	
	def create_track(self, machine):
		assert self._handle
		return Sequence(zzub_sequencer_create_track(self._handle,machine._handle))
	
	def destroy(self):
		assert self._handle
		zzub_sequencer_destroy(self._handle)
	
	def get_track(self, index):
		assert self._handle
		return Sequence(zzub_sequencer_get_track(self._handle,index))
	
	def get_track_count(self):
		assert self._handle
		return zzub_sequencer_get_track_count(self._handle)
	
	def move_track(self, index, newIndex):
		assert self._handle
		zzub_sequencer_move_track(self._handle,index,newIndex)
	
	def remove_track(self, index):
		assert self._handle
		zzub_sequencer_remove_track(self._handle,index)
	
class Wave(object):
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
		zzub_wave_clear(self._handle)
	
	def get_envelope(self, index):
		assert self._handle
		return Envelope(zzub_wave_get_envelope(self._handle,index))
	
	def get_envelope_count(self):
		assert self._handle
		return zzub_wave_get_envelope_count(self._handle)
	
	def get_flags(self):
		assert self._handle
		return zzub_wave_get_flags(self._handle)
	
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
	
	def load_sample(self, level, path):
		assert self._handle
		return zzub_wave_load_sample(self._handle,level,path)
	
	def save_sample(self, level, path):
		assert self._handle
		return zzub_wave_save_sample(self._handle,level,path)
	
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
	
class Wavelevel(object):
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
	
	def get_samples(self):
		assert self._handle
		return zzub_wavelevel_get_samples(self._handle)
	
	def get_samples_per_second(self):
		assert self._handle
		return zzub_wavelevel_get_samples_per_second(self._handle)
	
	def set_loop_end(self, loopend):
		assert self._handle
		zzub_wavelevel_set_loop_end(self._handle,loopend)
	
	def set_loop_start(self, loopstart):
		assert self._handle
		zzub_wavelevel_set_loop_start(self._handle,loopstart)
	
	def set_root_note(self, rootnote):
		assert self._handle
		zzub_wavelevel_set_root_note(self._handle,rootnote)
	
	def set_samples_per_second(self, samplespersecond):
		assert self._handle
		zzub_wavelevel_set_samples_per_second(self._handle,samplespersecond)
	
