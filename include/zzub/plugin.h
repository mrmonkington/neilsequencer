// zzub Plugin Interface
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

#if !defined(__ZZUBPLUGIN_H)
#define __ZZUBPLUGIN_H

#include <vector>
#include <string>
#include <cassert>
#include "types.h"


namespace zzub {

enum {
	// Current version of the zzub interface. Pass this to the
	// version member of zzub::info.
	// It is not unlikely, that this enum argument is going
	// to be removed in future versions, since an automatic
	// signature system is now in place.
	version = zzub_version,

	// The maximum size of an audio buffer in samples. libzzub
	// will never pass a buffer that is larger than
	// zzub::buffer_size * sizeof(float) * 2 to any of the
	// plugins process methods.
	buffer_size = zzub_buffer_size,
};

// Possible event types sent by the host. A plugin can register to
// receive these events in zzub::plugin::init() using the
// zzub::host::set_event_handler() method.
enum event_type {
	// Sent by the gui when the plugins visual representation
	// is being double clicked in the router. This is a good
	// moment to fire up an external gui.
	event_type_double_click = zzub_event_type_double_click,
	
	// most of the following events are used to organize library and gui
	// communication and should not be handled by a plugin.
	
	event_type_new_plugin = zzub_event_type_new_plugin,
	event_type_delete_plugin = zzub_event_type_delete_plugin,
	event_type_disconnect = zzub_event_type_disconnect,
	event_type_connect = zzub_event_type_connect,
	event_type_parameter_changed = zzub_event_type_parameter_changed,
	event_type_set_tracks = zzub_event_type_set_tracks,

	event_type_pre_disconnect = zzub_event_type_pre_disconnect,
	event_type_pre_connect = zzub_event_type_pre_connect,
	event_type_pre_set_tracks = zzub_event_type_pre_set_tracks,

	// global/master events
	event_type_load_progress	= zzub_event_type_load_progress,
	event_type_midi_control	= zzub_event_type_midi_control,
	event_type_wave_allocated	= zzub_event_type_wave_allocated,
	
	event_type_player_state_changed = zzub_event_type_player_state_changed,
	event_type_osc_message = zzub_event_type_osc_message,
	
	// catch all event
	event_type_all = zzub_event_type_all,
};

// Possible types for plugin parameters. These attributes can
// be passed to the type member of the zzub::parameter structure.
// They influence the size of data passed to the plugin,
// the visual appearance of parameter fields in the pattern
// editor and individual editing behaviour.
enum parameter_type {
	// Indicates that the parameter is a note, accepting
	// values from C-0 to B-9 and a special note off value. This
	// parameter has a size of 1 byte.
	parameter_type_note	= zzub_parameter_type_note,
	
	// Indicates that the parameter is a switch, which
	// can either be on or off. This parameter has a size of 1
	// byte.
	parameter_type_switch	= zzub_parameter_type_switch,
	
	// Indicates that the parameter is a byte, which can
	// take any random value between 0 and 255. This parameter
	// has a size of 1 byte.
	parameter_type_byte	= zzub_parameter_type_byte,
	
