#!/usr/bin/env python
# encoding: latin-1

from ctypes import *

import sys, os

def load_library(*names,**kw):
	"""
	searches for a library with given names and returns a ctypes 
	.so/.dll library object if successful. if the library can not
	be loaded, an assertion error will be thrown.
	
	@type  names: list of strings
	@param names: one or more aliases for required libraries, e.g.
				  'SDL','SDL-1.2'.
	@rtype: ctypes CDLL handle
	"""
	import ctypes, os, sys
	searchpaths = []
	if os.name in ('posix', 'mac'):
		if os.environ.has_key('LD_LIBRARY_PATH'):
			searchpaths += os.environ['LD_LIBRARY_PATH'].split(os.pathsep)
		searchpaths += [
			'/usr/local/lib64',
			'/usr/local/lib',
			'/usr/lib64',
			'/usr/lib',
		]
	elif os.name == 'nt':
		searchpaths += ['.']
		if 'PATH' in os.environ:
			searchpaths += os.environ['PATH'].split(os.pathsep)
	else:
		assert 0, "Unknown OS: %s" % os.name
	if 'paths' in kw:
		searchpaths += kw['paths']
	for name in names:
		if os.name == 'nt':
			libname = name + '.dll'
		elif sys.platform == 'darwin':
			libname = 'lib' + name + '.dylib'
			if 'version' in kw:
				libname += '.' + kw['version']
		else:
			libname = 'lib' + name + '.so'
			if 'version' in kw:
				libname += '.' + kw['version']
		m = None
		for path in searchpaths:
			if os.path.isdir(path):
				libpath = os.path.join(path,libname)
				if os.path.isfile(libpath):
					m = ctypes.CDLL(libpath)
					break
				for filename in reversed(sorted(os.listdir(path))):
					if filename.startswith(libname):
						m = ctypes.CDLL(os.path.join(path,filename))
						break
				if m:
					break
		if m:
			break
	assert m, "libraries %s not found in %s" % (','.join(["'%s'" % a for a in names]),','.join(searchpaths))
	return m
	
def dlopen(*args,**kwds):
	"""
	Opens a library by name and returns a handle object. See 
	{library.load} for more information.
	"""
	return load_library(*args,**kwds)

def dlsym(lib, name, restype, *args):
	"""
	Retrieves a symbol from a library loaded by dlopen and
	assigns correct result and argument types.
	
	@param lib: Library object.
	@type lib: ctypes.CDLL
	@param name: Name of symbol.
	@type name: str
	@param restype: Type of function return value.
	@param args: Types of function arguments.	
	"""
	if not lib:
		return None
	proc = getattr(lib,name)
	proc.restype = restype
	proc.argtypes = [argtype for argname,argtype in args]
	proc.o_restype = restype
	proc.o_args = args
	return proc


def callback_from_param(cls, obj):
	"""
	A workaround to assign None to a CFUNCTYPE object.
	"""
	if obj is None:
		return None # return a NULL pointer
	from ctypes import _CFuncPtr
	return _CFuncPtr.from_param(obj)

libzzub = dlopen("zzub", "0.3")

# enum zzub
zzub_version = 15
zzub_buffer_size = 256

# enum zzub_event_type
zzub_event_type_pre_flag = 16384
zzub_event_type_double_click = 0
zzub_event_type_new_plugin = 1
zzub_event_type_delete_plugin = 2
zzub_event_type_pre_delete_plugin = 9
zzub_event_type_disconnect = 3
zzub_event_type_connect = 4
zzub_event_type_plugin_changed = 30
zzub_event_type_parameter_changed = 7
zzub_event_type_set_tracks = 13
zzub_event_type_set_sequence_tracks = 23
zzub_event_type_set_sequence_event = 24
zzub_event_type_new_pattern = 25
zzub_event_type_delete_pattern = 26
zzub_event_type_pre_delete_pattern = 47
zzub_event_type_edit_pattern = 27
zzub_event_type_pattern_changed = 31
zzub_event_type_pattern_insert_rows = 42
zzub_event_type_pattern_remove_rows = 43
zzub_event_type_sequencer_add_track = 32
zzub_event_type_sequencer_remove_track = 33
zzub_event_type_sequencer_changed = 41
zzub_event_type_pre_disconnect = 34
zzub_event_type_pre_connect = 35
zzub_event_type_post_connect = 46
zzub_event_type_pre_set_tracks = 36
zzub_event_type_post_set_tracks = 45
zzub_event_type_envelope_changed = 37
zzub_event_type_slices_changed = 38
zzub_event_type_wave_changed = 39
zzub_event_type_delete_wave = 40
zzub_event_type_load_progress = 8
zzub_event_type_midi_control = 11
zzub_event_type_wave_allocated = 12
zzub_event_type_player_state_changed = 20
zzub_event_type_osc_message = 21
zzub_event_type_vu = 22
zzub_event_type_custom = 44
zzub_event_type_all = 255

# enum zzub_player_state
zzub_player_state_playing = 0
zzub_player_state_stopped = 1
zzub_player_state_muted = 2
zzub_player_state_released = 3

# enum zzub_parameter_type
zzub_parameter_type_note = 0
zzub_parameter_type_switch = 1
zzub_parameter_type_byte = 2
zzub_parameter_type_word = 3

# enum zzub_wave_buffer_type
zzub_wave_buffer_type_si16 = 0
zzub_wave_buffer_type_f32 = 1
zzub_wave_buffer_type_si32 = 2
zzub_wave_buffer_type_si24 = 3

# enum zzub_oscillator_type
zzub_oscillator_type_sine = 0
zzub_oscillator_type_sawtooth = 1
zzub_oscillator_type_pulse = 2
zzub_oscillator_type_triangle = 3
zzub_oscillator_type_noise = 4
zzub_oscillator_type_sawtooth_303 = 5

# enum zzub_note_value
zzub_note_value_none = 0
zzub_note_value_off = 255
zzub_note_value_min = 1
zzub_note_value_max = 156
zzub_note_value_c4 = 65

# enum zzub_switch_value
zzub_switch_value_none = 255
zzub_switch_value_off = 0
zzub_switch_value_on = 1

# enum zzub_wavetable_index_value
zzub_wavetable_index_value_none = 0
zzub_wavetable_index_value_min = 1
zzub_wavetable_index_value_max = 200

# enum zzub_parameter_flag
zzub_parameter_flag_wavetable_index = (1 << 0)
zzub_parameter_flag_state = (1 << 1)
zzub_parameter_flag_event_on_edit = (1 << 2)

# enum zzub_plugin_flag
zzub_plugin_flag_mono_to_stereo = (1 << 0)
zzub_plugin_flag_plays_waves = (1 << 1)
zzub_plugin_flag_uses_lib_interface = (1 << 2)
zzub_plugin_flag_uses_instruments = (1 << 3)
zzub_plugin_flag_does_input_mixing = (1 << 4)
zzub_plugin_flag_no_output = (1 << 5)
zzub_plugin_flag_control_plugin = (1 << 6)
zzub_plugin_flag_auxiliary = (1 << 7)
zzub_plugin_flag_is_root = (1 << 16)
zzub_plugin_flag_has_audio_input = (1 << 17)
zzub_plugin_flag_has_audio_output = (1 << 18)
zzub_plugin_flag_has_event_input = (1 << 19)
zzub_plugin_flag_has_event_output = (1 << 20)
zzub_plugin_flag_offline = (1 << 21)
zzub_plugin_flag_stream = (1 << 22)
zzub_plugin_flag_import = (1 << 25)
zzub_plugin_flag_has_midi_input = (1 << 23)
zzub_plugin_flag_has_midi_output = (1 << 24)
zzub_plugin_flag_no_undo = (1 << 25)
zzub_plugin_flag_no_save = (1 << 26)

# enum zzub_state_flag
zzub_state_flag_playing = 1
zzub_state_flag_recording = 2

# enum zzub_wave_flag
zzub_wave_flag_loop = (1 << 0)
zzub_wave_flag_extended = (1 << 2)
zzub_wave_flag_stereo = (1 << 3)
zzub_wave_flag_pingpong = (1 << 4)
zzub_wave_flag_envelope = (1 << 7)

# enum zzub_envelope_flag
zzub_envelope_flag_sustain = (1 << 0)
zzub_envelope_flag_loop = (1 << 1)

# enum zzub_process_mode
zzub_process_mode_no_io = 0
zzub_process_mode_read = (1 << 0)
zzub_process_mode_write = (1 << 1)
zzub_process_mode_read_write = (1 << 0)|(1 << 1)

# enum zzub_connection_type
zzub_connection_type_audio = 0
zzub_connection_type_event = 1
zzub_connection_type_midi = 2

# enum zzub_parameter_group
zzub_parameter_group_connection = 0
zzub_parameter_group_global = 1
zzub_parameter_group_track = 2

# enum zzub_sequence_type
zzub_sequence_type_pattern = 0
zzub_sequence_type_wave = 1
zzub_sequence_type_automation = 2
class zzub_plugin_t(Structure):
	"""Plugin methods
	Retreive more details about plugins."""
	_fields_ = [
	]
	_anonymous_ = [
	]
class zzub_event_data_new_plugin_t(Structure):
	_fields_ = [
		("plugin", POINTER(zzub_plugin_t)),
	]
	_anonymous_ = [
	]
class zzub_event_data_delete_plugin_t(Structure):
	_fields_ = [
		("plugin", POINTER(zzub_plugin_t)),
	]
	_anonymous_ = [
	]
class zzub_event_data_midi_message_t(Structure):
	_fields_ = [
		("status", c_ubyte),
		("data1", c_ubyte),
		("data2", c_ubyte),
	]
	_anonymous_ = [
	]
class zzub_event_data_connect_t(Structure):
	_fields_ = [
		("from_plugin", POINTER(zzub_plugin_t)),
		("to_plugin", POINTER(zzub_plugin_t)),
		("type", c_int),
	]
	_anonymous_ = [
	]
class zzub_event_data_plugin_changed_t(Structure):
	_fields_ = [
		("plugin", POINTER(zzub_plugin_t)),
	]
	_anonymous_ = [
	]
class zzub_event_data_change_parameter_t(Structure):
	_fields_ = [
		("plugin", POINTER(zzub_plugin_t)),
		("group", c_int),
		("track", c_int),
		("param", c_int),
		("value", c_int),
	]
	_anonymous_ = [
	]
class zzub_event_data_set_tracks_t(Structure):
	_fields_ = [
		("plugin", POINTER(zzub_plugin_t)),
	]
	_anonymous_ = [
	]
class zzub_event_data_player_state_changed_t(Structure):
	_fields_ = [
		("player_state", c_int),
	]
	_anonymous_ = [
	]
class zzub_event_data_osc_message_t(Structure):
	_fields_ = [
		("path", c_char_p),
		("types", c_char_p),
		("argv", POINTER(c_void_p)),
		("argc", c_int),
		("msg", c_void_p),
	]
	_anonymous_ = [
	]
class zzub_event_data_vu_t(Structure):
	_fields_ = [
		("size", c_int),
		("left_amp", c_float),
		("right_amp", c_float),
		("time", c_float),
	]
	_anonymous_ = [
	]
class zzub_archive_t(Structure):
	_fields_ = [
	]
	_anonymous_ = [
	]
class zzub_event_data_serialize_t(Structure):
	_fields_ = [
		("mode", c_char),
		("archive", POINTER(zzub_archive_t)),
	]
	_anonymous_ = [
	]
class zzub_event_data_set_sequence_tracks_t(Structure):
	_fields_ = [
		("plugin", POINTER(zzub_plugin_t)),
	]
	_anonymous_ = [
	]
class zzub_event_data_set_sequence_event_t(Structure):
	_fields_ = [
		("plugin", POINTER(zzub_plugin_t)),
		("track", c_int),
		("time", c_int),
	]
	_anonymous_ = [
	]
class zzub_event_data_new_pattern_t(Structure):
	_fields_ = [
		("plugin", POINTER(zzub_plugin_t)),
		("index", c_int),
	]
	_anonymous_ = [
	]
class zzub_event_data_delete_pattern_t(Structure):
	_fields_ = [
		("plugin", POINTER(zzub_plugin_t)),
		("index", c_int),
	]
	_anonymous_ = [
	]
class zzub_event_data_edit_pattern_t(Structure):
	_fields_ = [
		("plugin", POINTER(zzub_plugin_t)),
		("index", c_int),
		("group", c_int),
		("track", c_int),
		("column", c_int),
		("row", c_int),
		("value", c_int),
	]
	_anonymous_ = [
	]
class zzub_event_data_pattern_changed_t(Structure):
	_fields_ = [
		("plugin", POINTER(zzub_plugin_t)),
		("index", c_int),
	]
	_anonymous_ = [
	]
class zzub_wave_t(Structure):
	"""Wave table"""
	_fields_ = [
	]
	_anonymous_ = [
	]
class zzub_event_data_change_wave_t(Structure):
	_fields_ = [
		("wave", POINTER(zzub_wave_t)),
	]
	_anonymous_ = [
	]
class zzub_event_data_delete_wave_t(Structure):
	_fields_ = [
		("wave", POINTER(zzub_wave_t)),
	]
	_anonymous_ = [
	]
class zzub_wavelevel_t(Structure):
	"""Wavelevel"""
	_fields_ = [
	]
	_anonymous_ = [
	]
class zzub_event_data_allocate_wavelevel_t(Structure):
	_fields_ = [
		("wavelevel", POINTER(zzub_wavelevel_t)),
	]
	_anonymous_ = [
	]
class zzub_event_data_pattern_insert_rows_t(Structure):
	_fields_ = [
		("plugin", POINTER(zzub_plugin_t)),
		("index", c_int),
		("row", c_int),
		("rows", c_int),
		("column_indices", POINTER(c_int)),
		("indices", c_int),
	]
	_anonymous_ = [
	]
class zzub_event_data_pattern_remove_rows_t(Structure):
	_fields_ = [
		("plugin", POINTER(zzub_plugin_t)),
		("index", c_int),
		("row", c_int),
		("rows", c_int),
		("column_indices", POINTER(c_int)),
		("indices", c_int),
	]
	_anonymous_ = [
	]
class zzub_event_data_custom_t(Structure):
	_fields_ = [
		("id", c_int),
		("data", c_void_p),
	]
	_anonymous_ = [
	]
class zzub_event_data_all_t(Structure):
	_fields_ = [
		("data", POINTER("zzub_event_data_t")),
	]
	_anonymous_ = [
	]
class zzub_event_data_unknown_t(Structure):
	_fields_ = [
		("param", c_void_p),
	]
	_anonymous_ = [
	]
class zzub_event_data_union_00000000_t(Union):
	_fields_ = [
		("new_plugin", zzub_event_data_new_plugin_t),
		("delete_plugin", zzub_event_data_delete_plugin_t),
		("midi_message", zzub_event_data_midi_message_t),
		("connect_plugin", zzub_event_data_connect_t),
		("disconnect_plugin", zzub_event_data_connect_t),
		("plugin_changed", zzub_event_data_plugin_changed_t),
		("change_parameter", zzub_event_data_change_parameter_t),
		("set_tracks", zzub_event_data_set_tracks_t),
		("player_state_changed", zzub_event_data_player_state_changed_t),
		("osc_message", zzub_event_data_osc_message_t),
		("vu", zzub_event_data_vu_t),
		("serialize", zzub_event_data_serialize_t),
		("set_sequence_tracks", zzub_event_data_set_sequence_tracks_t),
		("set_sequence_event", zzub_event_data_set_sequence_event_t),
		("new_pattern", zzub_event_data_new_pattern_t),
		("delete_pattern", zzub_event_data_delete_pattern_t),
		("edit_pattern", zzub_event_data_edit_pattern_t),
		("pattern_changed", zzub_event_data_pattern_changed_t),
		("change_wave", zzub_event_data_change_wave_t),
		("delete_wave", zzub_event_data_delete_wave_t),
		("allocate_wavelevel", zzub_event_data_allocate_wavelevel_t),
		("pattern_insert_rows", zzub_event_data_pattern_insert_rows_t),
		("pattern_remove_rows", zzub_event_data_pattern_remove_rows_t),
		("custom", zzub_event_data_custom_t),
		("all", zzub_event_data_all_t),
		("unknown", zzub_event_data_unknown_t),
	]
class zzub_event_data_t(Structure):
	_fields_ = [
		("type", c_int),
		("union_00000000",zzub_event_data_union_00000000_t),
	]
	_anonymous_ = [
		"union_00000000",
	]
class zzub_audiodriver_t(Structure):
	"""Audio Driver Methods
	Configure and create an audio driver instance."""
	_fields_ = [
	]
	_anonymous_ = [
	]
class zzub_mididriver_t(Structure):
	"""MIDI Driver Methods
	Open midi devices."""
	_fields_ = [
	]
	_anonymous_ = [
	]
class zzub_plugincollection_t(Structure):
	"""Plugin Collection Methods
	For enumerating and configuring plugin collections."""
	_fields_ = [
	]
	_anonymous_ = [
	]
class zzub_input_t(Structure):
	_fields_ = [
	]
	_anonymous_ = [
	]
class zzub_output_t(Structure):
	_fields_ = [
	]
	_anonymous_ = [
	]
class zzub_midimapping_t(Structure):
	"""MIDI Mapping Methods"""
	_fields_ = [
	]
	_anonymous_ = [
	]
class zzub_pattern_t(Structure):
	"""Offline pattern methods
	These functions are meant to help editing patterns. Note you cannot
	retreive a direct zzub_pattern_t object for a "live pattern". You can
	however, use zzub_plugin_get_pattern to retreive copies of live patterns,
	and then call zzub_plugin_update_pattern to write the changed pattern back
	to the engine.
	Alternately, zzub_plugin_get_pattern_value/zzub_plugin_set_pattern_value
	can also be used to edit single values in live patterns."""
	_fields_ = [
	]
	_anonymous_ = [
	]
