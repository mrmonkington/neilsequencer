// zzub Types
// Copyright (C) 2006 Leonard Ritter (contact@leonard-ritter.com)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#if !defined(__ZZUBTYPES_H)
#define __ZZUBTYPES_H

#if defined(__GNUC__)
#ifdef cdecl
#undef cdecl
#endif
#ifdef stdcall
#undef stdcall
#endif
#	define ZZUB_CALLING_CONVENTION __attribute__((cdecl))
#else
#	define ZZUB_CALLING_CONVENTION __cdecl
#endif
#define ZZUB_EXTERN_C extern "C"

#if defined(__cplusplus)
extern "C" {
#endif

enum {
	zzub_version = 15,

	zzub_buffer_size = 256,
};

enum zzub_event_type {
	zzub_event_type_pre_flag = 16384,

	// plugin events
	zzub_event_type_double_click = 0,
	zzub_event_type_new_plugin = 1,
	zzub_event_type_delete_plugin = 2,
	zzub_event_type_pre_delete_plugin = 9,
	zzub_event_type_disconnect = 3,
	zzub_event_type_connect = 4,
	zzub_event_type_plugin_changed = 30,
	zzub_event_type_parameter_changed = 7,
	zzub_event_type_set_tracks = 13,
	zzub_event_type_set_sequence_tracks = 23,
	zzub_event_type_set_sequence_event = 24,

	zzub_event_type_new_pattern = 25,
	zzub_event_type_delete_pattern = 26,
	zzub_event_type_pre_delete_pattern = 47,
	zzub_event_type_edit_pattern = 27,
	zzub_event_type_pattern_changed = 31,
	zzub_event_type_pattern_insert_rows = 42,
	zzub_event_type_pattern_remove_rows = 43,

	zzub_event_type_sequencer_add_track = 32,
	zzub_event_type_sequencer_remove_track = 33,
	zzub_event_type_sequencer_changed = 41,

	zzub_event_type_pre_disconnect = 34,
	zzub_event_type_pre_connect = 35,
	zzub_event_type_post_connect = 46,
	zzub_event_type_pre_set_tracks = 36,
	zzub_event_type_post_set_tracks = 45,

	zzub_event_type_envelope_changed = 37,
	zzub_event_type_slices_changed = 38,
	zzub_event_type_wave_changed = 39,
	zzub_event_type_delete_wave = 40,

	// global/master events
	zzub_event_type_load_progress	= 8,
	zzub_event_type_midi_control	= 11,
	zzub_event_type_wave_allocated	= 12,
	
	zzub_event_type_player_state_changed = 20,
	zzub_event_type_osc_message = 21,
	zzub_event_type_vu = 22,
	
	zzub_event_type_custom = 44,

	// catch all event
	zzub_event_type_all = 255,
};

enum zzub_player_state {
	zzub_player_state_playing = 0,
	zzub_player_state_stopped,
	zzub_player_state_muted,
	zzub_player_state_released
};

#if !defined(ZZUB_NO_CTYPES)

// opaque types. can not be accessed directly
typedef struct _zzub_player zzub_player_t;
typedef struct _zzub_audiodriver zzub_audiodriver_t;
typedef struct _zzub_mididriver zzub_mididriver_t;
typedef struct _zzub_plugin zzub_plugin_t;
typedef struct _zzub_pluginloader zzub_pluginloader_t;
typedef struct _zzub_plugincollection zzub_plugincollection_t;
typedef struct _zzub_pattern zzub_pattern_t;
typedef struct _zzub_sequence zzub_sequence_t;
//typedef struct _zzub_patterntrack zzub_patterntrack_t;
//typedef struct _zzub_connection zzub_connection_t;
//typedef struct _zzub_audio_connection zzub_audio_connection_t;
//typedef struct _zzub_event_connection zzub_event_connection_t;
typedef struct _zzub_event_connection_binding zzub_event_connection_binding_t;
typedef struct _zzub_wave zzub_wave_t;
typedef struct _zzub_wavelevel zzub_wavelevel_t;
typedef struct _zzub_parameter zzub_parameter_t;
typedef struct _zzub_attribute zzub_attribute_t;
typedef struct _zzub_envelope zzub_envelope_t;
typedef struct _zzub_midimapping zzub_midimapping_t;
//typedef struct _zzub_postprocess zzub_postprocess_t;
typedef struct _zzub_archive zzub_archive_t;

typedef struct _zzub_input zzub_input_t;
typedef struct _zzub_output zzub_output_t;

#endif

typedef struct zzub_event_data zzub_event_data_t;

typedef struct zzub_event_data_all {
	zzub_event_data_t *data;
} zzub_event_data_all_t;

typedef struct zzub_event_data_new_plugin {
	zzub_plugin_t* plugin;
} zzub_event_data_new_plugin_t;

typedef struct zzub_event_data_delete_plugin {
	zzub_plugin_t* plugin;
} zzub_event_data_delete_plugin_t;

typedef struct zzub_event_data_connect {
	zzub_plugin_t* from_plugin;
	zzub_plugin_t* to_plugin;
	int type;
} zzub_event_data_connect_t;

typedef struct zzub_event_data_plugin_changed {
	zzub_plugin_t* plugin;
} zzub_event_data_plugin_changed_t;

typedef struct zzub_event_data_set_tracks {
	zzub_plugin_t* plugin;
} zzub_event_data_set_tracks_t;

typedef struct zzub_event_data_set_sequence_tracks {
	zzub_plugin_t* plugin;
} zzub_event_data_set_sequence_tracks_t;

typedef struct zzub_event_data_set_sequence_event {
	zzub_plugin_t* plugin;
	int track;
	int time;
} zzub_event_data_set_sequence_event_t;

typedef struct zzub_event_data_new_pattern {
	zzub_plugin_t* plugin;
	int index;
} zzub_event_data_new_pattern_t;

typedef struct zzub_event_data_delete_pattern {
	zzub_plugin_t* plugin;
	int index;
} zzub_event_data_delete_pattern_t;

typedef struct zzub_event_data_edit_pattern {
	zzub_plugin_t* plugin;
	int index;
	int group, track, column;
	int row, value;
} zzub_event_data_edit_pattern_t;

typedef struct zzub_event_data_pattern_insert_rows {
	zzub_plugin_t* plugin;
	int index;
	int row, rows;
	int* column_indices;
	int indices;
} zzub_event_data_pattern_insert_rows_t;

typedef struct zzub_event_data_pattern_remove_rows {
	zzub_plugin_t* plugin;
	int index;
	int row, rows;
	int* column_indices;
	int indices;
} zzub_event_data_pattern_remove_rows_t;

typedef struct zzub_event_data_pattern_changed {
	zzub_plugin_t* plugin;
	int index;
} zzub_event_data_pattern_changed_t;

typedef struct zzub_event_data_midi_message {
	unsigned char status, data1, data2;
} zzub_event_data_midi_message_t;

typedef struct zzub_event_data_change_parameter {
	zzub_plugin_t* plugin;
	int group, track, param, value;
} zzub_event_data_change_parameter_t;

typedef struct zzub_event_data_player_state_changed {
	int player_state;
} zzub_event_data_player_state_changed_t;

typedef struct zzub_event_data_vu {
	int size;
	float left_amp;
	float right_amp;
	float time; // sample timestamp of information
} zzub_event_data_vu_t;

typedef struct zzub_event_data_serialize {
	char mode; // 'r' or 'w'
	zzub_archive_t *archive;
} zzub_event_data_serialize_t;

typedef struct zzub_event_data_unknown {
	void *param;
} zzub_event_data_unknown_t;

typedef struct zzub_event_data_osc_message {
	const char *path;
	const char *types;
	void **argv;
	int argc;
	void *msg;
} zzub_event_data_osc_message_t;

typedef struct zzub_event_data_change_wave {
	zzub_wave_t* wave;
} zzub_event_data_change_wave_t;

typedef struct zzub_event_data_delete_wave {
	zzub_wave_t* wave;
} zzub_event_data_delete_wave_t;

typedef struct zzub_event_data_allocate_wavelevel {
	zzub_wavelevel_t* wavelevel;
} zzub_event_data_allocate_wavelevel_t;

typedef struct zzub_event_data_custom {
	int id;
	void* data;
} zzub_event_data_custom_t;

struct zzub_event_data {
	int type;
	union {
		zzub_event_data_new_plugin_t new_plugin;
		zzub_event_data_delete_plugin_t delete_plugin;
		//zzub_event_data_delete_plugin_t pre_delete_plugin;
        zzub_event_data_midi_message_t midi_message;
        zzub_event_data_connect_t connect_plugin;
        zzub_event_data_connect_t disconnect_plugin;
		zzub_event_data_plugin_changed_t plugin_changed;
        zzub_event_data_change_parameter_t change_parameter;
        zzub_event_data_set_tracks_t set_tracks;
		zzub_event_data_player_state_changed_t player_state_changed;
		zzub_event_data_osc_message_t osc_message;
		zzub_event_data_vu_t vu;
		zzub_event_data_serialize_t serialize;
        //zzub_event_data_connect_t pre_connect_plugin;
        //zzub_event_data_connect_t pre_disconnect_plugin;
        //zzub_event_data_set_tracks_t pre_set_tracks;
		zzub_event_data_set_sequence_tracks_t set_sequence_tracks;
		zzub_event_data_set_sequence_event_t set_sequence_event;
		zzub_event_data_new_pattern_t new_pattern;
		zzub_event_data_delete_pattern_t delete_pattern;
		//zzub_event_data_delete_pattern_t pre_delete_pattern;
		zzub_event_data_edit_pattern_t edit_pattern;
		zzub_event_data_pattern_changed_t pattern_changed;
		zzub_event_data_change_wave_t change_wave;
		zzub_event_data_delete_wave_t delete_wave;
		zzub_event_data_allocate_wavelevel_t allocate_wavelevel;
		zzub_event_data_pattern_insert_rows_t pattern_insert_rows;
		zzub_event_data_pattern_remove_rows_t pattern_remove_rows;
		zzub_event_data_custom_t custom;
		zzub_event_data_all_t all;
		
		zzub_event_data_unknown_t unknown;
	};
};

typedef int (*ZzubCallback)(zzub_player_t *player, zzub_plugin_t* machine, zzub_event_data_t* data, void *tag);
typedef void (*ZzubMixCallback)(float *left, float *right, int size, void *tag);

enum zzub_parameter_type {
    // parameter types
    zzub_parameter_type_note	= 0,
    zzub_parameter_type_switch	= 1,
    zzub_parameter_type_byte	= 2,
    zzub_parameter_type_word	= 3,
};

enum zzub_wave_buffer_type {
    zzub_wave_buffer_type_si16	= 0,    // signed int 16bit
    zzub_wave_buffer_type_f32	= 1,    // float 32bit
    zzub_wave_buffer_type_si32	= 2,    // signed int 32bit
    zzub_wave_buffer_type_si24	= 3,    // signed int 24bit
};

enum zzub_oscillator_type {
    zzub_oscillator_type_sine	= 0,
    zzub_oscillator_type_sawtooth	= 1,
    zzub_oscillator_type_pulse	= 2,
    zzub_oscillator_type_triangle	= 3,
    zzub_oscillator_type_noise	= 4,
    zzub_oscillator_type_sawtooth_303 = 5,
};

enum zzub_note_value {
    // predefined values for notes
    zzub_note_value_none	= 0,
    zzub_note_value_off	= 255,
    zzub_note_value_min	= 1,
    zzub_note_value_max	= ((16 * 9) + 12),
	zzub_note_value_c4 = ((16 * 4) + 1)
};

enum zzub_switch_value {
    // predefined values for switches
    zzub_switch_value_none	= 255,
    zzub_switch_value_off	= 0,
    zzub_switch_value_on	= 1,
};

enum zzub_wavetable_index_value {
    // predefined values for wavetable indices
    zzub_wavetable_index_value_none	= 0,
    zzub_wavetable_index_value_min	= 1,
    zzub_wavetable_index_value_max	= 200,
};

enum zzub_parameter_flag {
    // parameter flags
    zzub_parameter_flag_wavetable_index	= (1 << 0),
    zzub_parameter_flag_state	= (1 << 1),
    zzub_parameter_flag_event_on_edit	= (1 << 2),
};

enum zzub_plugin_flag {
    // plugin flags
    zzub_plugin_flag_mono_to_stereo	= (1 << 0),
    zzub_plugin_flag_plays_waves	= (1 << 1),
    zzub_plugin_flag_uses_lib_interface	= (1 << 2),
    zzub_plugin_flag_uses_instruments	= (1 << 3),
    zzub_plugin_flag_does_input_mixing	= (1 << 4),
    zzub_plugin_flag_no_output	= (1 << 5), // deprecated
    zzub_plugin_flag_control_plugin	= (1 << 6),
    zzub_plugin_flag_auxiliary	= (1 << 7),
	
	zzub_plugin_flag_is_root = (1 << 16), // master plugin only
	zzub_plugin_flag_has_audio_input = (1 << 17), // for audio effects
	zzub_plugin_flag_has_audio_output = (1 << 18), // for audio generators and effects
	zzub_plugin_flag_has_event_input = (1 << 19), // for event effects
	zzub_plugin_flag_has_event_output = (1 << 20), // for event generators and effects
	
	zzub_plugin_flag_offline = (1 << 21), // works on fixed ranges of data, no realtime processing
	zzub_plugin_flag_stream = (1 << 22), // stream plugin
	zzub_plugin_flag_import = (1 << 25), // import plugin

	zzub_plugin_flag_has_midi_input = (1 << 23), // for midi effects
	zzub_plugin_flag_has_midi_output = (1 << 24), // for midi generators and effects
};

enum zzub_state_flag {
    // player state flags
    zzub_state_flag_playing	= 1,
    zzub_state_flag_recording = 2,
};

enum zzub_wave_flag {
    zzub_wave_flag_loop	= (1 << 0),
    zzub_wave_flag_extended	= (1 << 2),
    zzub_wave_flag_stereo = (1 << 3),
    zzub_wave_flag_pingpong	= (1 << 4),
    zzub_wave_flag_envelope	= (1 << 7),
};

enum zzub_envelope_flag {
    zzub_envelope_flag_sustain	= (1 << 0),
    zzub_envelope_flag_loop	= (1 << 1),
};

enum zzub_process_mode {
    // processing modes
    zzub_process_mode_no_io	= 0,
    zzub_process_mode_read	= (1 << 0),
    zzub_process_mode_write	= (1 << 1),
    zzub_process_mode_read_write = zzub_process_mode_read | zzub_process_mode_write,
};

enum zzub_connection_type {
	zzub_connection_type_audio = 0,
	zzub_connection_type_event = 1,
	zzub_connection_type_midi = 2,
};

#if defined(__cplusplus)
} // extern "C"
#endif


#endif // __ZZUBTYPES_H