	// Indicates that the parameter is a word, which can
	// take any random value between 0 and 65535. This parameter
	// has a size of 2 bytes.
	parameter_type_word	= zzub_parameter_type_word,
};

enum wave_buffer_type {
	wave_buffer_type_si16	= zzub_wave_buffer_type_si16,    // signed int 16bit
	wave_buffer_type_f32	= zzub_wave_buffer_type_f32,    // float 32bit
	wave_buffer_type_si32	= zzub_wave_buffer_type_si32,    // signed int 32bit
	wave_buffer_type_si24	= zzub_wave_buffer_type_si24,    // signed int 24bit
};

enum oscillator_type {
	oscillator_type_sine	= zzub_oscillator_type_sine,
	oscillator_type_sawtooth	= zzub_oscillator_type_sawtooth,
	oscillator_type_pulse	= zzub_oscillator_type_pulse,
	oscillator_type_triangle	= zzub_oscillator_type_triangle,
	oscillator_type_noise	= zzub_oscillator_type_noise,
	oscillator_type_sawtooth_303 = zzub_oscillator_type_sawtooth_303,
};

enum note_value {
	// predefined values for notes
	note_value_none	= zzub_note_value_none,
	note_value_off	= zzub_note_value_off,
	note_value_min	= zzub_note_value_min,
	note_value_max	= zzub_note_value_max,
	note_value_c4 = zzub_note_value_c4
};

enum switch_value {
	// predefined values for switches
	switch_value_none	= zzub_switch_value_none,
	switch_value_off	= zzub_switch_value_off,
	switch_value_on	= zzub_switch_value_on,
};

enum wavetable_index_value {
	// predefined values for wavetable indices
	wavetable_index_value_none	= zzub_wavetable_index_value_none,
	wavetable_index_value_min	= zzub_wavetable_index_value_min,
	wavetable_index_value_max	= zzub_wavetable_index_value_max,
};

enum parameter_flag {
	// parameter flags
	parameter_flag_wavetable_index	= zzub_parameter_flag_wavetable_index,
	parameter_flag_state	= zzub_parameter_flag_state,
	parameter_flag_event_on_edit	= zzub_parameter_flag_event_on_edit,
};

enum plugin_flag {
	// plugin flags
	plugin_flag_mono_to_stereo	= zzub_plugin_flag_mono_to_stereo,
	plugin_flag_plays_waves	= zzub_plugin_flag_plays_waves,
	plugin_flag_uses_lib_interface	= zzub_plugin_flag_uses_lib_interface,
	plugin_flag_uses_instruments	= zzub_plugin_flag_uses_instruments,
	plugin_flag_does_input_mixing	= zzub_plugin_flag_does_input_mixing,
	//~ plugin_flag_no_output	= zzub_plugin_flag_no_output,
	plugin_flag_control_plugin	= zzub_plugin_flag_control_plugin,
	plugin_flag_auxiliary	= zzub_plugin_flag_auxiliary,
	plugin_flag_is_root = zzub_plugin_flag_is_root,
	plugin_flag_has_audio_input = zzub_plugin_flag_has_audio_input,
	plugin_flag_has_audio_output = zzub_plugin_flag_has_audio_output,
	plugin_flag_has_event_input = zzub_plugin_flag_has_event_input,
	plugin_flag_has_event_output = zzub_plugin_flag_has_event_output,
	plugin_flag_offline = zzub_plugin_flag_offline, 
	plugin_flag_stream = zzub_plugin_flag_stream,
};

enum state_flag {
	// player state flags
	state_flag_playing	= zzub_state_flag_playing,
	state_flag_recording = zzub_state_flag_recording,
};

enum wave_flag {
	wave_flag_loop	= zzub_wave_flag_loop,
	wave_flag_extended	= zzub_wave_flag_extended,
	wave_flag_stereo = zzub_wave_flag_stereo,
	wave_flag_pingpong	= zzub_wave_flag_pingpong,
	wave_flag_envelope	= zzub_wave_flag_envelope,
};

enum envelope_flag {
	envelope_flag_sustain	= zzub_envelope_flag_sustain,
	envelope_flag_loop	= zzub_envelope_flag_loop,
};

enum process_mode {
	// processing modes
	process_mode_no_io	= zzub_process_mode_no_io,
	process_mode_read	= zzub_process_mode_read,
	process_mode_write	= zzub_process_mode_write,
	process_mode_read_write = zzub_process_mode_read_write,
};

enum connection_type {
	connection_type_audio = zzub_connection_type_audio,
	connection_type_event = zzub_connection_type_event,
};

struct parameter {
	parameter_type type;
	const char *name;
	const char *description;
	int value_min;
	int value_max;
	int value_none;
	int flags;
	int value_default;
	
	parameter() {
		type = parameter_type_switch;
		name = 0;
		description = 0;
		value_min = 0;
		value_max = 0;
		value_none = 0;
		flags = 0;
		value_default = 0;
	}
	