class zzub_parameter_t(Structure):
	"""Parameter methods
	Retreive more details from zzub_parameter_t objects."""
	_fields_ = [
	]
	_anonymous_ = [
	]
class zzub_attribute_t(Structure):
	"""Attribute methods
	Retreive more details from zzub_attribute_t objects."""
	_fields_ = [
	]
	_anonymous_ = [
	]
class zzub_pluginloader_t(Structure):
	"""Plugin loading methods
	Retreive more details from zzub_pluginloader_t objects."""
	_fields_ = [
	]
	_anonymous_ = [
	]
class zzub_sequence_t(Structure):
	"""Sequencer methods"""
	_fields_ = [
	]
	_anonymous_ = [
	]
class zzub_envelope_t(Structure):
	"""Envelopes"""
	_fields_ = [
	]
	_anonymous_ = [
	]
class zzub_recorder_t(Structure):
	"""Memory and file streams - load/save from/to file/clipboard
	Create file or memory data streams for use by e.g 
	zzub_wavetable_load_sample() and
	zzub_player_load_bmx()/zzub_player_save_bmx().
	
	In-memory streams are implemented via the zzub_archive_t object
	and destroyed via zzub_archive_destroy().
	File-streams are created with zzub_input_open_file and zzub_output_create_file()
	and closed/destroyed with zzub_input_destroy() and zzub_output_destroy()."""
	_fields_ = [
	]
	_anonymous_ = [
	]
class zzub_player_t(Structure):
	"""Player Methods"""
	_fields_ = [
	]
	_anonymous_ = [
	]