	parameter &set_type(parameter_type type) { this->type = type; return *this; }
	parameter &set_note() {
		this->type = parameter_type_note;
		this->name = "Note";
		this->description = "Note";
		this->value_min = note_value_min;
		this->value_max = note_value_max;
		this->value_none = note_value_none;
		this->value_default = this->value_none;
		return *this;
	}
	parameter &set_switch() {
		this->type = parameter_type_switch;
		this->name = "Switch";
		this->description = "Switch";
		this->value_min = switch_value_off;
		this->value_max = switch_value_on;
		this->value_none = switch_value_none;
		this->value_default = this->value_none;
		return *this;
	}
	parameter &set_byte() {
		this->type = parameter_type_byte;
		this->name = "Byte";
		this->description = "Byte";
		this->value_min = 0;
		this->value_max = 128;
		this->value_none = 255;
		this->value_default = this->value_none;
		return *this;
	}
	parameter &set_word() {
		this->type = parameter_type_word;
		this->name = "Word";
		this->description = "Word";
		this->value_min = 0;
		this->value_max = 32768;
		this->value_none = 65535;
		this->value_default = this->value_none;
		return *this;
	}
	parameter &set_wavetable_index() {
		this->type = parameter_type_byte;
		this->name = "Wave";
		this->description = "Wave to use (01-C8)";
		this->value_min = wavetable_index_value_min;
		this->value_max = wavetable_index_value_max;
		this->value_none = wavetable_index_value_none;
		this->flags = parameter_flag_wavetable_index;
		this->value_default = 0;
		return *this;
	}
	parameter &set_name(const char *name) { this->name = name; return *this; }
	parameter &set_description(const char *description) { this->description = description; return *this; }
	parameter &set_value_min(int value_min) { this->value_min = value_min; return *this; }
	parameter &set_value_max(int value_max) { this->value_max = value_max; return *this; }
	parameter &set_value_none(int value_none) { this->value_none = value_none; return *this; }
	parameter &set_flags(int flags) { this->flags = flags; return *this; }
	parameter &set_state_flag() { this->flags |= zzub::parameter_flag_state; return *this; }
	parameter &set_wavetable_index_flag() { this->flags |= zzub::parameter_flag_wavetable_index; return *this; }
	parameter &set_event_on_edit_flag() { this->flags |= zzub::parameter_flag_event_on_edit; return *this; }
	parameter &set_value_default(int value_default) { this->value_default = value_default; return *this; }
	
	float normalize(int value) const {
		assert(value != this->value_none);
		return float(value - this->value_min) / float(this->value_max - this->value_min);
	}
	
	int scale(float normal) const {
		return int(normal * float(this->value_max - this->value_min) + 0.5) + this->value_min;
	}
	
	size_t get_bytesize() const {
		switch(this->type) {
			case parameter_type_note:
			case parameter_type_switch:
			case parameter_type_byte:
				return 1;
			case parameter_type_word:
				return 2;
			default:
				return 0;
		}
		return 0;
	}
	
	parameter &append(std::vector<const parameter *>& paramlist) {
		paramlist.push_back(this);
		return *this;
	}
};

struct attribute {
	const char *name;
	int value_min;
	int value_max;
	int value_default;
	
	attribute() {
		name = "";
		value_min = 0;
		value_max = 0;
		value_default = 0;
	}
	
	attribute &set_name(const char *name) { this->name = name; return *this; }
	attribute &set_value_min(int value_min) { this->value_min = value_min; return *this; }
	attribute &set_value_max(int value_max) { this->value_max = value_max; return *this; }
	attribute &set_value_default(int value_default) { this->value_default = value_default; return *this; }

};

struct envelope_info {
	const char* name;
	int flags;
};

struct master_info {
	int beats_per_minute;
	int ticks_per_beat;
	int samples_per_second;
	int samples_per_tick;
	int tick_position;
	float ticks_per_second;
	float samples_per_tick_frac;    // zzub extension
};

struct wave_info_ex;

struct wave_level {
	int sample_count;
	short *samples;
	int root_note;
	int samples_per_second;
	int loop_start;
	int loop_end;

	std::string stream_plugin_uri;
	std::string stream_data_url;
	wave_info_ex* wave;
	int level;

	wave_level() {
		level = 0;
		samples = 0;
		sample_count = 0;
		loop_start = 0;
		loop_end = 0;
		samples_per_second = 0;
		wave = 0;
	}
};

struct envelope_point {
	unsigned short x, y;
	unsigned char flags;		// flags: bit 0 = sustain
};

struct envelope_entry {
	unsigned short attack, decay, sustain, release;
	char subDivide, flags;	// ADSR Subdivide & Flags
	bool disabled;
	std::vector<envelope_point> points;

	envelope_entry();
	void clear();
};

struct wave_info {
	int flags;
	float volume;

	std::string fileName;
	std::string name;
	std::vector<envelope_entry> envelopes;
	std::vector<wave_level> levels;

	int get_levels() const {
		return levels.size();
	}