zzub_callback_t = CFUNCTYPE(c_int, POINTER(zzub_player_t), POINTER(zzub_plugin_t), POINTER(zzub_event_data_t), c_void_p)
zzub_callback_t.from_param = classmethod(callback_from_param)
zzub_mix_callback_t = CFUNCTYPE(None, POINTER(c_float), POINTER(c_float), c_int, c_void_p)
zzub_mix_callback_t.from_param = classmethod(callback_from_param)
zzub_audiodriver_create_portaudio = dlsym(libzzub, "zzub_audiodriver_create_portaudio", POINTER(zzub_audiodriver_t), ("player",POINTER(zzub_player_t)))
zzub_audiodriver_create_rtaudio = dlsym(libzzub, "zzub_audiodriver_create_rtaudio", POINTER(zzub_audiodriver_t), ("player",POINTER(zzub_player_t)))
zzub_audiodriver_create_silent = dlsym(libzzub, "zzub_audiodriver_create_silent", POINTER(zzub_audiodriver_t), ("player",POINTER(zzub_player_t)), ("name",c_char_p), ("out_channels",c_int), ("in_channels",c_int), ("supported_rates",POINTER(c_int)), ("num_rates",c_int))
zzub_audiodriver_create = dlsym(libzzub, "zzub_audiodriver_create", POINTER(zzub_audiodriver_t), ("player",POINTER(zzub_player_t)))
zzub_audiodriver_get_count = dlsym(libzzub, "zzub_audiodriver_get_count", c_int, ("audiodriver", POINTER(zzub_audiodriver_t)))
zzub_audiodriver_get_name = dlsym(libzzub, "zzub_audiodriver_get_name", c_int, ("audiodriver", POINTER(zzub_audiodriver_t)), ("index",c_int), ("name",c_char_p), ("max_len",c_int))
zzub_audiodriver_get_supported_samplerates = dlsym(libzzub, "zzub_audiodriver_get_supported_samplerates", c_int, ("audiodriver", POINTER(zzub_audiodriver_t)), ("index",c_int), ("result",POINTER(c_int)), ("maxrates",c_int))
zzub_audiodriver_get_supported_output_channels = dlsym(libzzub, "zzub_audiodriver_get_supported_output_channels", c_int, ("audiodriver", POINTER(zzub_audiodriver_t)), ("index",c_int))
zzub_audiodriver_get_supported_input_channels = dlsym(libzzub, "zzub_audiodriver_get_supported_input_channels", c_int, ("audiodriver", POINTER(zzub_audiodriver_t)), ("index",c_int))
zzub_audiodriver_create_device = dlsym(libzzub, "zzub_audiodriver_create_device", c_int, ("audiodriver", POINTER(zzub_audiodriver_t)), ("input_index",c_int), ("output_index",c_int))
zzub_audiodriver_enable = dlsym(libzzub, "zzub_audiodriver_enable", None, ("audiodriver", POINTER(zzub_audiodriver_t)), ("state",c_int))
zzub_audiodriver_get_enabled = dlsym(libzzub, "zzub_audiodriver_get_enabled", c_int, ("audiodriver", POINTER(zzub_audiodriver_t)))
zzub_audiodriver_destroy = dlsym(libzzub, "zzub_audiodriver_destroy", None, ("audiodriver", POINTER(zzub_audiodriver_t)))
zzub_audiodriver_destroy_device = dlsym(libzzub, "zzub_audiodriver_destroy_device", None, ("audiodriver", POINTER(zzub_audiodriver_t)))
zzub_audiodriver_set_samplerate = dlsym(libzzub, "zzub_audiodriver_set_samplerate", None, ("audiodriver", POINTER(zzub_audiodriver_t)), ("samplerate",c_uint))
zzub_audiodriver_get_samplerate = dlsym(libzzub, "zzub_audiodriver_get_samplerate", c_uint, ("audiodriver", POINTER(zzub_audiodriver_t)))
zzub_audiodriver_set_buffersize = dlsym(libzzub, "zzub_audiodriver_set_buffersize", None, ("audiodriver", POINTER(zzub_audiodriver_t)), ("buffersize",c_uint))
zzub_audiodriver_get_buffersize = dlsym(libzzub, "zzub_audiodriver_get_buffersize", c_uint, ("audiodriver", POINTER(zzub_audiodriver_t)))
zzub_audiodriver_get_cpu_load = dlsym(libzzub, "zzub_audiodriver_get_cpu_load", c_double, ("audiodriver", POINTER(zzub_audiodriver_t)))
zzub_audiodriver_is_output = dlsym(libzzub, "zzub_audiodriver_is_output", c_int, ("audiodriver", POINTER(zzub_audiodriver_t)), ("index",c_int))
zzub_audiodriver_is_input = dlsym(libzzub, "zzub_audiodriver_is_input", c_int, ("audiodriver", POINTER(zzub_audiodriver_t)), ("index",c_int))
zzub_audiodriver_get_master_channel = dlsym(libzzub, "zzub_audiodriver_get_master_channel", c_int, ("audiodriver", POINTER(zzub_audiodriver_t)))
zzub_audiodriver_set_master_channel = dlsym(libzzub, "zzub_audiodriver_set_master_channel", None, ("audiodriver", POINTER(zzub_audiodriver_t)), ("index",c_int))
zzub_mididriver_get_count = dlsym(libzzub, "zzub_mididriver_get_count", c_int, ("player",POINTER(zzub_player_t)))
zzub_mididriver_get_name = dlsym(libzzub, "zzub_mididriver_get_name", c_char_p, ("player",POINTER(zzub_player_t)), ("index",c_int))
zzub_mididriver_is_input = dlsym(libzzub, "zzub_mididriver_is_input", c_int, ("player",POINTER(zzub_player_t)), ("index",c_int))
zzub_mididriver_is_output = dlsym(libzzub, "zzub_mididriver_is_output", c_int, ("player",POINTER(zzub_player_t)), ("index",c_int))
zzub_mididriver_open = dlsym(libzzub, "zzub_mididriver_open", c_int, ("player",POINTER(zzub_player_t)), ("index",c_int))
zzub_mididriver_close_all = dlsym(libzzub, "zzub_mididriver_close_all", c_int, ("player",POINTER(zzub_player_t)))
zzub_plugincollection_get_by_uri = dlsym(libzzub, "zzub_plugincollection_get_by_uri", POINTER(zzub_plugincollection_t), ("player",POINTER(zzub_player_t)), ("uri",c_char_p))
zzub_plugincollection_configure = dlsym(libzzub, "zzub_plugincollection_configure", None, ("plugincollection", POINTER(zzub_plugincollection_t)), ("key",c_char_p), ("value",c_char_p))
zzub_input_open_file = dlsym(libzzub, "zzub_input_open_file", POINTER(zzub_input_t), ("filename",c_char_p))
zzub_input_destroy = dlsym(libzzub, "zzub_input_destroy", None, ("input", POINTER(zzub_input_t)))
zzub_input_read = dlsym(libzzub, "zzub_input_read", None, ("input", POINTER(zzub_input_t)), ("buffer",POINTER(c_char)), ("bytes",c_int))
zzub_input_size = dlsym(libzzub, "zzub_input_size", c_int, ("input", POINTER(zzub_input_t)))
zzub_input_position = dlsym(libzzub, "zzub_input_position", c_int, ("input", POINTER(zzub_input_t)))
zzub_input_seek = dlsym(libzzub, "zzub_input_seek", None, ("input", POINTER(zzub_input_t)), ("pos",c_int), ("mode",c_int))
zzub_output_create_file = dlsym(libzzub, "zzub_output_create_file", POINTER(zzub_output_t), ("filename",c_char_p))
zzub_output_destroy = dlsym(libzzub, "zzub_output_destroy", None, ("output", POINTER(zzub_output_t)))
zzub_output_write = dlsym(libzzub, "zzub_output_write", None, ("output", POINTER(zzub_output_t)), ("buffer",POINTER(c_char)), ("bytes",c_int))
zzub_output_position = dlsym(libzzub, "zzub_output_position", c_int, ("output", POINTER(zzub_output_t)))
zzub_output_seek = dlsym(libzzub, "zzub_output_seek", None, ("output", POINTER(zzub_output_t)), ("pos",c_int), ("mode",c_int))
zzub_archive_create_memory = dlsym(libzzub, "zzub_archive_create_memory", POINTER(zzub_archive_t))
zzub_archive_get_output = dlsym(libzzub, "zzub_archive_get_output", POINTER(zzub_output_t), ("archive", POINTER(zzub_archive_t)), ("path",c_char_p))
zzub_archive_get_input = dlsym(libzzub, "zzub_archive_get_input", POINTER(zzub_input_t), ("archive", POINTER(zzub_archive_t)), ("path",c_char_p))
zzub_archive_destroy = dlsym(libzzub, "zzub_archive_destroy", None, ("archive", POINTER(zzub_archive_t)))
zzub_midimapping_get_plugin = dlsym(libzzub, "zzub_midimapping_get_plugin", c_int, ("midimapping", POINTER(zzub_midimapping_t)))
zzub_midimapping_get_group = dlsym(libzzub, "zzub_midimapping_get_group", c_int, ("midimapping", POINTER(zzub_midimapping_t)))
zzub_midimapping_get_track = dlsym(libzzub, "zzub_midimapping_get_track", c_int, ("midimapping", POINTER(zzub_midimapping_t)))
zzub_midimapping_get_column = dlsym(libzzub, "zzub_midimapping_get_column", c_int, ("midimapping", POINTER(zzub_midimapping_t)))
zzub_midimapping_get_channel = dlsym(libzzub, "zzub_midimapping_get_channel", c_int, ("midimapping", POINTER(zzub_midimapping_t)))
zzub_midimapping_get_controller = dlsym(libzzub, "zzub_midimapping_get_controller", c_int, ("midimapping", POINTER(zzub_midimapping_t)))
zzub_pattern_destroy = dlsym(libzzub, "zzub_pattern_destroy", None, ("pattern", POINTER(zzub_pattern_t)))
zzub_pattern_get_name = dlsym(libzzub, "zzub_pattern_get_name", None, ("pattern", POINTER(zzub_pattern_t)), ("name",c_char_p), ("maxLen",c_int))
zzub_pattern_set_name = dlsym(libzzub, "zzub_pattern_set_name", None, ("pattern", POINTER(zzub_pattern_t)), ("name",c_char_p))
zzub_pattern_get_row_count = dlsym(libzzub, "zzub_pattern_get_row_count", c_int, ("pattern", POINTER(zzub_pattern_t)))
zzub_pattern_get_group_count = dlsym(libzzub, "zzub_pattern_get_group_count", c_int, ("pattern", POINTER(zzub_pattern_t)))
zzub_pattern_get_track_count = dlsym(libzzub, "zzub_pattern_get_track_count", c_int, ("pattern", POINTER(zzub_pattern_t)), ("group",c_int))
zzub_pattern_get_column_count = dlsym(libzzub, "zzub_pattern_get_column_count", c_int, ("pattern", POINTER(zzub_pattern_t)), ("group",c_int), ("track",c_int))
zzub_pattern_get_value = dlsym(libzzub, "zzub_pattern_get_value", c_int, ("pattern", POINTER(zzub_pattern_t)), ("row",c_int), ("group",c_int), ("track",c_int), ("column",c_int))
zzub_pattern_set_value = dlsym(libzzub, "zzub_pattern_set_value", None, ("pattern", POINTER(zzub_pattern_t)), ("row",c_int), ("group",c_int), ("track",c_int), ("column",c_int), ("value",c_int))
zzub_pattern_interpolate = dlsym(libzzub, "zzub_pattern_interpolate", None, ("pattern", POINTER(zzub_pattern_t)))
zzub_parameter_get_type = dlsym(libzzub, "zzub_parameter_get_type", c_int, ("parameter", POINTER(zzub_parameter_t)))
zzub_parameter_get_name = dlsym(libzzub, "zzub_parameter_get_name", c_char_p, ("parameter", POINTER(zzub_parameter_t)))
zzub_parameter_get_description = dlsym(libzzub, "zzub_parameter_get_description", c_char_p, ("parameter", POINTER(zzub_parameter_t)))
zzub_parameter_get_value_min = dlsym(libzzub, "zzub_parameter_get_value_min", c_int, ("parameter", POINTER(zzub_parameter_t)))
zzub_parameter_get_value_max = dlsym(libzzub, "zzub_parameter_get_value_max", c_int, ("parameter", POINTER(zzub_parameter_t)))
zzub_parameter_get_value_none = dlsym(libzzub, "zzub_parameter_get_value_none", c_int, ("parameter", POINTER(zzub_parameter_t)))
zzub_parameter_get_value_default = dlsym(libzzub, "zzub_parameter_get_value_default", c_int, ("parameter", POINTER(zzub_parameter_t)))
zzub_parameter_get_flags = dlsym(libzzub, "zzub_parameter_get_flags", c_int, ("parameter", POINTER(zzub_parameter_t)))
zzub_attribute_get_name = dlsym(libzzub, "zzub_attribute_get_name", c_char_p, ("attribute", POINTER(zzub_attribute_t)))
zzub_attribute_get_value_min = dlsym(libzzub, "zzub_attribute_get_value_min", c_int, ("attribute", POINTER(zzub_attribute_t)))
zzub_attribute_get_value_max = dlsym(libzzub, "zzub_attribute_get_value_max", c_int, ("attribute", POINTER(zzub_attribute_t)))
zzub_attribute_get_value_default = dlsym(libzzub, "zzub_attribute_get_value_default", c_int, ("attribute", POINTER(zzub_attribute_t)))
zzub_pluginloader_get_name = dlsym(libzzub, "zzub_pluginloader_get_name", c_char_p, ("pluginloader", POINTER(zzub_pluginloader_t)))
zzub_pluginloader_get_short_name = dlsym(libzzub, "zzub_pluginloader_get_short_name", c_char_p, ("pluginloader", POINTER(zzub_pluginloader_t)))
zzub_pluginloader_get_parameter_count = dlsym(libzzub, "zzub_pluginloader_get_parameter_count", c_int, ("pluginloader", POINTER(zzub_pluginloader_t)), ("group",c_int))
zzub_pluginloader_get_parameter = dlsym(libzzub, "zzub_pluginloader_get_parameter", POINTER(zzub_parameter_t), ("pluginloader", POINTER(zzub_pluginloader_t)), ("group",c_int), ("index",c_int))
zzub_pluginloader_get_attribute_count = dlsym(libzzub, "zzub_pluginloader_get_attribute_count", c_int, ("pluginloader", POINTER(zzub_pluginloader_t)))
zzub_pluginloader_get_attribute = dlsym(libzzub, "zzub_pluginloader_get_attribute", POINTER(zzub_attribute_t), ("pluginloader", POINTER(zzub_pluginloader_t)), ("index",c_int))
zzub_pluginloader_get_loader_name = dlsym(libzzub, "zzub_pluginloader_get_loader_name", c_char_p, ("pluginloader", POINTER(zzub_pluginloader_t)))
zzub_pluginloader_get_flags = dlsym(libzzub, "zzub_pluginloader_get_flags", c_int, ("pluginloader", POINTER(zzub_pluginloader_t)))
zzub_pluginloader_get_uri = dlsym(libzzub, "zzub_pluginloader_get_uri", c_char_p, ("pluginloader", POINTER(zzub_pluginloader_t)))
zzub_pluginloader_get_author = dlsym(libzzub, "zzub_pluginloader_get_author", c_char_p, ("pluginloader", POINTER(zzub_pluginloader_t)))
zzub_pluginloader_get_instrument_list = dlsym(libzzub, "zzub_pluginloader_get_instrument_list", c_int, ("pluginloader", POINTER(zzub_pluginloader_t)), ("result",POINTER(c_char)), ("maxbytes",c_int))
zzub_pluginloader_get_tracks_min = dlsym(libzzub, "zzub_pluginloader_get_tracks_min", c_int, ("pluginloader", POINTER(zzub_pluginloader_t)))
zzub_pluginloader_get_tracks_max = dlsym(libzzub, "zzub_pluginloader_get_tracks_max", c_int, ("pluginloader", POINTER(zzub_pluginloader_t)))
zzub_pluginloader_get_stream_format_count = dlsym(libzzub, "zzub_pluginloader_get_stream_format_count", c_int, ("pluginloader", POINTER(zzub_pluginloader_t)))
zzub_pluginloader_get_stream_format_ext = dlsym(libzzub, "zzub_pluginloader_get_stream_format_ext", c_char_p, ("pluginloader", POINTER(zzub_pluginloader_t)), ("index",c_int))
zzub_plugin_destroy = dlsym(libzzub, "zzub_plugin_destroy", c_int, ("plugin", POINTER(zzub_plugin_t)))
zzub_plugin_load = dlsym(libzzub, "zzub_plugin_load", c_int, ("plugin", POINTER(zzub_plugin_t)), ("input",POINTER(zzub_input_t)))
zzub_plugin_save = dlsym(libzzub, "zzub_plugin_save", c_int, ("plugin", POINTER(zzub_plugin_t)), ("ouput",POINTER(zzub_output_t)))
zzub_plugin_set_name = dlsym(libzzub, "zzub_plugin_set_name", c_int, ("plugin", POINTER(zzub_plugin_t)), ("name",c_char_p))
zzub_plugin_get_name = dlsym(libzzub, "zzub_plugin_get_name", c_int, ("plugin", POINTER(zzub_plugin_t)), ("name",c_char_p), ("maxlen",c_int))
zzub_plugin_get_id = dlsym(libzzub, "zzub_plugin_get_id", c_int, ("plugin", POINTER(zzub_plugin_t)))
zzub_plugin_get_position = dlsym(libzzub, "zzub_plugin_get_position", None, ("plugin", POINTER(zzub_plugin_t)), ("x",POINTER(c_float)), ("y",POINTER(c_float)))
zzub_plugin_set_position = dlsym(libzzub, "zzub_plugin_set_position", None, ("plugin", POINTER(zzub_plugin_t)), ("x",c_float), ("y",c_float))
zzub_plugin_set_position_direct = dlsym(libzzub, "zzub_plugin_set_position_direct", None, ("plugin", POINTER(zzub_plugin_t)), ("x",c_float), ("y",c_float))
zzub_plugin_get_flags = dlsym(libzzub, "zzub_plugin_get_flags", c_int, ("plugin", POINTER(zzub_plugin_t)))
zzub_plugin_get_track_count = dlsym(libzzub, "zzub_plugin_get_track_count", c_int, ("plugin", POINTER(zzub_plugin_t)))
zzub_plugin_set_track_count = dlsym(libzzub, "zzub_plugin_set_track_count", None, ("plugin", POINTER(zzub_plugin_t)), ("count",c_int))
zzub_plugin_get_group_track_count = dlsym(libzzub, "zzub_plugin_get_group_track_count", c_int, ("plugin", POINTER(zzub_plugin_t)), ("group",c_int))
zzub_plugin_get_mute = dlsym(libzzub, "zzub_plugin_get_mute", c_int, ("plugin", POINTER(zzub_plugin_t)))
zzub_plugin_set_mute = dlsym(libzzub, "zzub_plugin_set_mute", None, ("plugin", POINTER(zzub_plugin_t)), ("muted",c_int))
zzub_plugin_get_bypass = dlsym(libzzub, "zzub_plugin_get_bypass", c_int, ("plugin", POINTER(zzub_plugin_t)))
zzub_plugin_configure = dlsym(libzzub, "zzub_plugin_configure", None, ("plugin", POINTER(zzub_plugin_t)), ("key",c_char_p), ("value",c_char_p))
zzub_plugin_set_bypass = dlsym(libzzub, "zzub_plugin_set_bypass", None, ("plugin", POINTER(zzub_plugin_t)), ("muted",c_int))
zzub_plugin_get_commands = dlsym(libzzub, "zzub_plugin_get_commands", c_int, ("plugin", POINTER(zzub_plugin_t)), ("commands",c_char_p), ("maxlen",c_int))
zzub_plugin_get_sub_commands = dlsym(libzzub, "zzub_plugin_get_sub_commands", c_int, ("plugin", POINTER(zzub_plugin_t)), ("i",c_int), ("commands",c_char_p), ("maxlen",c_int))
zzub_plugin_command = dlsym(libzzub, "zzub_plugin_command", None, ("plugin", POINTER(zzub_plugin_t)), ("i",c_int))
zzub_plugin_get_pluginloader = dlsym(libzzub, "zzub_plugin_get_pluginloader", POINTER(zzub_pluginloader_t), ("plugin", POINTER(zzub_plugin_t)))
zzub_plugin_get_midi_output_device_count = dlsym(libzzub, "zzub_plugin_get_midi_output_device_count", c_int, ("plugin", POINTER(zzub_plugin_t)))
zzub_plugin_get_midi_output_device = dlsym(libzzub, "zzub_plugin_get_midi_output_device", c_char_p, ("plugin", POINTER(zzub_plugin_t)), ("index",c_int))
zzub_plugin_get_envelope_count = dlsym(libzzub, "zzub_plugin_get_envelope_count", c_int, ("plugin", POINTER(zzub_plugin_t)))
zzub_plugin_get_envelope_flags = dlsym(libzzub, "zzub_plugin_get_envelope_flags", c_int, ("plugin", POINTER(zzub_plugin_t)), ("index",c_int))
zzub_plugin_get_envelope_name = dlsym(libzzub, "zzub_plugin_get_envelope_name", c_char_p, ("plugin", POINTER(zzub_plugin_t)), ("index",c_int))
zzub_plugin_set_stream_source = dlsym(libzzub, "zzub_plugin_set_stream_source", None, ("plugin", POINTER(zzub_plugin_t)), ("resource",c_char_p))
zzub_plugin_set_instrument = dlsym(libzzub, "zzub_plugin_set_instrument", c_int, ("plugin", POINTER(zzub_plugin_t)), ("name",c_char_p))
zzub_plugin_create_range_pattern = dlsym(libzzub, "zzub_plugin_create_range_pattern", POINTER(zzub_pattern_t), ("player",POINTER(zzub_player_t)), ("columns",c_int), ("rows",c_int))
zzub_plugin_create_pattern = dlsym(libzzub, "zzub_plugin_create_pattern", POINTER(zzub_pattern_t), ("plugin", POINTER(zzub_plugin_t)), ("rows",c_int))
zzub_plugin_get_pattern_count = dlsym(libzzub, "zzub_plugin_get_pattern_count", c_int, ("plugin", POINTER(zzub_plugin_t)))
zzub_plugin_add_pattern = dlsym(libzzub, "zzub_plugin_add_pattern", None, ("plugin", POINTER(zzub_plugin_t)), ("pattern",POINTER(zzub_pattern_t)))
zzub_plugin_remove_pattern = dlsym(libzzub, "zzub_plugin_remove_pattern", None, ("plugin", POINTER(zzub_plugin_t)), ("pattern",c_int))
zzub_plugin_move_pattern = dlsym(libzzub, "zzub_plugin_move_pattern", None, ("plugin", POINTER(zzub_plugin_t)), ("index",c_int), ("newIndex",c_int))
zzub_plugin_update_pattern = dlsym(libzzub, "zzub_plugin_update_pattern", None, ("plugin", POINTER(zzub_plugin_t)), ("index",c_int), ("pattern",POINTER(zzub_pattern_t)))
zzub_plugin_get_pattern = dlsym(libzzub, "zzub_plugin_get_pattern", POINTER(zzub_pattern_t), ("plugin", POINTER(zzub_plugin_t)), ("index",c_int))
zzub_plugin_get_pattern_by_name = dlsym(libzzub, "zzub_plugin_get_pattern_by_name", c_int, ("plugin", POINTER(zzub_plugin_t)), ("name",c_char_p))
zzub_plugin_get_pattern_name = dlsym(libzzub, "zzub_plugin_get_pattern_name", c_char_p, ("plugin", POINTER(zzub_plugin_t)), ("index",c_int))
zzub_plugin_set_pattern_name = dlsym(libzzub, "zzub_plugin_set_pattern_name", None, ("plugin", POINTER(zzub_plugin_t)), ("index",c_int), ("name",c_char_p))
zzub_plugin_get_pattern_length = dlsym(libzzub, "zzub_plugin_get_pattern_length", c_int, ("plugin", POINTER(zzub_plugin_t)), ("index",c_int))
zzub_plugin_set_pattern_length = dlsym(libzzub, "zzub_plugin_set_pattern_length", None, ("plugin", POINTER(zzub_plugin_t)), ("index",c_int), ("rows",c_int))
zzub_plugin_get_pattern_value = dlsym(libzzub, "zzub_plugin_get_pattern_value", c_int, ("plugin", POINTER(zzub_plugin_t)), ("pattern",c_int), ("group",c_int), ("track",c_int), ("column",c_int), ("row",c_int))
zzub_plugin_set_pattern_value = dlsym(libzzub, "zzub_plugin_set_pattern_value", None, ("plugin", POINTER(zzub_plugin_t)), ("pattern",c_int), ("group",c_int), ("track",c_int), ("column",c_int), ("row",c_int), ("value",c_int))
zzub_plugin_get_new_pattern_name = dlsym(libzzub, "zzub_plugin_get_new_pattern_name", None, ("plugin", POINTER(zzub_plugin_t)), ("name",c_char_p), ("maxLen",c_int))
zzub_plugin_linear_to_pattern = dlsym(libzzub, "zzub_plugin_linear_to_pattern", c_int, ("plugin", POINTER(zzub_plugin_t)), ("index",c_int), ("group",POINTER(c_int)), ("track",POINTER(c_int)), ("column",POINTER(c_int)))
zzub_plugin_pattern_to_linear = dlsym(libzzub, "zzub_plugin_pattern_to_linear", c_int, ("plugin", POINTER(zzub_plugin_t)), ("group",c_int), ("track",c_int), ("column",c_int), ("index",POINTER(c_int)))
zzub_plugin_get_pattern_column_count = dlsym(libzzub, "zzub_plugin_get_pattern_column_count", c_int, ("plugin", POINTER(zzub_plugin_t)))
zzub_plugin_insert_pattern_rows = dlsym(libzzub, "zzub_plugin_insert_pattern_rows", None, ("plugin", POINTER(zzub_plugin_t)), ("pattern",c_int), ("column_indices",POINTER(c_int)), ("num_indices",c_int), ("start",c_int), ("rows",c_int))
zzub_plugin_remove_pattern_rows = dlsym(libzzub, "zzub_plugin_remove_pattern_rows", None, ("plugin", POINTER(zzub_plugin_t)), ("pattern",c_int), ("column_indices",POINTER(c_int)), ("num_indices",c_int), ("start",c_int), ("rows",c_int))
zzub_plugin_describe_value = dlsym(libzzub, "zzub_plugin_describe_value", c_int, ("plugin", POINTER(zzub_plugin_t)), ("group",c_int), ("column",c_int), ("value",c_int), ("name",c_char_p), ("maxlen",c_int))
zzub_plugin_get_parameter_value = dlsym(libzzub, "zzub_plugin_get_parameter_value", c_int, ("plugin", POINTER(zzub_plugin_t)), ("group",c_int), ("track",c_int), ("column",c_int))
zzub_plugin_set_parameter_value = dlsym(libzzub, "zzub_plugin_set_parameter_value", None, ("plugin", POINTER(zzub_plugin_t)), ("group",c_int), ("track",c_int), ("column",c_int), ("value",c_int), ("record",c_int))
zzub_plugin_set_parameter_value_direct = dlsym(libzzub, "zzub_plugin_set_parameter_value_direct", None, ("plugin", POINTER(zzub_plugin_t)), ("group",c_int), ("track",c_int), ("column",c_int), ("value",c_int), ("record",c_int))
zzub_plugin_get_parameter_count = dlsym(libzzub, "zzub_plugin_get_parameter_count", c_int, ("plugin", POINTER(zzub_plugin_t)), ("group",c_int), ("track",c_int))
zzub_plugin_get_parameter = dlsym(libzzub, "zzub_plugin_get_parameter", POINTER(zzub_parameter_t), ("plugin", POINTER(zzub_plugin_t)), ("group",c_int), ("track",c_int), ("column",c_int))
zzub_plugin_get_input_connection_count = dlsym(libzzub, "zzub_plugin_get_input_connection_count", c_int, ("plugin", POINTER(zzub_plugin_t)))
zzub_plugin_get_input_connection_by_type = dlsym(libzzub, "zzub_plugin_get_input_connection_by_type", c_int, ("plugin", POINTER(zzub_plugin_t)), ("from_plugin",POINTER(zzub_plugin_t)), ("type",c_int))
zzub_plugin_get_input_connection_type = dlsym(libzzub, "zzub_plugin_get_input_connection_type", c_int, ("plugin", POINTER(zzub_plugin_t)), ("index",c_int))
zzub_plugin_get_input_connection_plugin = dlsym(libzzub, "zzub_plugin_get_input_connection_plugin", POINTER(zzub_plugin_t), ("plugin", POINTER(zzub_plugin_t)), ("index",c_int))
zzub_plugin_get_output_connection_count = dlsym(libzzub, "zzub_plugin_get_output_connection_count", c_int, ("plugin", POINTER(zzub_plugin_t)))
zzub_plugin_get_output_connection_by_type = dlsym(libzzub, "zzub_plugin_get_output_connection_by_type", c_int, ("plugin", POINTER(zzub_plugin_t)), ("from_plugin",POINTER(zzub_plugin_t)), ("type",c_int))
zzub_plugin_get_output_connection_type = dlsym(libzzub, "zzub_plugin_get_output_connection_type", c_int, ("plugin", POINTER(zzub_plugin_t)), ("index",c_int))
zzub_plugin_get_output_connection_plugin = dlsym(libzzub, "zzub_plugin_get_output_connection_plugin", POINTER(zzub_plugin_t), ("plugin", POINTER(zzub_plugin_t)), ("index",c_int))
zzub_plugin_add_input = dlsym(libzzub, "zzub_plugin_add_input", c_int, ("plugin", POINTER(zzub_plugin_t)), ("from_plugin",POINTER(zzub_plugin_t)), ("type",c_int))
zzub_plugin_delete_input = dlsym(libzzub, "zzub_plugin_delete_input", None, ("plugin", POINTER(zzub_plugin_t)), ("from_plugin",POINTER(zzub_plugin_t)), ("type",c_int))
zzub_plugin_get_mixbuffer = dlsym(libzzub, "zzub_plugin_get_mixbuffer", c_int, ("plugin", POINTER(zzub_plugin_t)), ("leftbuffer",POINTER(c_float)), ("rightbuffer",POINTER(c_float)), ("size",POINTER(c_int)), ("samplepos",POINTER(c_longlong)))
zzub_plugin_get_last_peak = dlsym(libzzub, "zzub_plugin_get_last_peak", None, ("plugin", POINTER(zzub_plugin_t)), ("maxL",POINTER(c_float)), ("maxR",POINTER(c_float)))
zzub_plugin_get_last_worktime = dlsym(libzzub, "zzub_plugin_get_last_worktime", c_double, ("plugin", POINTER(zzub_plugin_t)))
zzub_plugin_get_last_cpu_load = dlsym(libzzub, "zzub_plugin_get_last_cpu_load", c_double, ("plugin", POINTER(zzub_plugin_t)))
zzub_plugin_get_last_midi_result = dlsym(libzzub, "zzub_plugin_get_last_midi_result", c_int, ("plugin", POINTER(zzub_plugin_t)))
zzub_plugin_get_last_audio_result = dlsym(libzzub, "zzub_plugin_get_last_audio_result", c_int, ("plugin", POINTER(zzub_plugin_t)))
zzub_plugin_invoke_event = dlsym(libzzub, "zzub_plugin_invoke_event", c_int, ("plugin", POINTER(zzub_plugin_t)), ("data",POINTER(zzub_event_data_t)), ("immediate",c_int))
zzub_plugin_tick = dlsym(libzzub, "zzub_plugin_tick", None, ("plugin", POINTER(zzub_plugin_t)))
zzub_plugin_get_attribute_value = dlsym(libzzub, "zzub_plugin_get_attribute_value", c_int, ("plugin", POINTER(zzub_plugin_t)), ("index",c_int))
zzub_plugin_set_attribute_value = dlsym(libzzub, "zzub_plugin_set_attribute_value", None, ("plugin", POINTER(zzub_plugin_t)), ("index",c_int), ("value",c_int))
zzub_plugin_play_midi_note = dlsym(libzzub, "zzub_plugin_play_midi_note", None, ("plugin", POINTER(zzub_plugin_t)), ("note",c_int), ("prevNote",c_int), ("velocity",c_int))
zzub_plugin_play_pattern_row_ref = dlsym(libzzub, "zzub_plugin_play_pattern_row_ref", None, ("plugin", POINTER(zzub_plugin_t)), ("pattern",c_int), ("row",c_int))
zzub_plugin_play_pattern_row = dlsym(libzzub, "zzub_plugin_play_pattern_row", None, ("plugin", POINTER(zzub_plugin_t)), ("pattern",POINTER(zzub_pattern_t)), ("row",c_int))
zzub_plugin_set_midi_connection_device = dlsym(libzzub, "zzub_plugin_set_midi_connection_device", c_int, ("plugin", POINTER(zzub_plugin_t)), ("from_plugin",POINTER(zzub_plugin_t)), ("name",c_char_p))
zzub_plugin_add_event_connection_binding = dlsym(libzzub, "zzub_plugin_add_event_connection_binding", None, ("plugin", POINTER(zzub_plugin_t)), ("from_plugin",POINTER(zzub_plugin_t)), ("sourceparam",c_int), ("targetgroup",c_int), ("targettrack",c_int), ("targetparam",c_int))
zzub_sequence_destroy = dlsym(libzzub, "zzub_sequence_destroy", None, ("sequence", POINTER(zzub_sequence_t)))
zzub_sequence_move = dlsym(libzzub, "zzub_sequence_move", None, ("sequence", POINTER(zzub_sequence_t)), ("newIndex",c_int))
zzub_sequence_insert_events = dlsym(libzzub, "zzub_sequence_insert_events", c_int, ("sequence", POINTER(zzub_sequence_t)), ("start",c_int), ("ticks",c_int))
zzub_sequence_remove_events = dlsym(libzzub, "zzub_sequence_remove_events", c_int, ("sequence", POINTER(zzub_sequence_t)), ("start",c_int), ("ticks",c_int))
zzub_sequence_set_event = dlsym(libzzub, "zzub_sequence_set_event", None, ("sequence", POINTER(zzub_sequence_t)), ("pos",c_int), ("value",c_int))
zzub_sequence_get_plugin = dlsym(libzzub, "zzub_sequence_get_plugin", POINTER(zzub_plugin_t), ("sequence", POINTER(zzub_sequence_t)))
zzub_sequence_get_event_at = dlsym(libzzub, "zzub_sequence_get_event_at", c_int, ("sequence", POINTER(zzub_sequence_t)), ("pos",c_int))
zzub_sequence_get_event_count = dlsym(libzzub, "zzub_sequence_get_event_count", c_int, ("sequence", POINTER(zzub_sequence_t)))
zzub_sequence_get_event = dlsym(libzzub, "zzub_sequence_get_event", c_int, ("sequence", POINTER(zzub_sequence_t)), ("index",c_int), ("pos",POINTER(c_int)), ("value",POINTER(c_int)))
zzub_sequence_get_type = dlsym(libzzub, "zzub_sequence_get_type", c_int, ("sequence", POINTER(zzub_sequence_t)))
zzub_wavelevel_get_wave = dlsym(libzzub, "zzub_wavelevel_get_wave", POINTER(zzub_wave_t), ("wavelevel", POINTER(zzub_wavelevel_t)))
zzub_wavelevel_clear = dlsym(libzzub, "zzub_wavelevel_clear", c_int, ("wavelevel", POINTER(zzub_wavelevel_t)))
zzub_wavelevel_get_sample_count = dlsym(libzzub, "zzub_wavelevel_get_sample_count", c_int, ("wavelevel", POINTER(zzub_wavelevel_t)))
zzub_wavelevel_set_sample_count = dlsym(libzzub, "zzub_wavelevel_set_sample_count", None, ("wavelevel", POINTER(zzub_wavelevel_t)), ("count",c_int))
zzub_wavelevel_get_root_note = dlsym(libzzub, "zzub_wavelevel_get_root_note", c_int, ("wavelevel", POINTER(zzub_wavelevel_t)))
zzub_wavelevel_set_root_note = dlsym(libzzub, "zzub_wavelevel_set_root_note", None, ("wavelevel", POINTER(zzub_wavelevel_t)), ("note",c_int))
zzub_wavelevel_get_samples_per_second = dlsym(libzzub, "zzub_wavelevel_get_samples_per_second", c_int, ("wavelevel", POINTER(zzub_wavelevel_t)))
zzub_wavelevel_set_samples_per_second = dlsym(libzzub, "zzub_wavelevel_set_samples_per_second", None, ("wavelevel", POINTER(zzub_wavelevel_t)), ("sps",c_int))
zzub_wavelevel_get_loop_start = dlsym(libzzub, "zzub_wavelevel_get_loop_start", c_int, ("wavelevel", POINTER(zzub_wavelevel_t)))
zzub_wavelevel_set_loop_start = dlsym(libzzub, "zzub_wavelevel_set_loop_start", None, ("wavelevel", POINTER(zzub_wavelevel_t)), ("pos",c_int))
zzub_wavelevel_get_loop_end = dlsym(libzzub, "zzub_wavelevel_get_loop_end", c_int, ("wavelevel", POINTER(zzub_wavelevel_t)))
zzub_wavelevel_set_loop_end = dlsym(libzzub, "zzub_wavelevel_set_loop_end", None, ("wavelevel", POINTER(zzub_wavelevel_t)), ("pos",c_int))
zzub_wavelevel_get_format = dlsym(libzzub, "zzub_wavelevel_get_format", c_int, ("wavelevel", POINTER(zzub_wavelevel_t)))
zzub_wavelevel_remove_sample_range = dlsym(libzzub, "zzub_wavelevel_remove_sample_range", None, ("wavelevel", POINTER(zzub_wavelevel_t)), ("start",c_int), ("end",c_int))
zzub_wavelevel_xfade = dlsym(libzzub, "zzub_wavelevel_xfade", None, ("wavelevel", POINTER(zzub_wavelevel_t)), ("start",c_int), ("end",c_int))
zzub_wavelevel_normalize = dlsym(libzzub, "zzub_wavelevel_normalize", None, ("wavelevel", POINTER(zzub_wavelevel_t)))
zzub_wavelevel_get_samples_digest = dlsym(libzzub, "zzub_wavelevel_get_samples_digest", None, ("wavelevel", POINTER(zzub_wavelevel_t)), ("channel",c_int), ("start",c_int), ("end",c_int), ("mindigest",POINTER(c_float)), ("maxdigest",POINTER(c_float)), ("ampdigest",POINTER(c_float)), ("digestsize",c_int))
zzub_envelope_get_attack = dlsym(libzzub, "zzub_envelope_get_attack", c_ushort, ("envelope", POINTER(zzub_envelope_t)))
zzub_envelope_get_decay = dlsym(libzzub, "zzub_envelope_get_decay", c_ushort, ("envelope", POINTER(zzub_envelope_t)))
zzub_envelope_get_sustain = dlsym(libzzub, "zzub_envelope_get_sustain", c_ushort, ("envelope", POINTER(zzub_envelope_t)))
zzub_envelope_get_release = dlsym(libzzub, "zzub_envelope_get_release", c_ushort, ("envelope", POINTER(zzub_envelope_t)))
zzub_envelope_set_attack = dlsym(libzzub, "zzub_envelope_set_attack", None, ("envelope", POINTER(zzub_envelope_t)), ("attack",c_ushort))
zzub_envelope_set_decay = dlsym(libzzub, "zzub_envelope_set_decay", None, ("envelope", POINTER(zzub_envelope_t)), ("decay",c_ushort))
zzub_envelope_set_sustain = dlsym(libzzub, "zzub_envelope_set_sustain", None, ("envelope", POINTER(zzub_envelope_t)), ("sustain",c_ushort))
zzub_envelope_set_release = dlsym(libzzub, "zzub_envelope_set_release", None, ("envelope", POINTER(zzub_envelope_t)), ("release",c_ushort))
zzub_envelope_get_subdivision = dlsym(libzzub, "zzub_envelope_get_subdivision", c_byte, ("envelope", POINTER(zzub_envelope_t)))
zzub_envelope_set_subdivision = dlsym(libzzub, "zzub_envelope_set_subdivision", None, ("envelope", POINTER(zzub_envelope_t)), ("subdiv",c_byte))
zzub_envelope_get_flags = dlsym(libzzub, "zzub_envelope_get_flags", c_byte, ("envelope", POINTER(zzub_envelope_t)))
zzub_envelope_set_flags = dlsym(libzzub, "zzub_envelope_set_flags", None, ("envelope", POINTER(zzub_envelope_t)), ("flags",c_byte))
zzub_envelope_is_enabled = dlsym(libzzub, "zzub_envelope_is_enabled", c_int, ("envelope", POINTER(zzub_envelope_t)))
zzub_envelope_enable = dlsym(libzzub, "zzub_envelope_enable", None, ("envelope", POINTER(zzub_envelope_t)), ("enable",c_int))
zzub_envelope_get_point_count = dlsym(libzzub, "zzub_envelope_get_point_count", c_int, ("envelope", POINTER(zzub_envelope_t)))
zzub_envelope_get_point = dlsym(libzzub, "zzub_envelope_get_point", None, ("envelope", POINTER(zzub_envelope_t)), ("index",c_int), ("x",POINTER(c_ushort)), ("y",POINTER(c_ushort)), ("flags",POINTER(c_byte)))
zzub_envelope_set_point = dlsym(libzzub, "zzub_envelope_set_point", None, ("envelope", POINTER(zzub_envelope_t)), ("index",c_int), ("x",c_ushort), ("y",c_ushort), ("flags",c_byte))
zzub_envelope_insert_point = dlsym(libzzub, "zzub_envelope_insert_point", None, ("envelope", POINTER(zzub_envelope_t)), ("index",c_int))
zzub_envelope_delete_point = dlsym(libzzub, "zzub_envelope_delete_point", None, ("envelope", POINTER(zzub_envelope_t)), ("index",c_int))
zzub_wave_get_index = dlsym(libzzub, "zzub_wave_get_index", c_int, ("wave", POINTER(zzub_wave_t)))
zzub_wave_load_sample = dlsym(libzzub, "zzub_wave_load_sample", c_int, ("wave", POINTER(zzub_wave_t)), ("level",c_int), ("offset",c_int), ("clear",c_int), ("path",c_char_p), ("datastream",POINTER(zzub_input_t)))
zzub_wave_save_sample = dlsym(libzzub, "zzub_wave_save_sample", c_int, ("wave", POINTER(zzub_wave_t)), ("level",c_int), ("datastream",POINTER(zzub_output_t)))
zzub_wave_save_sample_range = dlsym(libzzub, "zzub_wave_save_sample_range", c_int, ("wave", POINTER(zzub_wave_t)), ("level",c_int), ("datastream",POINTER(zzub_output_t)), ("start",c_int), ("end",c_int))
zzub_wave_clear = dlsym(libzzub, "zzub_wave_clear", c_int, ("wave", POINTER(zzub_wave_t)))
zzub_wave_get_name = dlsym(libzzub, "zzub_wave_get_name", c_char_p, ("wave", POINTER(zzub_wave_t)))
zzub_wave_set_name = dlsym(libzzub, "zzub_wave_set_name", None, ("wave", POINTER(zzub_wave_t)), ("name",c_char_p))
zzub_wave_get_path = dlsym(libzzub, "zzub_wave_get_path", c_char_p, ("wave", POINTER(zzub_wave_t)))
zzub_wave_set_path = dlsym(libzzub, "zzub_wave_set_path", None, ("wave", POINTER(zzub_wave_t)), ("path",c_char_p))
zzub_wave_get_flags = dlsym(libzzub, "zzub_wave_get_flags", c_int, ("wave", POINTER(zzub_wave_t)))
zzub_wave_set_flags = dlsym(libzzub, "zzub_wave_set_flags", None, ("wave", POINTER(zzub_wave_t)), ("flags",c_int))
zzub_wave_get_volume = dlsym(libzzub, "zzub_wave_get_volume", c_float, ("wave", POINTER(zzub_wave_t)))
zzub_wave_set_volume = dlsym(libzzub, "zzub_wave_set_volume", None, ("wave", POINTER(zzub_wave_t)), ("volume",c_float))
zzub_wave_get_envelope_count = dlsym(libzzub, "zzub_wave_get_envelope_count", c_int, ("wave", POINTER(zzub_wave_t)))
zzub_wave_set_envelope_count = dlsym(libzzub, "zzub_wave_set_envelope_count", None, ("wave", POINTER(zzub_wave_t)), ("count",c_int))
zzub_wave_get_envelope = dlsym(libzzub, "zzub_wave_get_envelope", POINTER(zzub_envelope_t), ("wave", POINTER(zzub_wave_t)), ("index",c_int))
zzub_wave_set_envelope = dlsym(libzzub, "zzub_wave_set_envelope", None, ("wave", POINTER(zzub_wave_t)), ("index",c_int), ("env",POINTER(zzub_envelope_t)))
zzub_wave_get_level_count = dlsym(libzzub, "zzub_wave_get_level_count", c_int, ("wave", POINTER(zzub_wave_t)))
zzub_wave_get_level = dlsym(libzzub, "zzub_wave_get_level", POINTER(zzub_wavelevel_t), ("wave", POINTER(zzub_wave_t)), ("index",c_int))
zzub_player_create = dlsym(libzzub, "zzub_player_create", POINTER(zzub_player_t))
zzub_player_destroy = dlsym(libzzub, "zzub_player_destroy", None, ("player", POINTER(zzub_player_t)))
zzub_player_add_plugin_path = dlsym(libzzub, "zzub_player_add_plugin_path", None, ("player", POINTER(zzub_player_t)), ("path",c_char_p))
zzub_player_blacklist_plugin = dlsym(libzzub, "zzub_player_blacklist_plugin", None, ("player", POINTER(zzub_player_t)), ("uri",c_char_p))
zzub_player_initialize = dlsym(libzzub, "zzub_player_initialize", c_int, ("player", POINTER(zzub_player_t)), ("samplesPerSecond",c_int))
zzub_player_load_bmx = dlsym(libzzub, "zzub_player_load_bmx", c_int, ("player", POINTER(zzub_player_t)), ("datastream",POINTER(zzub_input_t)), ("messages",c_char_p), ("maxLen",c_int), ("flags",c_int), ("x",c_float), ("y",c_float))
zzub_player_save_bmx = dlsym(libzzub, "zzub_player_save_bmx", c_int, ("player", POINTER(zzub_player_t)), ("plugins",POINTER(POINTER(zzub_plugin_t))), ("num_plugins",c_int), ("save_waves",c_int), ("datastream",POINTER(zzub_output_t)))
zzub_player_load_ccm = dlsym(libzzub, "zzub_player_load_ccm", c_int, ("player", POINTER(zzub_player_t)), ("fileName",c_char_p))
zzub_player_save_ccm = dlsym(libzzub, "zzub_player_save_ccm", c_int, ("player", POINTER(zzub_player_t)), ("fileName",c_char_p))
zzub_player_get_state = dlsym(libzzub, "zzub_player_get_state", c_int, ("player", POINTER(zzub_player_t)))
zzub_player_set_state = dlsym(libzzub, "zzub_player_set_state", None, ("player", POINTER(zzub_player_t)), ("state",c_int))
zzub_player_set_position = dlsym(libzzub, "zzub_player_set_position", None, ("player", POINTER(zzub_player_t)), ("tick",c_int))
zzub_player_get_bpm = dlsym(libzzub, "zzub_player_get_bpm", c_float, ("player", POINTER(zzub_player_t)))
zzub_player_get_tpb = dlsym(libzzub, "zzub_player_get_tpb", c_int, ("player", POINTER(zzub_player_t)))
zzub_player_set_bpm = dlsym(libzzub, "zzub_player_set_bpm", None, ("player", POINTER(zzub_player_t)), ("bpm",c_float))
zzub_player_set_tpb = dlsym(libzzub, "zzub_player_set_tpb", None, ("player", POINTER(zzub_player_t)), ("tpb",c_int))
zzub_player_get_pluginloader_count = dlsym(libzzub, "zzub_player_get_pluginloader_count", c_int, ("player", POINTER(zzub_player_t)))
zzub_player_get_pluginloader = dlsym(libzzub, "zzub_player_get_pluginloader", POINTER(zzub_pluginloader_t), ("player", POINTER(zzub_player_t)), ("index",c_int))
zzub_player_get_pluginloader_by_name = dlsym(libzzub, "zzub_player_get_pluginloader_by_name", POINTER(zzub_pluginloader_t), ("player", POINTER(zzub_player_t)), ("name",c_char_p))
zzub_player_get_plugin_count = dlsym(libzzub, "zzub_player_get_plugin_count", c_int, ("player", POINTER(zzub_player_t)))
zzub_player_add_midimapping = dlsym(libzzub, "zzub_player_add_midimapping", POINTER(zzub_midimapping_t), ("plugin",POINTER(zzub_plugin_t)), ("group",c_int), ("track",c_int), ("param",c_int), ("channel",c_int), ("controller",c_int))
zzub_player_remove_midimapping = dlsym(libzzub, "zzub_player_remove_midimapping", c_int, ("plugin",POINTER(zzub_plugin_t)), ("group",c_int), ("track",c_int), ("param",c_int))
zzub_player_get_plugin_by_name = dlsym(libzzub, "zzub_player_get_plugin_by_name", POINTER(zzub_plugin_t), ("player", POINTER(zzub_player_t)), ("name",c_char_p))
zzub_player_get_plugin_by_id = dlsym(libzzub, "zzub_player_get_plugin_by_id", POINTER(zzub_plugin_t), ("player", POINTER(zzub_player_t)), ("id",c_int))
zzub_player_get_plugin = dlsym(libzzub, "zzub_player_get_plugin", POINTER(zzub_plugin_t), ("player", POINTER(zzub_player_t)), ("index",c_int))
zzub_player_work_stereo = dlsym(libzzub, "zzub_player_work_stereo", POINTER(POINTER(c_float)), ("player", POINTER(zzub_player_t)), ("numSamples",POINTER(c_int)))
zzub_player_clear = dlsym(libzzub, "zzub_player_clear", None, ("player", POINTER(zzub_player_t)))
zzub_player_get_position = dlsym(libzzub, "zzub_player_get_position", c_int, ("player", POINTER(zzub_player_t)))
zzub_player_set_position = dlsym(libzzub, "zzub_player_set_position", None, ("player", POINTER(zzub_player_t)), ("pos",c_int))
zzub_player_get_loop_start = dlsym(libzzub, "zzub_player_get_loop_start", c_int, ("player", POINTER(zzub_player_t)))
zzub_player_get_loop_end = dlsym(libzzub, "zzub_player_get_loop_end", c_int, ("player", POINTER(zzub_player_t)))
zzub_player_set_loop_start = dlsym(libzzub, "zzub_player_set_loop_start", None, ("player", POINTER(zzub_player_t)), ("v",c_int))
zzub_player_set_loop_end = dlsym(libzzub, "zzub_player_set_loop_end", None, ("player", POINTER(zzub_player_t)), ("v",c_int))
zzub_player_get_loop = dlsym(libzzub, "zzub_player_get_loop", None, ("player", POINTER(zzub_player_t)), ("begin",POINTER(c_int)), ("end",POINTER(c_int)))
zzub_player_set_loop = dlsym(libzzub, "zzub_player_set_loop", None, ("player", POINTER(zzub_player_t)), ("begin",c_int), ("end",c_int))
zzub_player_get_song_start = dlsym(libzzub, "zzub_player_get_song_start", c_int, ("player", POINTER(zzub_player_t)))
zzub_player_get_song_end = dlsym(libzzub, "zzub_player_get_song_end", c_int, ("player", POINTER(zzub_player_t)))
zzub_player_set_song_start = dlsym(libzzub, "zzub_player_set_song_start", None, ("player", POINTER(zzub_player_t)), ("v",c_int))
zzub_player_set_song_end = dlsym(libzzub, "zzub_player_set_song_end", None, ("player", POINTER(zzub_player_t)), ("v",c_int))
zzub_player_set_loop_enabled = dlsym(libzzub, "zzub_player_set_loop_enabled", None, ("player", POINTER(zzub_player_t)), ("enable",c_int))
zzub_player_get_loop_enabled = dlsym(libzzub, "zzub_player_get_loop_enabled", c_int, ("player", POINTER(zzub_player_t)))
zzub_player_get_sequence_track_count = dlsym(libzzub, "zzub_player_get_sequence_track_count", c_int, ("player", POINTER(zzub_player_t)))
zzub_player_get_sequence = dlsym(libzzub, "zzub_player_get_sequence", POINTER(zzub_sequence_t), ("player", POINTER(zzub_player_t)), ("index",c_int))
zzub_player_get_currently_playing_pattern = dlsym(libzzub, "zzub_player_get_currently_playing_pattern", c_int, ("plugin",POINTER(zzub_plugin_t)), ("pattern",POINTER(c_int)), ("row",POINTER(c_int)))
zzub_player_get_currently_playing_pattern_row = dlsym(libzzub, "zzub_player_get_currently_playing_pattern_row", c_int, ("plugin",POINTER(zzub_plugin_t)), ("pattern",c_int), ("row",POINTER(c_int)))
zzub_player_get_wave_count = dlsym(libzzub, "zzub_player_get_wave_count", c_int, ("player", POINTER(zzub_player_t)))
zzub_player_get_wave = dlsym(libzzub, "zzub_player_get_wave", POINTER(zzub_wave_t), ("player", POINTER(zzub_player_t)), ("index",c_int))
zzub_player_get_next_event = dlsym(libzzub, "zzub_player_get_next_event", POINTER(zzub_event_data_t), ("player", POINTER(zzub_player_t)))
zzub_player_set_callback = dlsym(libzzub, "zzub_player_set_callback", None, ("player", POINTER(zzub_player_t)), ("callback",zzub_callback_t), ("tag",c_void_p))
zzub_player_handle_events = dlsym(libzzub, "zzub_player_handle_events", None, ("player", POINTER(zzub_player_t)))
zzub_player_set_event_queue_state = dlsym(libzzub, "zzub_player_set_event_queue_state", None, ("player", POINTER(zzub_player_t)), ("enable",c_int))
zzub_player_get_midimapping = dlsym(libzzub, "zzub_player_get_midimapping", POINTER(zzub_midimapping_t), ("player", POINTER(zzub_player_t)), ("index",c_int))
zzub_player_get_midimapping_count = dlsym(libzzub, "zzub_player_get_midimapping_count", c_int, ("player", POINTER(zzub_player_t)))
zzub_player_get_automation = dlsym(libzzub, "zzub_player_get_automation", c_int, ("player", POINTER(zzub_player_t)))
zzub_player_set_automation = dlsym(libzzub, "zzub_player_set_automation", None, ("player", POINTER(zzub_player_t)), ("enable",c_int))
zzub_player_get_midi_transport = dlsym(libzzub, "zzub_player_get_midi_transport", c_int, ("player", POINTER(zzub_player_t)))
zzub_player_set_midi_transport = dlsym(libzzub, "zzub_player_set_midi_transport", None, ("player", POINTER(zzub_player_t)), ("enable",c_int))
zzub_player_set_seqstep = dlsym(libzzub, "zzub_player_set_seqstep", None, ("player", POINTER(zzub_player_t)), ("step",c_int))
zzub_player_get_seqstep = dlsym(libzzub, "zzub_player_get_seqstep", c_int, ("player", POINTER(zzub_player_t)))
zzub_player_get_infotext = dlsym(libzzub, "zzub_player_get_infotext", c_char_p, ("player", POINTER(zzub_player_t)))
zzub_player_set_infotext = dlsym(libzzub, "zzub_player_set_infotext", None, ("player", POINTER(zzub_player_t)), ("text",c_char_p))
zzub_player_set_midi_plugin = dlsym(libzzub, "zzub_player_set_midi_plugin", None, ("player", POINTER(zzub_player_t)), ("plugin",POINTER(zzub_plugin_t)))
zzub_player_get_midi_plugin = dlsym(libzzub, "zzub_player_get_midi_plugin", POINTER(zzub_plugin_t), ("player", POINTER(zzub_player_t)))
zzub_player_get_new_plugin_name = dlsym(libzzub, "zzub_player_get_new_plugin_name", None, ("player", POINTER(zzub_player_t)), ("uri",c_char_p), ("name",c_char_p), ("maxLen",c_int))
zzub_player_reset_keyjazz = dlsym(libzzub, "zzub_player_reset_keyjazz", None, ("player", POINTER(zzub_player_t)))
zzub_player_create_plugin = dlsym(libzzub, "zzub_player_create_plugin", POINTER(zzub_plugin_t), ("player", POINTER(zzub_player_t)), ("input",POINTER(zzub_input_t)), ("dataSize",c_int), ("instanceName",c_char_p), ("loader",POINTER(zzub_pluginloader_t)), ("flags",c_int))
zzub_player_create_sequence = dlsym(libzzub, "zzub_player_create_sequence", POINTER(zzub_sequence_t), ("player", POINTER(zzub_player_t)), ("plugin",POINTER(zzub_plugin_t)), ("type",c_int))
zzub_player_flush = dlsym(libzzub, "zzub_player_flush", None, ("player", POINTER(zzub_player_t)), ("redo_event",POINTER(zzub_event_data_t)), ("undo_event",POINTER(zzub_event_data_t)))
zzub_player_undo = dlsym(libzzub, "zzub_player_undo", None, ("player", POINTER(zzub_player_t)))
zzub_player_redo = dlsym(libzzub, "zzub_player_redo", None, ("player", POINTER(zzub_player_t)))
zzub_player_history_commit = dlsym(libzzub, "zzub_player_history_commit", None, ("player", POINTER(zzub_player_t)), ("description",c_char_p))
zzub_player_history_get_uncomitted_operations = dlsym(libzzub, "zzub_player_history_get_uncomitted_operations", c_int, ("player", POINTER(zzub_player_t)))
zzub_player_history_flush_last = dlsym(libzzub, "zzub_player_history_flush_last", None, ("player", POINTER(zzub_player_t)))
zzub_player_history_flush = dlsym(libzzub, "zzub_player_history_flush", None, ("player", POINTER(zzub_player_t)))
zzub_player_history_get_size = dlsym(libzzub, "zzub_player_history_get_size", c_int, ("player", POINTER(zzub_player_t)))
zzub_player_history_get_position = dlsym(libzzub, "zzub_player_history_get_position", c_int, ("player", POINTER(zzub_player_t)))
zzub_player_history_get_description = dlsym(libzzub, "zzub_player_history_get_description", c_char_p, ("player", POINTER(zzub_player_t)), ("position",c_int))
zzub_player_set_host_info = dlsym(libzzub, "zzub_player_set_host_info", None, ("player", POINTER(zzub_player_t)), ("id",c_int), ("version",c_int), ("host_ptr",c_void_p))
EventData = zzub_event_data_t

class Audiodriver(object):
	"""Audio Driver Methods
	Configure and create an audio driver instance."""

	_as_parameter_ = None
	_hash = 0
	
	def __init__(self, handle):
		self._as_parameter_ = handle
		self._hash = cast(self._as_parameter_, c_void_p).value
	
	@classmethod
	def _new_from_handle(cls,handle):
		if not handle:
			return None
		return cls(handle)
	
	def __hash__(self):
		return self._hash
	
	def __eq__(self,other):
		return self._hash == hash(other)
	
	def __ne__(self,other):
		return self._hash != hash(other)
	
	@staticmethod
	def create_portaudio(player):
		"""Create an audio driver that uses the PortAudio API."""
		return Audiodriver._new_from_handle(zzub_audiodriver_create_portaudio(player))
	
	@staticmethod
	def create_rtaudio(player):
		"""Create an audio driver that uses the RtAudio API."""
		return Audiodriver._new_from_handle(zzub_audiodriver_create_rtaudio(player))
	
	@staticmethod
	def create_silent(player, name, out_channels, in_channels, num_rates):
		"""Create a silent, non-processing audio driver that has one device with the specified properties."""
		supported_rates = (c_int*num_rates)()
		_ret_arg = zzub_audiodriver_create_silent(player,name,out_channels,in_channels,supported_rates,num_rates)
		return Audiodriver._new_from_handle(_ret_arg),[v for v in supported_rates]
	
	@staticmethod
	def create(player):
		"""Creates the preferred audio driver."""
		return Audiodriver._new_from_handle(zzub_audiodriver_create(player))
	
	def get_count(self):
		"""Get number of detected input and output audio devices"""
		assert self._as_parameter_
		return zzub_audiodriver_get_count(self)
	
	def get_name(self, index, max_len=1024):
		"""Get name of specified audio device"""
		assert self._as_parameter_
		name = (c_char*max_len)()
		zzub_audiodriver_get_name(self,index,name,max_len)
		return name.value
	
	def get_supported_samplerates(self, index, maxrates):
		assert self._as_parameter_
		result = (c_int*maxrates)()
		_ret_arg = zzub_audiodriver_get_supported_samplerates(self,index,result,maxrates)
		return _ret_arg,[v for v in result]
	
	def get_supported_output_channels(self, index):
		assert self._as_parameter_
		return zzub_audiodriver_get_supported_output_channels(self,index)
	
	def get_supported_input_channels(self, index):
		assert self._as_parameter_
		return zzub_audiodriver_get_supported_input_channels(self,index)
	
	def create_device(self, input_index, output_index):
		"""Create specified audio device."""
		assert self._as_parameter_
		return zzub_audiodriver_create_device(self,input_index,output_index)
	
	def enable(self, state):
		"""Enable or disable current audio driver"""
		assert self._as_parameter_
		zzub_audiodriver_enable(self,state)
	
	def get_enabled(self):
		"""Returns whether current audio driver is enabled or disabled"""
		assert self._as_parameter_
		return zzub_audiodriver_get_enabled(self)
	
	def destroy(self):
		"""Disassociate audio driver and player"""
		assert self._as_parameter_
		zzub_audiodriver_destroy(self)
	
	def destroy_device(self):
		"""De-allocate the current device."""
		assert self._as_parameter_
		zzub_audiodriver_destroy_device(self)
	
	def set_samplerate(self, samplerate):
		"""Set audio driver sample rate"""
		assert self._as_parameter_
		zzub_audiodriver_set_samplerate(self,samplerate)
	
	def get_samplerate(self):
		"""Retreive audio driver sample rate"""
		assert self._as_parameter_
		return zzub_audiodriver_get_samplerate(self)
	
	def set_buffersize(self, buffersize):
		assert self._as_parameter_
		zzub_audiodriver_set_buffersize(self,buffersize)
	
	def get_buffersize(self):
		assert self._as_parameter_
		return zzub_audiodriver_get_buffersize(self)
	
	def get_cpu_load(self):
		assert self._as_parameter_
		return zzub_audiodriver_get_cpu_load(self)
	
	def is_output(self, index):
		assert self._as_parameter_
		return zzub_audiodriver_is_output(self,index)
	
	def is_input(self, index):
		assert self._as_parameter_
		return zzub_audiodriver_is_input(self,index)
	
	def get_master_channel(self):
		assert self._as_parameter_
		return zzub_audiodriver_get_master_channel(self)
	
	def set_master_channel(self, index):
		assert self._as_parameter_
		zzub_audiodriver_set_master_channel(self,index)
	