	wave_level* get_level(int level) const {
		if (level < 0 || level >= levels.size()) return 0;
		return &(wave_level&)levels[level];
	}

	bool get_extended() const {
		return flags&wave_flag_extended?true:false;
	}

	bool get_stereo() const {
		return flags&zzub::wave_flag_stereo?true:false;
	}

	void set_stereo(bool state) {
		unsigned f = ((unsigned)flags)&(0xFFFFFFFF^zzub::wave_flag_stereo);

		if (state)
			flags = f|zzub::wave_flag_stereo; else
			flags = f;
	}

	void* get_sample_ptr(int level, int offset=0) const {
		wave_level* l = get_level(level);
		if (!l) return 0;
		offset *= get_bytes_per_sample(level) * (get_stereo()?2:1);
		if (get_extended()) {
			return (char*)&l->samples[4] + offset;
		} else
			return (char*)l->samples + offset;
	}

	int get_bits_per_sample(int level) const {
		wave_level* l = get_level(level);
		if (!l) return 0;

		if (!get_extended()) return 16;

		switch (l->samples[0]) {
			case zzub::wave_buffer_type_si16:
				return 16;
			case zzub::wave_buffer_type_si24:
				return 24;
			case zzub::wave_buffer_type_f32:
			case zzub::wave_buffer_type_si32:
				return 32;
			default:
				//std::cerr << "Unknown extended sample format:" << l->samples[0] << std::endl;
				return 16;
		}
	}

	inline int get_bytes_per_sample(int level) const {
		return get_bits_per_sample(level) / 8;
	}

	inline unsigned int get_extended_samples(int level, unsigned int samples) const {
		int channels = get_stereo()?2:1;
		return ((samples-(4/channels)) *2 ) / get_bytes_per_sample(level);
	}

	inline unsigned int get_unextended_samples(int level, unsigned int samples) const {
		int channels = get_stereo()?2:1;
		return ((samples * get_bytes_per_sample(level)) / 2) + (4/channels);
	}

	unsigned int get_sample_count(int level) const {
		wave_level* l = get_level(level);
		if (!l) return 0;
		if (get_extended())
			return get_extended_samples(level, l->sample_count); else
			return l->sample_count;
	}

	unsigned int get_loop_start(int level) const {
		wave_level* l = get_level(level);
		if (!l) return 0;
		if (get_extended())
			return get_extended_samples(level, l->loop_start); else
			return l->loop_start;
	}

	unsigned int get_loop_end(int level) const {
		wave_level* l = get_level(level);
		if (!l) return 0;
		if (get_extended())
			return get_extended_samples(level, l->loop_end); else
			return l->loop_end;
	}

	void set_loop_start(int level, unsigned int value) {
		wave_level* l = get_level(level);
		if (!l) return ;
		if (get_extended()) {
			l->loop_start = get_unextended_samples(level, value);
		} else {
			l->loop_start = value;
		}
	}

	void set_loop_end(size_t level, size_t value) {
		wave_level* l = get_level(level);
		if (!l) return ;
		if (get_extended()) {
			l->loop_end = get_unextended_samples(level, value);
		} else {
			l->loop_end = value;
		}
	}