zzub_audiodriver_t._wrapper_ = Audiodriver

class Mididriver(object):
	"""MIDI Driver Methods
	Open midi devices."""

	_as_parameter_ = None
	_hash = 0
	
	def __init__(self, handle):
		self._as_parameter_ = handle
		self._hash = cast(self._as_parameter_, c_void_p).value
	
	@classmethod
	def _new_from_handle(cls,handle):
		if not handle:
			return None
		return cls(handle)
	
	def __hash__(self):
		return self._hash
	
	def __eq__(self,other):
		return self._hash == hash(other)
	
	def __ne__(self,other):
		return self._hash != hash(other)
	
	@staticmethod
	def get_count(player):
		return zzub_mididriver_get_count(player)
	
	@staticmethod
	def get_name(player, index):
		return zzub_mididriver_get_name(player,index)
	
	@staticmethod
	def is_input(player, index):
		return zzub_mididriver_is_input(player,index)
	
	@staticmethod
	def is_output(player, index):
		return zzub_mididriver_is_output(player,index)
	
	@staticmethod
	def open(player, index):
		return zzub_mididriver_open(player,index)
	
	@staticmethod
	def close_all(player):
		return zzub_mididriver_close_all(player)
	

zzub_mididriver_t._wrapper_ = Mididriver

class Plugincollection(object):
	"""Plugin Collection Methods
	For enumerating and configuring plugin collections."""

	_as_parameter_ = None
	_hash = 0
	
	def __init__(self, handle):
		self._as_parameter_ = handle
		self._hash = cast(self._as_parameter_, c_void_p).value
	
	@classmethod
	def _new_from_handle(cls,handle):
		if not handle:
			return None
		return cls(handle)
	
	def __hash__(self):
		return self._hash
	
	def __eq__(self,other):
		return self._hash == hash(other)
	
	def __ne__(self,other):
		return self._hash != hash(other)
	
	@staticmethod
	def get_by_uri(player, uri):
		return Plugincollection._new_from_handle(zzub_plugincollection_get_by_uri(player,uri))
	
	def configure(self, key, value):
		assert self._as_parameter_
		zzub_plugincollection_configure(self,key,value)
	

zzub_plugincollection_t._wrapper_ = Plugincollection

class Input(object):

	_as_parameter_ = None
	_hash = 0
	
	def __init__(self, handle):
		self._as_parameter_ = handle
		self._hash = cast(self._as_parameter_, c_void_p).value
	
	@classmethod
	def _new_from_handle(cls,handle):
		if not handle:
			return None
		return cls(handle)
	
	def __hash__(self):
		return self._hash
	
	def __eq__(self,other):
		return self._hash == hash(other)
	
	def __ne__(self,other):
		return self._hash != hash(other)
	
	@staticmethod
	def open_file(filename):
		"""Create an input stream that reads from a file."""
		return Input._new_from_handle(zzub_input_open_file(filename))
	
	def destroy(self):
		"""Closes an input stream created with zzub_create_output_XXX."""
		assert self._as_parameter_
		zzub_input_destroy(self)
	
	def read(self, bytes):
		assert self._as_parameter_
		buffer = (c_char*bytes)()
		zzub_input_read(self,buffer,bytes)
		return [v for v in buffer]
	
	def size(self):
		assert self._as_parameter_
		return zzub_input_size(self)
	
	def position(self):
		assert self._as_parameter_
		return zzub_input_position(self)
	
	def seek(self, pos, mode):
		assert self._as_parameter_
		zzub_input_seek(self,pos,mode)
	

zzub_input_t._wrapper_ = Input

class Output(object):

	_as_parameter_ = None
	_hash = 0
	
	def __init__(self, handle):
		self._as_parameter_ = handle
		self._hash = cast(self._as_parameter_, c_void_p).value
	
	@classmethod
	def _new_from_handle(cls,handle):
		if not handle:
			return None
		return cls(handle)
	
	def __hash__(self):
		return self._hash
	
	def __eq__(self,other):
		return self._hash == hash(other)
	
	def __ne__(self,other):
		return self._hash != hash(other)
	
	@staticmethod
	def create_file(filename):
		"""Create an output stream that writes to a file."""
		return Output._new_from_handle(zzub_output_create_file(filename))
	
	def destroy(self):
		"""Closes an output stream created with zzub_create_output_XXX."""
		assert self._as_parameter_
		zzub_output_destroy(self)
	
	def write(self, buffer, bytes):
		assert self._as_parameter_
		buffer = (c_char*bytes)(*buffer)
		zzub_output_write(self,buffer,bytes)
	
	def position(self):
		assert self._as_parameter_
		return zzub_output_position(self)
	
	def seek(self, pos, mode):
		assert self._as_parameter_
		zzub_output_seek(self,pos,mode)
	

zzub_output_t._wrapper_ = Output