	wave_buffer_type get_wave_format(size_t level) const {
		wave_level* l = get_level(level);
		if (!l) return wave_buffer_type_si16;
		if (get_extended() && l->sample_count > 0) {
			return (wave_buffer_type)l->samples[0];
		} else
			return wave_buffer_type_si16;
	}

};

// each oscillator table contains one period
// of a bandlimited waveform
//
// level | samples
// ------+------------------
// 0     | 2048
// 1     | 1024
// 2     | 512
// 3     | 256
// 4     | 128
// 5     | 64
// 6     | 32
// 7     | 16
// 8     | 8
// 9     | 4
// 10    | 2
//
// all waves are 16bit signed int
//
// get_oscillator_table returns a pointer to the table
//
// get_oscillator_table_offset returns an offset to
// the table for a specified level

inline int get_oscillator_table_offset(unsigned int level) {
	return ((1 << 12) - (1 << 2)) & ~(((1 << 12) - (1 << 2)) >> level);
}

struct pattern;
struct sequence;
struct metaplugin;
struct outstream;
struct info;
struct plugin;

typedef bool (plugin::*event_handler_method)(void *);

struct event_handler {
	virtual bool invoke(zzub_event_data_t& data)=0;
};

struct host {
	virtual const wave_info * get_wave(int index);
	virtual const wave_level * get_wave_level(int index, unsigned int level);
	virtual void message(const char *text);
	virtual void lock ();
	virtual void unlock();
	virtual int get_write_position();
	virtual int get_play_position();
	virtual void set_play_position(int pos);
	virtual float ** get_auxiliary_buffer();
	virtual void clear_auxiliary_buffer();
	virtual int get_next_free_wave_index();
	virtual bool allocate_wave(int index, int level, int samples, wave_buffer_type type, bool stereo, const char *name);
	virtual void schedule_event(int time, unsigned int data);
	virtual void get_midi_output_names(outstream *pout);
	virtual void midi_out(int device, unsigned int data);
	virtual int get_midi_device(const char* name);
	virtual const short * get_oscillator_table(int waveform);
	virtual int get_envelope_size(int wave, int envelope);
	virtual bool get_envelope_point(int wave, int envelope, int index, unsigned short &x, unsigned short &y, int &flags);
	virtual const wave_level * get_nearest_wave_level(int index, int note);
	virtual void set_track_count(int count);
	virtual pattern * create_pattern(const char *name, int length);
	virtual pattern * get_pattern(int index);
	virtual char const * get_pattern_name(pattern *_pattern);
	virtual void rename_pattern(char const *oldname, char const *newname);
	virtual void delete_pattern(pattern *_pattern);
	virtual int get_pattern_data(pattern *_pattern, int row, int group, int track, int field);
	virtual void set_pattern_data(pattern *_pattern, int row, int group, int track, int field, int value);
	virtual sequence * create_sequence();
	virtual void delete_sequence(sequence *_sequence);
	virtual pattern * get_sequence_data(int row);
	virtual void set_sequence_data(int row, pattern* pattern);
	virtual void _legacy_control_change(int group, int track, int param, int value);
	virtual int audio_driver_get_channel_count(bool input);
	virtual void audio_driver_write(int channel, float *samples, int buffersize);
	virtual void audio_driver_read(int channel, float *samples, int buffersize);
	virtual metaplugin * get_metaplugin();
	virtual void control_change(metaplugin *_metaplugin, int group, int track, int param, int value, bool record, bool immediate);
	
	virtual sequence * get_playing_sequence(metaplugin *_metaplugin);
	virtual void * get_playing_row(sequence *_sequence, int group, int track);
	virtual int get_state_flags();
	virtual void set_state_flags(int state);
	virtual void set_event_handler(metaplugin *_metaplugin, event_handler* handler);
	virtual const char * get_wave_name(int index);
	virtual void set_internal_wave_name(metaplugin *_metaplugin, int index, const char *name);
	virtual void get_plugin_names(outstream *os);
	virtual metaplugin * get_metaplugin(const char *name);
	virtual info const * get_info(metaplugin *_metaplugin);
	virtual char const * get_name(metaplugin *_metaplugin);
	virtual bool get_input(int index, float *samples, int buffersize, bool stereo, float *extrabuffer);
	virtual bool get_osc_url(metaplugin *pmac, char *url);
	
	// peerctrl extensions
	virtual int get_parameter(metaplugin *_metaplugin, int group, int track, int param);
	virtual plugin *get_plugin(metaplugin *_metaplugin);

	// hacked extensions
	virtual int get_song_begin();
	virtual void set_song_begin(int pos);
	virtual int get_song_end();
	virtual void set_song_end(int pos);
	virtual int get_song_begin_loop();
	virtual void set_song_begin_loop(int pos);
	virtual int get_song_end_loop();
	virtual void set_song_end_loop(int pos);

	// wavetable stream support
	virtual plugin* stream_create(int index, int level);
	virtual plugin* stream_create(const char* pluginUri, const char* dataUrl);
	virtual void stream_destroy(plugin* stream);

	metaplugin* _metaplugin;
	host(metaplugin* _metaplugin);
	~host();
	float *auxBuffer[2];
};


struct lib {
	virtual void get_instrument_list(outstream *os) = 0;
};
		
struct instream	{
	virtual int read(void *buffer, int size) = 0;

	virtual long position() = 0;
	virtual void seek(long, int) = 0;
	
	virtual long size() = 0;

	template <typename T>
	int read(T &d) { return read(&d, sizeof(T)); }