class Archive(object):

	_as_parameter_ = None
	_hash = 0
	
	def __init__(self, handle):
		self._as_parameter_ = handle
		self._hash = cast(self._as_parameter_, c_void_p).value
	
	@classmethod
	def _new_from_handle(cls,handle):
		if not handle:
			return None
		return cls(handle)
	
	def __hash__(self):
		return self._hash
	
	def __eq__(self,other):
		return self._hash == hash(other)
	
	def __ne__(self,other):
		return self._hash != hash(other)
	
	@staticmethod
	def create_memory():
		"""Create an in-memory archive of keyed input and output streams."""
		return Archive._new_from_handle(zzub_archive_create_memory())
	
	def get_output(self, path):
		"""Returns an output stream object for writing."""
		assert self._as_parameter_
		return Output._new_from_handle(zzub_archive_get_output(self,path))
	
	def get_input(self, path):
		"""Returns an input stream object for reading."""
		assert self._as_parameter_
		return Input._new_from_handle(zzub_archive_get_input(self,path))
	
	def destroy(self):
		assert self._as_parameter_
		zzub_archive_destroy(self)
	

zzub_archive_t._wrapper_ = Archive

class Midimapping(object):
	"""MIDI Mapping Methods"""

	_as_parameter_ = None
	_hash = 0
	
	def __init__(self, handle):
		self._as_parameter_ = handle
		self._hash = cast(self._as_parameter_, c_void_p).value
	
	@classmethod
	def _new_from_handle(cls,handle):
		if not handle:
			return None
		return cls(handle)
	
	def __hash__(self):
		return self._hash
	
	def __eq__(self,other):
		return self._hash == hash(other)
	
	def __ne__(self,other):
		return self._hash != hash(other)
	
	def get_plugin(self):
		assert self._as_parameter_
		return zzub_midimapping_get_plugin(self)
	
	def get_group(self):
		assert self._as_parameter_
		return zzub_midimapping_get_group(self)
	
	def get_track(self):
		assert self._as_parameter_
		return zzub_midimapping_get_track(self)
	
	def get_column(self):
		assert self._as_parameter_
		return zzub_midimapping_get_column(self)
	
	def get_channel(self):
		assert self._as_parameter_
		return zzub_midimapping_get_channel(self)
	
	def get_controller(self):
		assert self._as_parameter_
		return zzub_midimapping_get_controller(self)
	

zzub_midimapping_t._wrapper_ = Midimapping

class Pattern(object):
	"""Offline pattern methods
	These functions are meant to help editing patterns. Note you cannot
	retreive a direct zzub_pattern_t object for a "live pattern". You can
	however, use zzub_plugin_get_pattern to retreive copies of live patterns,
	and then call zzub_plugin_update_pattern to write the changed pattern back
	to the engine.
	Alternately, zzub_plugin_get_pattern_value/zzub_plugin_set_pattern_value
	can also be used to edit single values in live patterns."""

	_as_parameter_ = None
	_hash = 0
	
	def __init__(self, handle):
		self._as_parameter_ = handle
		self._hash = cast(self._as_parameter_, c_void_p).value
	
	@classmethod
	def _new_from_handle(cls,handle):
		if not handle:
			return None
		return cls(handle)
	
	def __hash__(self):
		return self._hash
	
	def __eq__(self,other):
		return self._hash == hash(other)
	
	def __ne__(self,other):
		return self._hash != hash(other)
	
	def destroy(self):
		assert self._as_parameter_
		zzub_pattern_destroy(self)
	
	def get_name(self, maxLen=1024):
		assert self._as_parameter_
		name = (c_char*maxLen)()
		zzub_pattern_get_name(self,name,maxLen)
		return name.value
	
	def set_name(self, name):
		assert self._as_parameter_
		zzub_pattern_set_name(self,name)
	
	def get_row_count(self):
		assert self._as_parameter_
		return zzub_pattern_get_row_count(self)
	
	def get_group_count(self):
		assert self._as_parameter_
		return zzub_pattern_get_group_count(self)
	
	def get_track_count(self, group):
		assert self._as_parameter_
		return zzub_pattern_get_track_count(self,group)
	
	def get_column_count(self, group, track):
		assert self._as_parameter_
		return zzub_pattern_get_column_count(self,group,track)
	
	def get_value(self, row, group, track, column):
		assert self._as_parameter_
		return zzub_pattern_get_value(self,row,group,track,column)
	
	def set_value(self, row, group, track, column, value):
		assert self._as_parameter_
		zzub_pattern_set_value(self,row,group,track,column,value)
	
	def interpolate(self):
		assert self._as_parameter_
		zzub_pattern_interpolate(self)
	

zzub_pattern_t._wrapper_ = Pattern

class Parameter(object):
	"""Parameter methods
	Retreive more details from zzub_parameter_t objects."""

	_as_parameter_ = None
	_hash = 0
	
	def __init__(self, handle):
		self._as_parameter_ = handle
		self._hash = cast(self._as_parameter_, c_void_p).value
	
	@classmethod
	def _new_from_handle(cls,handle):
		if not handle:
			return None
		return cls(handle)
	
	def __hash__(self):
		return self._hash
	
	def __eq__(self,other):
		return self._hash == hash(other)
	
	def __ne__(self,other):
		return self._hash != hash(other)
	
	def get_type(self):
		"""Returns one of the values in the zzub_parameter_type enumeration."""
		assert self._as_parameter_
		return zzub_parameter_get_type(self)
	
	def get_name(self):
		assert self._as_parameter_
		return zzub_parameter_get_name(self)
	
	def get_description(self):
		assert self._as_parameter_
		return zzub_parameter_get_description(self)
	
	def get_value_min(self):
		assert self._as_parameter_
		return zzub_parameter_get_value_min(self)
	
	def get_value_max(self):
		assert self._as_parameter_
		return zzub_parameter_get_value_max(self)
	
	def get_value_none(self):
		assert self._as_parameter_
		return zzub_parameter_get_value_none(self)
	
	def get_value_default(self):
		assert self._as_parameter_
		return zzub_parameter_get_value_default(self)
	
	def get_flags(self):
		"""A parameter flag is combined by zero or more values in the zzub_parameter_flag enumeration."""
		assert self._as_parameter_
		return zzub_parameter_get_flags(self)
	

zzub_parameter_t._wrapper_ = Parameter

class Attribute(object):
	"""Attribute methods
	Retreive more details from zzub_attribute_t objects."""

	_as_parameter_ = None
	_hash = 0
	
	def __init__(self, handle):
		self._as_parameter_ = handle
		self._hash = cast(self._as_parameter_, c_void_p).value
	
	@classmethod
	def _new_from_handle(cls,handle):
		if not handle:
			return None
		return cls(handle)
	
	def __hash__(self):
		return self._hash
	
	def __eq__(self,other):
		return self._hash == hash(other)
	
	def __ne__(self,other):
		return self._hash != hash(other)
	
	def get_name(self):
		assert self._as_parameter_
		return zzub_attribute_get_name(self)
	
	def get_value_min(self):
		assert self._as_parameter_
		return zzub_attribute_get_value_min(self)
	
	def get_value_max(self):
		assert self._as_parameter_
		return zzub_attribute_get_value_max(self)
	
	def get_value_default(self):
		assert self._as_parameter_
		return zzub_attribute_get_value_default(self)
	

zzub_attribute_t._wrapper_ = Attribute

class Pluginloader(object):
	"""Plugin loading methods
	Retreive more details from zzub_pluginloader_t objects."""

	_as_parameter_ = None
	_hash = 0
	
	def __init__(self, handle):
		self._as_parameter_ = handle
		self._hash = cast(self._as_parameter_, c_void_p).value
	
	@classmethod
	def _new_from_handle(cls,handle):
		if not handle:
			return None
		return cls(handle)
	
	def __hash__(self):
		return self._hash
	
	def __eq__(self,other):
		return self._hash == hash(other)
	
	def __ne__(self,other):
		return self._hash != hash(other)
	
	def get_name(self):
		assert self._as_parameter_
		return zzub_pluginloader_get_name(self)
	
	def get_short_name(self):
		assert self._as_parameter_
		return zzub_pluginloader_get_short_name(self)
	
	def get_parameter_count(self, group):
		assert self._as_parameter_
		return zzub_pluginloader_get_parameter_count(self,group)
	
	def get_parameter(self, group, index):
		"""Returns the parameter for a group and column. See also zzub_plugin_get_parameter() which also returns parameters in group 0."""
		assert self._as_parameter_
		return Parameter._new_from_handle(zzub_pluginloader_get_parameter(self,group,index))
	
	def get_attribute_count(self):
		assert self._as_parameter_
		return zzub_pluginloader_get_attribute_count(self)
	
	def get_attribute(self, index):
		assert self._as_parameter_
		return Attribute._new_from_handle(zzub_pluginloader_get_attribute(self,index))
	
	def get_attribute_list(self):
		for index in xrange(self.get_attribute_count()):
			yield self.get_attribute(index)
	
	def get_loader_name(self):
		assert self._as_parameter_
		return zzub_pluginloader_get_loader_name(self)
	
	def get_flags(self):
		"""Returns the flags for this plugin loader. Combined by zero or more values in the zzub_plugin_flag enumeration."""
		assert self._as_parameter_
		return zzub_pluginloader_get_flags(self)
	
	def get_uri(self):
		assert self._as_parameter_
		return zzub_pluginloader_get_uri(self)
	
	def get_author(self):
		assert self._as_parameter_
		return zzub_pluginloader_get_author(self)
	
	def get_instrument_list(self, maxbytes):
		assert self._as_parameter_
		result = (c_char*maxbytes)()
		_ret_arg = zzub_pluginloader_get_instrument_list(self,result,maxbytes)
		return _ret_arg,[v for v in result]
	
	def get_tracks_min(self):
		assert self._as_parameter_
		return zzub_pluginloader_get_tracks_min(self)
	
	def get_tracks_max(self):
		assert self._as_parameter_
		return zzub_pluginloader_get_tracks_max(self)
	
	def get_stream_format_count(self):
		"""Returns the number of supported stream formats. Used with plugins flagged zzub_plugin_flag_stream."""
		assert self._as_parameter_
		return zzub_pluginloader_get_stream_format_count(self)
	
	def get_stream_format_ext(self, index):
		"""Returns a supported stream file format extension stream. Used with plugins flagged zzub_plugin_flag_stream."""
		assert self._as_parameter_
		return zzub_pluginloader_get_stream_format_ext(self,index)
	

zzub_pluginloader_t._wrapper_ = Pluginloader

class Plugin(object):
	"""Plugin methods
	Retreive more details about plugins."""

	_as_parameter_ = None
	_hash = 0
	
	def __init__(self, handle):
		self._as_parameter_ = handle
		self._hash = cast(self._as_parameter_, c_void_p).value
	
	@classmethod
	def _new_from_handle(cls,handle):
		if not handle:
			return None
		return cls(handle)
	
	def __hash__(self):
		return self._hash
	
	def __eq__(self,other):
		return self._hash == hash(other)
	
	def __ne__(self,other):
		return self._hash != hash(other)
	
	def destroy(self):
		"""Deletes a plugin"""
		assert self._as_parameter_
		return zzub_plugin_destroy(self)
	
	def load(self, input):
		"""Load plugin state."""
		assert self._as_parameter_
		return zzub_plugin_load(self,input)
	
	def save(self, ouput):
		"""Save plugin state."""
		assert self._as_parameter_
		return zzub_plugin_save(self,ouput)
	
	def set_name(self, name):
		"""Renames a plugin. Should fail and return -1 if the name already exists."""
		assert self._as_parameter_
		return zzub_plugin_set_name(self,name)
	
	def get_name(self, maxlen=1024):
		"""Retreive the name of a plugin."""
		assert self._as_parameter_
		name = (c_char*maxlen)()
		zzub_plugin_get_name(self,name,maxlen)
		return name.value
	
	def get_id(self):
		"""Retreive the unique per-session id of a plugin. See also zzub_player_get_plugin_by_id()."""
		assert self._as_parameter_
		return zzub_plugin_get_id(self)
	
	def get_position(self):
		"""Returns the screen position coordinates for the plugin. Values are expected to be in the range -1..1."""
		assert self._as_parameter_
		x = c_float()
		y = c_float()
		zzub_plugin_get_position(self,byref(x),byref(y))
		return x.value,y.value
	
	def set_position(self, x, y):
		"""Sets the plugin screen position. Values are expected to be in the range -1..1."""
		assert self._as_parameter_
		zzub_plugin_set_position(self,x,y)
	
	def set_position_direct(self, x, y):
		"""Sets the plugin screen position. Values are expected to be in the range -1..1. This method is not undoable."""
		assert self._as_parameter_
		zzub_plugin_set_position_direct(self,x,y)
	
	def get_flags(self):
		"""Returns flags for this plugin. Shorthand for using zzub_pluginloader_get_flags(). Combined by zero or more values in the zzub_plugin_flag enumeration."""
		assert self._as_parameter_
		return zzub_plugin_get_flags(self)
	
	def get_track_count(self):
		"""Returns the number of tracks."""
		assert self._as_parameter_
		return zzub_plugin_get_track_count(self)
	
	def set_track_count(self, count):
		"""Sets the number of tracks. Will call plugin::set_track_count() from the player thread."""
		assert self._as_parameter_
		zzub_plugin_set_track_count(self,count)
	
	def get_group_track_count(self, group):
		"""Returns the number of tracks for one of ParameterGroup"""
		assert self._as_parameter_
		return zzub_plugin_get_group_track_count(self,group)
	
	def get_mute(self):
		"""Returns 1 if plugin is muted, otherwise 0."""
		assert self._as_parameter_
		return zzub_plugin_get_mute(self)
	
	def set_mute(self, muted):
		"""Set whether plugin is muted. 1 for muted, 0 for normal.
		A muted machine does not produce any sound."""
		assert self._as_parameter_
		zzub_plugin_set_mute(self,muted)
	
	def get_bypass(self):
		"""Returns 1 if plugin is bypassed, otherwise 0."""
		assert self._as_parameter_
		return zzub_plugin_get_bypass(self)
	
	def configure(self, key, value):
		"""Configure a plugin option. this is e.g. used by the recorder plugin to
		specify a file path to write to."""
		assert self._as_parameter_
		zzub_plugin_configure(self,key,value)
	
	def set_bypass(self, muted):
		"""Set whether plugin is bypassed. 1 for bypass, 0 for normal.
		Bypass causes no processing to occur in the given machine."""
		assert self._as_parameter_
		zzub_plugin_set_bypass(self,muted)
	
	def get_commands(self, maxlen=1024):
		"""Returns a string of \\\
-separated command strings"""
		assert self._as_parameter_
		commands = (c_char*maxlen)()
		zzub_plugin_get_commands(self,commands,maxlen)
		return commands.value
	
	def get_sub_commands(self, i, maxlen=1024):
		"""When a plugin command string starts with the char '\', it has subcommands.
		Unexpectedly, zzub_plugin_get_sub_commands returns a \\\
-separated string (like get_commands).
		Some plugins need to be ticked before calling get_sub_commands."""
		assert self._as_parameter_
		commands = (c_char*maxlen)()
		zzub_plugin_get_sub_commands(self,i,commands,maxlen)
		return commands.value
	
	def command(self, i):
		"""Invoke a command on the plugin."""
		assert self._as_parameter_
		zzub_plugin_command(self,i)
	
	def get_pluginloader(self):
		"""Returns the pluginloader used to create this plugin."""
		assert self._as_parameter_
		return Pluginloader._new_from_handle(zzub_plugin_get_pluginloader(self))
	
	def get_midi_output_device_count(self):
		assert self._as_parameter_
		return zzub_plugin_get_midi_output_device_count(self)
	
	def get_midi_output_device(self, index):
		assert self._as_parameter_
		return zzub_plugin_get_midi_output_device(self,index)
	
	def get_envelope_count(self):
		assert self._as_parameter_
		return zzub_plugin_get_envelope_count(self)
	
	def get_envelope_flags(self, index):
		assert self._as_parameter_
		return zzub_plugin_get_envelope_flags(self,index)
	
	def get_envelope_name(self, index):
		assert self._as_parameter_
		return zzub_plugin_get_envelope_name(self,index)
	
	def set_stream_source(self, resource):
		assert self._as_parameter_
		zzub_plugin_set_stream_source(self,resource)
	
	def set_instrument(self, name):
		"""Sets the plugin instrument (d'oh!)"""
		assert self._as_parameter_
		return zzub_plugin_set_instrument(self,name)
	
	@staticmethod
	def create_range_pattern(player, columns, rows):
		"""Creates a non-playable pattern with given columns and rows in group 0, track 0. All values are set to 0 by default."""
		return Pattern._new_from_handle(zzub_plugin_create_range_pattern(player,columns,rows))
	
	def create_pattern(self, rows):
		"""Creates a pattern compatible with given plugin. The pattern becomes incompatible if the plugin has tracks or incoming connections added."""
		assert self._as_parameter_
		return Pattern._new_from_handle(zzub_plugin_create_pattern(self,rows))
	
	def get_pattern_count(self):
		"""Returns how many patterns are associated with the plugin."""
		assert self._as_parameter_
		return zzub_plugin_get_pattern_count(self)
	
	def add_pattern(self, pattern):
		"""Adds a pattern at the end of the plugins list of patterns"""
		assert self._as_parameter_
		zzub_plugin_add_pattern(self,pattern)
	
	def remove_pattern(self, pattern):
		"""Remove the pattern from the plugin"""
		assert self._as_parameter_
		zzub_plugin_remove_pattern(self,pattern)
	
	def move_pattern(self, index, newIndex):
		"""Change the order of patterns"""
		assert self._as_parameter_
		zzub_plugin_move_pattern(self,index,newIndex)
	
	def update_pattern(self, index, pattern):
		"""Replaces pattern contents """
		assert self._as_parameter_
		zzub_plugin_update_pattern(self,index,pattern)
	
	def get_pattern(self, index):
		"""Returns a copy of the requested pattern. Callers must destroy the pattern returned from get_pattern"""
		assert self._as_parameter_
		return Pattern._new_from_handle(zzub_plugin_get_pattern(self,index))
	
	def get_pattern_list(self):
		for index in xrange(self.get_pattern_count()):
			yield self.get_pattern(index)
	
	def get_pattern_by_name(self, name):
		"""Returns the index of the pattern with the given name"""
		assert self._as_parameter_
		return zzub_plugin_get_pattern_by_name(self,name)
	
	def get_pattern_name(self, index):
		"""Returns the name of given pattern."""
		assert self._as_parameter_
		return zzub_plugin_get_pattern_name(self,index)
	
	def set_pattern_name(self, index, name):
		"""Updates the name of the pattern."""
		assert self._as_parameter_
		zzub_plugin_set_pattern_name(self,index,name)
	
	def get_pattern_length(self, index):
		"""Returns the length of the pattern."""
		assert self._as_parameter_
		return zzub_plugin_get_pattern_length(self,index)
	
	def set_pattern_length(self, index, rows):
		"""Updates the number of rows in the pattern."""
		assert self._as_parameter_
		zzub_plugin_set_pattern_length(self,index,rows)
	
	def get_pattern_value(self, pattern, group, track, column, row):
		"""Returns a value from the requested pattern."""
		assert self._as_parameter_
		return zzub_plugin_get_pattern_value(self,pattern,group,track,column,row)
	
	def set_pattern_value(self, pattern, group, track, column, row, value):
		"""Sets a value in a pattern."""
		assert self._as_parameter_
		zzub_plugin_set_pattern_value(self,pattern,group,track,column,row,value)
	
	def get_new_pattern_name(self, maxLen=1024):
		assert self._as_parameter_
		name = (c_char*maxLen)()
		zzub_plugin_get_new_pattern_name(self,name,maxLen)
		return name.value
	
	def linear_to_pattern(self, index):
		assert self._as_parameter_
		group = c_int()
		track = c_int()
		column = c_int()
		_ret_arg = zzub_plugin_linear_to_pattern(self,index,byref(group),byref(track),byref(column))
		return _ret_arg,group.value,track.value,column.value
	
	def pattern_to_linear(self, group, track, column):
		assert self._as_parameter_
		index = c_int()
		_ret_arg = zzub_plugin_pattern_to_linear(self,group,track,column,byref(index))
		return _ret_arg,index.value
	
	def get_pattern_column_count(self):
		assert self._as_parameter_
		return zzub_plugin_get_pattern_column_count(self)
	
	def insert_pattern_rows(self, pattern, column_indices, num_indices, start, rows):
		"""Inserts rows in a pattern. column_indices has a total length of 3 * num_indices, where each index is a triple of group, track and column."""
		assert self._as_parameter_
		column_indices = (c_int*(num_indices*3))(*column_indices)
		zzub_plugin_insert_pattern_rows(self,pattern,column_indices,num_indices,start,rows)
	
	def remove_pattern_rows(self, pattern, column_indices, num_indices, start, rows):
		"""Removes rows in a pattern. column_indices has a total length of 3 * num_indices, where each index is a triple of group, track and column."""
		assert self._as_parameter_
		column_indices = (c_int*(num_indices*3))(*column_indices)
		zzub_plugin_remove_pattern_rows(self,pattern,column_indices,num_indices,start,rows)
	
	def describe_value(self, group, column, value, maxlen=1024):
		"""Copies columns from an offline pattern to a live pattern. Source and target columns are set up in
		the mappings array, which has 6 ints for each mapping: group, track and column for source and target
		plugins.
		Creates a textual description of the given value. The return value is the number of characters in the output string."""
		assert self._as_parameter_
		name = (c_char*maxlen)()
		zzub_plugin_describe_value(self,group,column,value,name,maxlen)
		return name.value
	
	def get_parameter_value(self, group, track, column):
		"""Returns the last written value of the requested parameter."""
		assert self._as_parameter_
		return zzub_plugin_get_parameter_value(self,group,track,column)
	
	def set_parameter_value(self, group, track, column, value, record):
		"""Sets the value of a plugin parameter. The method will wait for the player thread to pick up the modified value and call process_events()."""
		assert self._as_parameter_
		zzub_plugin_set_parameter_value(self,group,track,column,value,record)
	
	def set_parameter_value_direct(self, group, track, column, value, record):
		"""Sets the value of a plugin parameter. Unlike zzub_plugin_set_parameter_value(), this method returns immediately. The parameter will be changed later when the player thread notices the modified value. Is also not undoable."""
		assert self._as_parameter_
		zzub_plugin_set_parameter_value_direct(self,group,track,column,value,record)
	
	def get_parameter_count(self, group, track):
		assert self._as_parameter_
		return zzub_plugin_get_parameter_count(self,group,track)
	
	def get_parameter(self, group, track, column):
		assert self._as_parameter_
		return Parameter._new_from_handle(zzub_plugin_get_parameter(self,group,track,column))
	
	def get_input_connection_count(self):
		"""Returns the number of input connections for given plugin."""
		assert self._as_parameter_
		return zzub_plugin_get_input_connection_count(self)
	
	def get_input_connection_by_type(self, from_plugin, type):
		"""Returns the input connection index for given plugin and connection type."""
		assert self._as_parameter_
		return zzub_plugin_get_input_connection_by_type(self,from_plugin,type)
	
	def get_input_connection_type(self, index):
		"""Returns the connection type for given plugin and connection index."""
		assert self._as_parameter_
		return zzub_plugin_get_input_connection_type(self,index)
	
	def get_input_connection_plugin(self, index):
		"""Returns the plugin index for given plugin and connection index."""
		assert self._as_parameter_
		return Plugin._new_from_handle(zzub_plugin_get_input_connection_plugin(self,index))
	
	def get_output_connection_count(self):
		"""Returns the number of output connections for given plugin."""
		assert self._as_parameter_
		return zzub_plugin_get_output_connection_count(self)
	
	def get_output_connection_by_type(self, from_plugin, type):
		"""Returns the output connection index for given plugin and connection type."""
		assert self._as_parameter_
		return zzub_plugin_get_output_connection_by_type(self,from_plugin,type)
	
	def get_output_connection_type(self, index):
		"""Returns the connection type for given plugin and connection index."""
		assert self._as_parameter_
		return zzub_plugin_get_output_connection_type(self,index)
	
	def get_output_connection_plugin(self, index):
		"""Returns the plugin index for given plugin and connection index."""
		assert self._as_parameter_
		return Plugin._new_from_handle(zzub_plugin_get_output_connection_plugin(self,index))
	
	def add_input(self, from_plugin, type):
		"""Connect two plugins"""
		assert self._as_parameter_
		return zzub_plugin_add_input(self,from_plugin,type)
	
	def delete_input(self, from_plugin, type):
		"""Disconnect two plugins"""
		assert self._as_parameter_
		zzub_plugin_delete_input(self,from_plugin,type)
	
	def get_mixbuffer(self):
		"""Copies the given plugins work buffer."""
		assert self._as_parameter_
		leftbuffer = (c_float*size)()
		rightbuffer = (c_float*size)()
		size = c_int()
		samplepos = c_longlong()
		_ret_arg = zzub_plugin_get_mixbuffer(self,leftbuffer,rightbuffer,byref(size),byref(samplepos))
		return _ret_arg,[v for v in leftbuffer],[v for v in rightbuffer],size.value,samplepos.value
	
	def get_last_peak(self):
		assert self._as_parameter_
		maxL = c_float()
		maxR = c_float()
		zzub_plugin_get_last_peak(self,byref(maxL),byref(maxR))
		return maxL.value,maxR.value
	
	def get_last_worktime(self):
		assert self._as_parameter_
		return zzub_plugin_get_last_worktime(self)
	
	def get_last_cpu_load(self):
		assert self._as_parameter_
		return zzub_plugin_get_last_cpu_load(self)
	
	def get_last_midi_result(self):
		assert self._as_parameter_
		return zzub_plugin_get_last_midi_result(self)
	
	def get_last_audio_result(self):
		assert self._as_parameter_
		return zzub_plugin_get_last_audio_result(self)
	
	def invoke_event(self, data, immediate):
		assert self._as_parameter_
		return zzub_plugin_invoke_event(self,data,immediate)
	
	def tick(self):
		assert self._as_parameter_
		zzub_plugin_tick(self)
	
	def get_attribute_value(self, index):
		assert self._as_parameter_
		return zzub_plugin_get_attribute_value(self,index)
	
	def set_attribute_value(self, index, value):
		assert self._as_parameter_
		zzub_plugin_set_attribute_value(self,index,value)
	
	def play_midi_note(self, note, prevNote, velocity):
		assert self._as_parameter_
		zzub_plugin_play_midi_note(self,note,prevNote,velocity)
	
	def play_pattern_row_ref(self, pattern, row):
		assert self._as_parameter_
		zzub_plugin_play_pattern_row_ref(self,pattern,row)
	
	def play_pattern_row(self, pattern, row):
		assert self._as_parameter_
		zzub_plugin_play_pattern_row(self,pattern,row)
	
	def set_midi_connection_device(self, from_plugin, name):
		assert self._as_parameter_
		return zzub_plugin_set_midi_connection_device(self,from_plugin,name)
	
	def add_event_connection_binding(self, from_plugin, sourceparam, targetgroup, targettrack, targetparam):
		assert self._as_parameter_
		zzub_plugin_add_event_connection_binding(self,from_plugin,sourceparam,targetgroup,targettrack,targetparam)
	

zzub_plugin_t._wrapper_ = Plugin

class Sequence(object):
	"""Sequencer methods"""

	_as_parameter_ = None
	_hash = 0
	
	def __init__(self, handle):
		self._as_parameter_ = handle
		self._hash = cast(self._as_parameter_, c_void_p).value
	
	@classmethod
	def _new_from_handle(cls,handle):
		if not handle:
			return None
		return cls(handle)
	
	def __hash__(self):
		return self._hash
	
	def __eq__(self,other):
		return self._hash == hash(other)
	
	def __ne__(self,other):
		return self._hash != hash(other)
	
	def destroy(self):
		assert self._as_parameter_
		zzub_sequence_destroy(self)
	
	def move(self, newIndex):
		assert self._as_parameter_
		zzub_sequence_move(self,newIndex)
	
	def insert_events(self, start, ticks):
		assert self._as_parameter_
		return zzub_sequence_insert_events(self,start,ticks)
	
	def remove_events(self, start, ticks):
		assert self._as_parameter_
		return zzub_sequence_remove_events(self,start,ticks)
	
	def set_event(self, pos, value):
		assert self._as_parameter_
		zzub_sequence_set_event(self,pos,value)
	
	def get_plugin(self):
		assert self._as_parameter_
		return Plugin._new_from_handle(zzub_sequence_get_plugin(self))
	
	def get_event_at(self, pos):
		assert self._as_parameter_
		return zzub_sequence_get_event_at(self,pos)
	
	def get_event_count(self):
		assert self._as_parameter_
		return zzub_sequence_get_event_count(self)
	
	def get_event(self, index):
		assert self._as_parameter_
		pos = c_int()
		value = c_int()
		zzub_sequence_get_event(self,index,byref(pos),byref(value))
		return pos.value,value.value
	
	def get_event_list(self):
		for index in xrange(self.get_event_count()):
			yield self.get_event(index)
	
	def get_type(self):
		assert self._as_parameter_
		return zzub_sequence_get_type(self)
	

zzub_sequence_t._wrapper_ = Sequence

class Wavelevel(object):
	"""Wavelevel"""

	_as_parameter_ = None
	_hash = 0
	
	def __init__(self, handle):
		self._as_parameter_ = handle
		self._hash = cast(self._as_parameter_, c_void_p).value
	
	@classmethod
	def _new_from_handle(cls,handle):
		if not handle:
			return None
		return cls(handle)
	
	def __hash__(self):
		return self._hash
	
	def __eq__(self,other):
		return self._hash == hash(other)
	
	def __ne__(self,other):
		return self._hash != hash(other)
	
	def get_wave(self):
		assert self._as_parameter_
		return Wave._new_from_handle(zzub_wavelevel_get_wave(self))
	
	def clear(self):
		assert self._as_parameter_
		return zzub_wavelevel_clear(self)
	
	def get_sample_count(self):
		assert self._as_parameter_
		return zzub_wavelevel_get_sample_count(self)
	
	def set_sample_count(self, count):
		assert self._as_parameter_
		zzub_wavelevel_set_sample_count(self,count)
	
	def get_root_note(self):
		assert self._as_parameter_
		return zzub_wavelevel_get_root_note(self)
	
	def set_root_note(self, note):
		assert self._as_parameter_
		zzub_wavelevel_set_root_note(self,note)
	
	def get_samples_per_second(self):
		assert self._as_parameter_
		return zzub_wavelevel_get_samples_per_second(self)
	
	def set_samples_per_second(self, sps):
		assert self._as_parameter_
		zzub_wavelevel_set_samples_per_second(self,sps)
	
	def get_loop_start(self):
		assert self._as_parameter_
		return zzub_wavelevel_get_loop_start(self)
	
	def set_loop_start(self, pos):
		assert self._as_parameter_
		zzub_wavelevel_set_loop_start(self,pos)
	
	def get_loop_end(self):
		assert self._as_parameter_
		return zzub_wavelevel_get_loop_end(self)
	
	def set_loop_end(self, pos):
		assert self._as_parameter_
		zzub_wavelevel_set_loop_end(self,pos)
	
	def get_format(self):
		assert self._as_parameter_
		return zzub_wavelevel_get_format(self)
	
	def remove_sample_range(self, start, end):
		assert self._as_parameter_
		zzub_wavelevel_remove_sample_range(self,start,end)
	
	def xfade(self, start, end):
		assert self._as_parameter_
		zzub_wavelevel_xfade(self,start,end)
	
	def normalize(self):
		assert self._as_parameter_
		zzub_wavelevel_normalize(self)
	
	def get_samples_digest(self, channel, start, end, digestsize):
		assert self._as_parameter_
		mindigest = (c_float*digestsize)()
		maxdigest = (c_float*digestsize)()
		ampdigest = (c_float*digestsize)()
		zzub_wavelevel_get_samples_digest(self,channel,start,end,mindigest,maxdigest,ampdigest,digestsize)
		return [v for v in mindigest],[v for v in maxdigest],[v for v in ampdigest]
	

zzub_wavelevel_t._wrapper_ = Wavelevel

class Envelope(object):
	"""Envelopes"""

	_as_parameter_ = None
	_hash = 0
	
	def __init__(self, handle):
		self._as_parameter_ = handle
		self._hash = cast(self._as_parameter_, c_void_p).value
	
	@classmethod
	def _new_from_handle(cls,handle):
		if not handle:
			return None
		return cls(handle)
	
	def __hash__(self):
		return self._hash
	
	def __eq__(self,other):
		return self._hash == hash(other)
	
	def __ne__(self,other):
		return self._hash != hash(other)
	
	def get_attack(self):
		assert self._as_parameter_
		return zzub_envelope_get_attack(self)
	
	def get_decay(self):
		assert self._as_parameter_
		return zzub_envelope_get_decay(self)
	
	def get_sustain(self):
		assert self._as_parameter_
		return zzub_envelope_get_sustain(self)
	
	def get_release(self):
		assert self._as_parameter_
		return zzub_envelope_get_release(self)
	
	def set_attack(self, attack):
		assert self._as_parameter_
		zzub_envelope_set_attack(self,attack)
	
	def set_decay(self, decay):
		assert self._as_parameter_
		zzub_envelope_set_decay(self,decay)
	
	def set_sustain(self, sustain):
		assert self._as_parameter_
		zzub_envelope_set_sustain(self,sustain)
	
	def set_release(self, release):
		assert self._as_parameter_
		zzub_envelope_set_release(self,release)
	
	def get_subdivision(self):
		assert self._as_parameter_
		return zzub_envelope_get_subdivision(self)
	
	def set_subdivision(self, subdiv):
		assert self._as_parameter_
		zzub_envelope_set_subdivision(self,subdiv)
	
	def get_flags(self):
		assert self._as_parameter_
		return zzub_envelope_get_flags(self)
	
	def set_flags(self, flags):
		assert self._as_parameter_
		zzub_envelope_set_flags(self,flags)
	
	def is_enabled(self):
		assert self._as_parameter_
		return zzub_envelope_is_enabled(self)
	
	def enable(self, enable):
		assert self._as_parameter_
		zzub_envelope_enable(self,enable)
	
	def get_point_count(self):
		assert self._as_parameter_
		return zzub_envelope_get_point_count(self)
	
	def get_point(self, index):
		assert self._as_parameter_
		x = c_ushort()
		y = c_ushort()
		flags = c_byte()
		zzub_envelope_get_point(self,index,byref(x),byref(y),byref(flags))
		return x.value,y.value,flags.value
	
	def get_point_list(self):
		for index in xrange(self.get_point_count()):
			yield self.get_point(index)
	
	def set_point(self, index, x, y, flags):
		assert self._as_parameter_
		zzub_envelope_set_point(self,index,x,y,flags)
	
	def insert_point(self, index):
		assert self._as_parameter_
		zzub_envelope_insert_point(self,index)
	
	def delete_point(self, index):
		assert self._as_parameter_
		zzub_envelope_delete_point(self,index)
	

zzub_envelope_t._wrapper_ = Envelope