	int read(std::string& d) {
		char c = -1;
		d = "";
		int i = 0;
		do {
			if (!read<char>(c)) break;
			if (c) d += c;
			i++;
		} while (c != 0);
		return i;
	}
};

struct outstream {
	virtual int write(void *buffer, int size) = 0;
	
	template <typename T>
	int write(T d) { return write(&d, sizeof(T)); }

	#if defined(_STRING_H_) || defined(_INC_STRING)
	// include string.h or cstring before zzubplugin.h to get this function
	int write(const char *str) { return write((void*)str, (int)strlen(str) + 1); }
	#elif defined(_GLIBCXX_CSTRING)
	int write(const char *str) { return write((void*)str, (int)std::strlen(str) + 1); }
	#endif

	virtual long position() = 0;
	virtual void seek(long, int) = 0;
};

struct archive {
	virtual outstream *get_outstream(const char *path) = 0;
	virtual instream *get_instream(const char *path) = 0;
};

struct info	{
	int version;
	int flags;
	unsigned int min_tracks;
	unsigned int max_tracks;
	const char* name;
	const char* short_name;
	const char* author;
	const char* commands;
	lib* plugin_lib;
	const char* uri;
	
	std::vector<const zzub::parameter*> global_parameters;
	
  std::vector<const zzub::parameter*> track_parameters;
  
  // for controller plugins: those will be associated with parameters of remote plugins
  // they are purely internal and will not be visible in the pattern editor or gui
  std::vector<const zzub::parameter*> controller_parameters; 

	std::vector<const zzub::attribute*> attributes;
	
	virtual zzub::plugin* create_plugin() const = 0;// { return 0; }
	virtual bool store_info(zzub::archive *arc) const = 0;// { return false; }

	zzub::parameter& add_global_parameter() {
		zzub::parameter *param = new zzub::parameter();
		global_parameters.push_back(param);
		return *param;
	}

	zzub::parameter& add_track_parameter() {
		zzub::parameter *param = new zzub::parameter();
		track_parameters.push_back(param);
		return *param;
	}

	zzub::parameter& add_controller_parameter() {
		zzub::parameter *param = new zzub::parameter();
		controller_parameters.push_back(param);
		return *param;
	}

	zzub::attribute& add_attribute() {
		zzub::attribute *attrib = new zzub::attribute();
		attributes.push_back(attrib);
		return *attrib;
	}
	
	info() {
		version = zzub::version;
		flags = 0;
		min_tracks = 0;
		max_tracks = 0;
		name = "";
		short_name = "";
		author = "";
		commands = 0;
		plugin_lib = 0;
		uri = 0;
	}
	
	virtual ~info() {
		for (std::vector<const zzub::parameter*>::iterator i = global_parameters.begin();
			i != global_parameters.end(); ++i) {
			delete *i;
		}
		global_parameters.clear();
		for (std::vector<const zzub::parameter*>::iterator i = track_parameters.begin();
			i != track_parameters.end(); ++i) {
			delete *i;
		}
		track_parameters.clear();
		for (std::vector<const zzub::parameter*>::iterator i = controller_parameters.begin();
			i != controller_parameters.end(); ++i) {
			delete *i;
		}
		controller_parameters.clear();
		for (std::vector<const zzub::attribute*>::iterator i = attributes.begin();
			i != attributes.end(); ++i) {
			delete *i;
		}
		attributes.clear();
	}

    static int calc_column_size(const std::vector<const zzub::parameter*> &params) {
        int size = 0;
        for (unsigned i = 0; i<params.size(); i++) {
            size += params[i]->get_bytesize();
        }
        return size;
    }