class Wave(object):
	"""Wave table"""

	_as_parameter_ = None
	_hash = 0
	
	def __init__(self, handle):
		self._as_parameter_ = handle
		self._hash = cast(self._as_parameter_, c_void_p).value
	
	@classmethod
	def _new_from_handle(cls,handle):
		if not handle:
			return None
		return cls(handle)
	
	def __hash__(self):
		return self._hash
	
	def __eq__(self,other):
		return self._hash == hash(other)
	
	def __ne__(self,other):
		return self._hash != hash(other)
	
	def get_index(self):
		assert self._as_parameter_
		return zzub_wave_get_index(self)
	
	def load_sample(self, level, offset, clear, path, datastream):
		assert self._as_parameter_
		return zzub_wave_load_sample(self,level,offset,clear,path,datastream)
	
	def save_sample(self, level, datastream):
		assert self._as_parameter_
		return zzub_wave_save_sample(self,level,datastream)
	
	def save_sample_range(self, level, datastream, start, end):
		assert self._as_parameter_
		return zzub_wave_save_sample_range(self,level,datastream,start,end)
	
	def clear(self):
		assert self._as_parameter_
		return zzub_wave_clear(self)
	
	def get_name(self):
		assert self._as_parameter_
		return zzub_wave_get_name(self)
	
	def set_name(self, name):
		assert self._as_parameter_
		zzub_wave_set_name(self,name)
	
	def get_path(self):
		assert self._as_parameter_
		return zzub_wave_get_path(self)
	
	def set_path(self, path):
		assert self._as_parameter_
		zzub_wave_set_path(self,path)
	
	def get_flags(self):
		assert self._as_parameter_
		return zzub_wave_get_flags(self)
	
	def set_flags(self, flags):
		assert self._as_parameter_
		zzub_wave_set_flags(self,flags)
	
	def get_volume(self):
		assert self._as_parameter_
		return zzub_wave_get_volume(self)
	
	def set_volume(self, volume):
		assert self._as_parameter_
		zzub_wave_set_volume(self,volume)
	
	def get_envelope_count(self):
		assert self._as_parameter_
		return zzub_wave_get_envelope_count(self)
	
	def set_envelope_count(self, count):
		assert self._as_parameter_
		zzub_wave_set_envelope_count(self,count)
	
	def get_envelope(self, index):
		assert self._as_parameter_
		return Envelope._new_from_handle(zzub_wave_get_envelope(self,index))
	
	def set_envelope(self, index, env):
		assert self._as_parameter_
		zzub_wave_set_envelope(self,index,env)
	
	def get_level_count(self):
		assert self._as_parameter_
		return zzub_wave_get_level_count(self)
	
	def get_level(self, index):
		assert self._as_parameter_
		return Wavelevel._new_from_handle(zzub_wave_get_level(self,index))
	
	def get_level_list(self):
		for index in xrange(self.get_level_count()):
			yield self.get_level(index)
	

zzub_wave_t._wrapper_ = Wave

class Recorder(object):
	"""Memory and file streams - load/save from/to file/clipboard
	Create file or memory data streams for use by e.g 
	zzub_wavetable_load_sample() and
	zzub_player_load_bmx()/zzub_player_save_bmx().
	
	In-memory streams are implemented via the zzub_archive_t object
	and destroyed via zzub_archive_destroy().
	File-streams are created with zzub_input_open_file and zzub_output_create_file()
	and closed/destroyed with zzub_input_destroy() and zzub_output_destroy()."""

	_as_parameter_ = None
	_hash = 0
	
	def __init__(self, handle):
		self._as_parameter_ = handle
		self._hash = cast(self._as_parameter_, c_void_p).value
	
	@classmethod
	def _new_from_handle(cls,handle):
		if not handle:
			return None
		return cls(handle)
	
	def __hash__(self):
		return self._hash
	
	def __eq__(self,other):
		return self._hash == hash(other)
	
	def __ne__(self,other):
		return self._hash != hash(other)
	

zzub_recorder_t._wrapper_ = Recorder

class Player(object):
	"""Player Methods"""

	_as_parameter_ = None
	_hash = 0
	
	def __init__(self, handle):
		self._as_parameter_ = handle
		self._hash = cast(self._as_parameter_, c_void_p).value
	
	@classmethod
	def _new_from_handle(cls,handle):
		if not handle:
			return None
		return cls(handle)
	
	def __hash__(self):
		return self._hash
	
	def __eq__(self,other):
		return self._hash == hash(other)
	
	def __ne__(self,other):
		return self._hash != hash(other)
	
	@staticmethod
	def create():
		"""Create a player instance."""
		return Player._new_from_handle(zzub_player_create())
	
	def destroy(self):
		"""Destroy a player instance and all its resources."""
		assert self._as_parameter_
		zzub_player_destroy(self)
	
	def add_plugin_path(self, path):
		"""Adds a directory that will be scanned for plugins upon initialization.
		The path *must* be terminated with an ending (back)slash. """
		assert self._as_parameter_
		zzub_player_add_plugin_path(self,path)
	
	def blacklist_plugin(self, uri):
		"""Blacklist plugin."""
		assert self._as_parameter_
		zzub_player_blacklist_plugin(self,uri)
	
	def initialize(self, samplesPerSecond):
		"""Inititializes the player.
		initialize() must be called only after the audio driver,
		plugin directories and optional blacklists are set up."""
		assert self._as_parameter_
		return zzub_player_initialize(self,samplesPerSecond)
	
	def load_bmx(self, datastream, maxLen, flags, x, y):
		"""Loads a BMX from memory or file.
		 
		Load warnings and error messages are placed in the messages string."""
		assert self._as_parameter_
		messages = (c_char*maxLen)()
		_ret_arg = zzub_player_load_bmx(self,datastream,messages,maxLen,flags,x,y)
		return _ret_arg,messages.value
	
	def save_bmx(self, plugins, num_plugins, save_waves, datastream):
		"""Saves a BMX to memory or file.
		 
		plugins is an array of ints containing the plugin ids to save in the song. If plugins is NULL,
		everything in the running graph is saved. Optionally without waves, when save_waves is zero."""
		assert self._as_parameter_
		plugins = (POINTER(zzub_plugin_t)*num_plugins)(*plugins)
		return zzub_player_save_bmx(self,plugins,num_plugins,save_waves,datastream)
	
	def load_ccm(self, fileName):
		"""Load a project in CCM file format from disk."""
		assert self._as_parameter_
		return zzub_player_load_ccm(self,fileName)
	
	def save_ccm(self, fileName):
		"""Save current project in the CCM file format to disk."""
		assert self._as_parameter_
		return zzub_player_save_ccm(self,fileName)
	
	def get_state(self):
		"""Returns one of the values in the state enumeration."""
		assert self._as_parameter_
		return zzub_player_get_state(self)
	
	def set_state(self, state):
		"""Set player state. Takes one of the values in the state enumeration as parameter."""
		assert self._as_parameter_
		zzub_player_set_state(self,state)
	
	def set_position(self, tick):
		assert self._as_parameter_
		zzub_player_set_position(self,tick)
	
	def get_bpm(self):
		assert self._as_parameter_
		return zzub_player_get_bpm(self)
	
	def get_tpb(self):
		assert self._as_parameter_
		return zzub_player_get_tpb(self)
	
	def set_bpm(self, bpm):
		assert self._as_parameter_
		zzub_player_set_bpm(self,bpm)
	
	def set_tpb(self, tpb):
		assert self._as_parameter_
		zzub_player_set_tpb(self,tpb)
	
	def get_pluginloader_count(self):
		"""Returns number of plugin loaders."""
		assert self._as_parameter_
		return zzub_player_get_pluginloader_count(self)
	
	def get_pluginloader(self, index):
		"""Returns a zzub_pluginloader_t handle by index."""
		assert self._as_parameter_
		return Pluginloader._new_from_handle(zzub_player_get_pluginloader(self,index))
	
	def get_pluginloader_by_name(self, name):
		"""Finds a zzub_pluginloader_t handle by uri."""
		assert self._as_parameter_
		return Pluginloader._new_from_handle(zzub_player_get_pluginloader_by_name(self,name))
	
	def get_pluginloader_list(self):
		for index in xrange(self.get_pluginloader_count()):
			yield self.get_pluginloader(index)
	
	def get_plugin_count(self):
		"""Returns number of plugins in the current song."""
		assert self._as_parameter_
		return zzub_player_get_plugin_count(self)
	
	@staticmethod
	def add_midimapping(plugin, group, track, param, channel, controller):
		return Midimapping._new_from_handle(zzub_player_add_midimapping(plugin,group,track,param,channel,controller))
	
	@staticmethod
	def remove_midimapping(plugin, group, track, param):
		return zzub_player_remove_midimapping(plugin,group,track,param)
	
	def get_plugin_by_name(self, name):
		"""Returns the plugin object given the plugins name."""
		assert self._as_parameter_
		return Plugin._new_from_handle(zzub_player_get_plugin_by_name(self,name))
	
	def get_plugin_by_id(self, id):
		"""Returns the plugin object given the plugin id. See also zzub_plugin_get_id()."""
		assert self._as_parameter_
		return Plugin._new_from_handle(zzub_player_get_plugin_by_id(self,id))
	
	def get_plugin(self, index):
		"""Returns the plugin object given the plugins index in the graph."""
		assert self._as_parameter_
		return Plugin._new_from_handle(zzub_player_get_plugin(self,index))
	
	def get_plugin_list(self):
		for index in xrange(self.get_plugin_count()):
			yield self.get_plugin(index)
	
	def work_stereo(self):
		assert self._as_parameter_
		numSamples = c_int()
		_ret_arg = zzub_player_work_stereo(self,byref(numSamples))
		return _ret_arg,numSamples.value
	
	def clear(self):
		assert self._as_parameter_
		zzub_player_clear(self)
	
	def get_position(self):
		assert self._as_parameter_
		return zzub_player_get_position(self)
	
	def set_position(self, pos):
		assert self._as_parameter_
		zzub_player_set_position(self,pos)
	
	def get_loop_start(self):
		assert self._as_parameter_
		return zzub_player_get_loop_start(self)
	
	def get_loop_end(self):
		assert self._as_parameter_
		return zzub_player_get_loop_end(self)
	
	def set_loop_start(self, v):
		assert self._as_parameter_
		zzub_player_set_loop_start(self,v)
	
	def set_loop_end(self, v):
		assert self._as_parameter_
		zzub_player_set_loop_end(self,v)
	
	def get_loop(self):
		assert self._as_parameter_
		begin = c_int()
		end = c_int()
		zzub_player_get_loop(self,byref(begin),byref(end))
		return begin.value,end.value
	
	def set_loop(self, begin, end):
		assert self._as_parameter_
		zzub_player_set_loop(self,begin,end)
	
	def get_song_start(self):
		assert self._as_parameter_
		return zzub_player_get_song_start(self)
	
	def get_song_end(self):
		assert self._as_parameter_
		return zzub_player_get_song_end(self)
	
	def set_song_start(self, v):
		assert self._as_parameter_
		zzub_player_set_song_start(self,v)
	
	def set_song_end(self, v):
		assert self._as_parameter_
		zzub_player_set_song_end(self,v)
	
	def set_loop_enabled(self, enable):
		assert self._as_parameter_
		zzub_player_set_loop_enabled(self,enable)
	
	def get_loop_enabled(self):
		assert self._as_parameter_
		return zzub_player_get_loop_enabled(self)
	
	def get_sequence_track_count(self):
		assert self._as_parameter_
		return zzub_player_get_sequence_track_count(self)
	
	def get_sequence(self, index):
		assert self._as_parameter_
		return Sequence._new_from_handle(zzub_player_get_sequence(self,index))
	
	def get_sequence_list(self):
		for index in xrange(self.get_sequence_track_count()):
			yield self.get_sequence(index)
	
	@staticmethod
	def get_currently_playing_pattern(plugin):
		"""Retreive the currently playing pattern and row for a plugin."""
		pattern = c_int()
		row = c_int()
		_ret_arg = zzub_player_get_currently_playing_pattern(plugin,byref(pattern),byref(row))
		return _ret_arg,pattern.value,row.value
	
	@staticmethod
	def get_currently_playing_pattern_row(plugin, pattern):
		"""Retreive the currently playing row for a plugin and a pattern."""
		row = c_int()
		_ret_arg = zzub_player_get_currently_playing_pattern_row(plugin,pattern,byref(row))
		return _ret_arg,row.value
	
	def get_wave_count(self):
		assert self._as_parameter_
		return zzub_player_get_wave_count(self)
	
	def get_wave(self, index):
		assert self._as_parameter_
		return Wave._new_from_handle(zzub_player_get_wave(self,index))
	
	def get_wave_list(self):
		for index in xrange(self.get_wave_count()):
			yield self.get_wave(index)
	
	def get_next_event(self):
		"""Returns a pointer to the next event or zero. Intended to replace
		the set_callback/handle_events combo. Call this periodically
		in a timer or on idle processing. When calling, call get_next_event
		until a NULL pointer occurs. After the call, all previously returned
		pointers are invalid."""
		assert self._as_parameter_
		return (lambda p: p and p.contents)(zzub_player_get_next_event(self))
	
	def set_callback(self, callback, tag):
		"""Sets a function that receives events."""
		assert self._as_parameter_
		zzub_player_set_callback(self,callback,tag)
	
	def handle_events(self):
		"""Process player events. Intended to be called by the host in a timer
		or on idle processing to receive events about parameter changes etc."""
		assert self._as_parameter_
		zzub_player_handle_events(self)
	
	def set_event_queue_state(self, enable):
		assert self._as_parameter_
		zzub_player_set_event_queue_state(self,enable)
	
	def get_midimapping(self, index):
		assert self._as_parameter_
		return Midimapping._new_from_handle(zzub_player_get_midimapping(self,index))
	
	def get_midimapping_count(self):
		assert self._as_parameter_
		return zzub_player_get_midimapping_count(self)
	
	def get_midimapping_list(self):
		for index in xrange(self.get_midimapping_count()):
			yield self.get_midimapping(index)
	
	def get_automation(self):
		assert self._as_parameter_
		return zzub_player_get_automation(self)
	
	def set_automation(self, enable):
		assert self._as_parameter_
		zzub_player_set_automation(self,enable)
	
	def get_midi_transport(self):
		assert self._as_parameter_
		return zzub_player_get_midi_transport(self)
	
	def set_midi_transport(self, enable):
		assert self._as_parameter_
		zzub_player_set_midi_transport(self,enable)
	
	def set_seqstep(self, step):
		assert self._as_parameter_
		zzub_player_set_seqstep(self,step)
	
	def get_seqstep(self):
		assert self._as_parameter_
		return zzub_player_get_seqstep(self)
	
	def get_infotext(self):
		assert self._as_parameter_
		return zzub_player_get_infotext(self)
	
	def set_infotext(self, text):
		assert self._as_parameter_
		zzub_player_set_infotext(self,text)
	
	def set_midi_plugin(self, plugin):
		"""Sets the plugin to receive MIDI data if the plugin's internal MIDI
		channel is set to the special channel 17 ("Play if selected")."""
		assert self._as_parameter_
		zzub_player_set_midi_plugin(self,plugin)
	
	def get_midi_plugin(self):
		assert self._as_parameter_
		return Plugin._new_from_handle(zzub_player_get_midi_plugin(self))
	
	def get_new_plugin_name(self, uri, maxLen=1024):
		"""Generates a new plugin name that can be used in a call to create_plugin()."""
		assert self._as_parameter_
		name = (c_char*maxLen)()
		zzub_player_get_new_plugin_name(self,uri,name,maxLen)
		return name.value
	
	def reset_keyjazz(self):
		assert self._as_parameter_
		zzub_player_reset_keyjazz(self)
	
	def create_plugin(self, input, dataSize, instanceName, loader, flags=0):
		"""Create a new plugin"""
		assert self._as_parameter_
		return Plugin._new_from_handle(zzub_player_create_plugin(self,input,dataSize,instanceName,loader,flags))
	
	def create_sequence(self, plugin, type):
		assert self._as_parameter_
		return Sequence._new_from_handle(zzub_player_create_sequence(self,plugin,type))
	
	def flush(self, redo_event, undo_event):
		"""Write changes made to the graph since zzub_player_begin().
		
		When redo_event and/or undo_event are NULL, zzub will invoke the callback for every editing operation.
		If a custom event is specified, the callback is invoked only once with either redo_event or undo_event
		as its parameter."""
		assert self._as_parameter_
		zzub_player_flush(self,redo_event,undo_event)
	
	def undo(self):
		"""Rolls back all editing operations one step. Each step is defined with a call to zzub_player_history_commit()."""
		assert self._as_parameter_
		zzub_player_undo(self)
	
	def redo(self):
		"""Redoes all editing operations since last call to zzub_player_history_commit()."""
		assert self._as_parameter_
		zzub_player_redo(self)
	
	def history_commit(self, description):
		"""Commits the last operations to the undo buffer and marks a new undo step."""
		assert self._as_parameter_
		zzub_player_history_commit(self,description)
	
	def history_get_uncomitted_operations(self):
		"""Returns the count of uncomitted operations."""
		assert self._as_parameter_
		return zzub_player_history_get_uncomitted_operations(self)
	
	def history_flush_last(self):
		"""Causes the last operations to not appear in the undo buffer."""
		assert self._as_parameter_
		zzub_player_history_flush_last(self)
	
	def history_flush(self):
		"""Clears the undo buffer and frees all associated resources."""
		assert self._as_parameter_
		zzub_player_history_flush(self)
	
	def history_get_size(self):
		"""Returns the size of the undo buffer."""
		assert self._as_parameter_
		return zzub_player_history_get_size(self)
	
	def history_get_position(self):
		"""Returns the current position in the undo buffer."""
		assert self._as_parameter_
		return zzub_player_history_get_position(self)
	
	def history_get_description(self, position):
		"""Returns the description of an operation in the undo buffer."""
		assert self._as_parameter_
		return zzub_player_history_get_description(self,position)
	
	def set_host_info(self, id, version, host_ptr):
		"""Set versioned, host-specific data. Plugins can retreive a pointer to this information with _host->get_host_info().
		Use and/or dependence on the host's version is regarded as bad practise and should not be used in new code."""
		assert self._as_parameter_
		zzub_player_set_host_info(self,id,version,host_ptr)
	

zzub_player_t._wrapper_ = Player