    int get_group_size(int group) const {
        switch (group) {
            case 0:
                return 2*sizeof(short);
            case 1:
                return calc_column_size(global_parameters);
            case 2:
                return calc_column_size(track_parameters);
            default:
                return 0;
        }
    }

};

struct plugin {
	virtual ~plugin() { }
	virtual void destroy() = 0;//{ delete this; }
	virtual void init(zzub::archive *arc) = 0;//{}
	virtual void process_events() = 0;//{}
	virtual void process_controller_events() = 0;//{}
	virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode) = 0;//{ return false; }
	virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) = 0;//{ return false; }
	virtual void stop() = 0;//{}
	virtual void load(zzub::archive *arc) = 0;//{}
	virtual void save(zzub::archive *arc) = 0;//{}
	virtual void attributes_changed() = 0;//{}
	virtual void command(int index) = 0;//{}
	virtual void set_track_count(int count) = 0;//{}
	virtual void mute_track(int index) = 0;//{}
	virtual bool is_track_muted(int index) const = 0;//{ return false; }
	virtual void midi_note(int channel, int value, int velocity)  = 0;//{}
	virtual void event(unsigned int data)  = 0;//{}
	virtual const char * describe_value(int param, int value) = 0;//{ return 0; }
	virtual const zzub::envelope_info ** get_envelope_infos() = 0;//{ return 0; }
	virtual bool play_wave(int wave, int note, float volume) = 0;//{ return false; }
	virtual void stop_wave() = 0;//{}
	virtual int get_wave_envelope_play_position(int env) = 0;//{ return -1; }

	// these have been in zzub::plugin2 before
	virtual const char* describe_param(int param) = 0;//{ return 0; }
	virtual bool set_instrument(const char *name) = 0;//{ return false; }
	virtual void get_sub_menu(int index, zzub::outstream *os) = 0;//{}
	virtual void add_input(const char *name) = 0;//{}
	virtual void delete_input(const char *name) = 0;//{}
	virtual void rename_input(const char *oldname, const char *newname) = 0;//{}
	virtual void input(float **samples, int size, float amp) = 0;//{}
	virtual void midi_control_change(int ctrl, int channel, int value) = 0;//{}
	virtual bool handle_input(int index, int amp, int pan) = 0;//{ return false; }

	plugin() {
		global_values = 0;
		track_values = 0;
    controller_values = 0;
		attributes = 0;
		_master_info = 0;
		_host = 0;
	}

	void *global_values;
	void *track_values;
  void *controller_values;
	int *attributes;

	master_info *_master_info;
	host *_host;

};

// A plugin factory allows to add and replace plugin infos
// known to the host.
struct pluginfactory {
	
	// Registers a plugin info to the host. If the uri argument
	// of the info struct designates a plugin already existing
	// to the host, the old info struct will be replaced.
	virtual void register_info(const zzub::info *_info) = 0;
};

// A plugin collection registers plugin infos and provides
// serialization services for plugin info, to allow
// loading of plugins from song data.
struct plugincollection {
	
	// Called by the host initially. The collection registers
	// plugins through the pluginfactory::register_info method.
	// The factory pointer remains valid and can be stored
	// for later reference.
	virtual void initialize(zzub::pluginfactory *factory) = 0;//{}
	
	// Called by the host upon song loading. If the collection
	// can not provide a plugin info based on the uri or
	// the metainfo passed, it should return a null pointer.
	// This will usually only be called if the host does
	// not know about the uri already.
	virtual const zzub::info *get_info(const char *uri, zzub::archive *arc) = 0;//{ return 0; }
	
	// Returns the uri of the collection to be identified,
	// return zero for no uri. Collections without uri can not be 
	// configured.
	virtual const char *get_uri() = 0;//{ return 0; }
	
	// Called by the host to set specific configuration options,
	// usually related to paths.
	virtual void configure(const char *key, const char *value) = 0;//{}
	
	// Called by the host upon destruction. You should
	// delete the instance in this function
	virtual void destroy() = 0;//{ delete this; }
};

struct scopelock {
	scopelock(host *h) {
		this->h = h;
		h->lock ()
		;
	}
	~scopelock() {
		h->unlock();
	}

	host *h;
};

#define SIGNAL_TRESHOLD (0.0000158489f)

inline bool buffer_has_signals(const float *buffer, int ns) {
	while (ns--) {
		if ((*buffer > SIGNAL_TRESHOLD)||(*buffer < -SIGNAL_TRESHOLD)) {
			return true;
		}
		buffer++;
	}
	return false;
}

#define ZZUB_PLUGIN_LOCK zzub::scopelock _sl(this->_host);

} // namespace zzub

/*
	in case you are using this header to write a plugin,
	you need to compile your library with a .def file that
	looks like this:

	EXPORTS
		zzub_get_infos
		zzub_get_signature
*/

ZZUB_EXTERN_C const char *zzub_get_signature();
ZZUB_EXTERN_C zzub::plugincollection *zzub_get_plugincollection();

typedef zzub::plugincollection *(*zzub_get_plugincollection_function)();
typedef const char *(*zzub_get_signature_function)();
#endif  // __ZZUBPLUGIN_H
