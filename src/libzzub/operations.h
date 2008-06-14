/*
Copyright (C) 2008 Anders Ervik <calvin@countzero.no>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#pragma once

namespace zzub {

struct player;

struct operation {
	operation_copy_flags copy_flags;
	zzub_event_data event_data;
	virtual ~operation() {}

	virtual bool prepare(zzub::song& song) { return false; }
	virtual bool operate(zzub::song& song) { return false; }
	virtual void finish(zzub::song& song, bool send_events) { }
protected:
	operation() { }

};

struct op_state_change : operation {
	zzub::player_state state;

	op_state_change(zzub::player_state _state);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
	virtual void finish(zzub::song& song, bool send_events);
};

struct op_player_song_loop : operation {
	int song_begin, song_end, song_loop_begin, song_loop_end, song_loop_enabled;

	op_player_song_loop(int _song_begin, int _song_end, int _song_loop_begin, int _song_loop_end, int _song_loop_enabled);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
};

struct op_plugin_create : operation {
	int id;
	zzub::player* player;
	std::vector<char> bytes;
	std::string name;
	const zzub::info* loader;

	op_plugin_create(zzub::player* _player, int _id, std::string name, std::vector<char>& bytes, const zzub::info* loader);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
	virtual void finish(zzub::song& song, bool send_events);
};

struct op_plugin_delete : operation {
	int id;
	zzub::player* player;
	metaplugin* plugin;

	op_plugin_delete(zzub::player* _player, int _id);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
	virtual void finish(zzub::song& song, bool send_events);
};

struct op_plugin_connect : operation {
	int from_id, to_id;
	std::string from_name;
	zzub::connection_type type;
	std::vector<int> values;
	std::string midi_device;
	std::vector<event_connection_binding> bindings;

	op_plugin_connect(int _from_id, int _to_id, zzub::connection_type type);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
	virtual void finish(zzub::song& song, bool send_events);
};

struct op_plugin_disconnect : operation {
	int from_id, to_id;
	std::string from_name;
	zzub::connection_type type;
	connection* conn;

	op_plugin_disconnect(int _from_id, int _to_id, zzub::connection_type type);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
	virtual void finish(zzub::song& song, bool send_events);
};

struct op_plugin_set_midi_connection_device : operation {
	int from_id, to_id;
	std::string device;
	op_plugin_set_midi_connection_device(int _to_id, int _from_id, std::string _name);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
	virtual void finish(zzub::song& song, bool send_events);
};

struct op_plugin_add_event_connection_binding : operation {
	int from_id, to_id;
	event_connection_binding binding;
	op_plugin_add_event_connection_binding(int _to_id, int _from_id, event_connection_binding _binding);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
	virtual void finish(zzub::song& song, bool send_events);
};

struct op_plugin_remove_event_connection_binding : operation {
	int from_id, to_id;
	int index;
	op_plugin_remove_event_connection_binding(int _to_id, int _from_id, int _index);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
	virtual void finish(zzub::song& song, bool send_events);
};

struct op_plugin_replace : operation {
	int id;
	metaplugin plugin;

	op_plugin_replace(int _id, const metaplugin& _plugin);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
	virtual void finish(zzub::song& song, bool send_events);
};

struct op_plugin_set_track_count : operation {
	int id;
	int tracks;

	plugin_map graph;

	op_plugin_set_track_count(int _id, int _tracks);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
	virtual void finish(zzub::song& song, bool send_events);
};

struct op_plugin_play_note : operation {
	int id;
	int note;
	int prev_note;
	int velocity;

	op_plugin_play_note(int _id, int _note, int _prev_note, int _velocity);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
};

// alternatively there could be a series of op_plugin_set_parameter and a final op_plugin_tick:
struct op_plugin_set_parameters_and_tick : operation {
	int id;
	zzub::pattern pattern;
	int row;
	bool no_process;
	bool record;
	//std::vector<keyjazz_note> keyjazz;

	op_plugin_set_parameters_and_tick(int _id, zzub::pattern& _pattern, int row, bool _no_process = false);
	//op_plugin_set_parameters_and_tick(int _id, zzub::pattern& _pattern, int row, bool _no_process, std::vector<keyjazz_note>& keyjazz);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
};

struct op_plugin_set_event_handlers : operation {
	std::string name;
	std::vector<event_handler*> handlers;

	op_plugin_set_event_handlers(std::string name, std::vector<event_handler*>& handlers);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
};

struct op_plugin_set_stream_source : operation {
	int id;
	std::string data_url;

	op_plugin_set_stream_source(int _id, std::string _data_url);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
	virtual void finish(zzub::song& song, bool send_events);
};

struct op_pattern_edit : operation {
	int id;
	int index;
	int group, track, column, row, value;

	op_pattern_edit(int _id, int _index, int _group, int _track, int _column, int _row, int _value);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
	virtual void finish(zzub::song& song, bool send_events);
};

struct op_pattern_insert : operation {
	int id;
	int index;
	zzub::pattern pattern;

	op_pattern_insert(int _id, int _index, zzub::pattern _pattern);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
	virtual void finish(zzub::song& song, bool send_events);
};

struct op_pattern_remove : operation {
	int id;
	int index;

	op_pattern_remove(int _id, int _index);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
	virtual void finish(zzub::song& song, bool send_events);
};

struct op_pattern_move : operation {
	int id;
	int index, newindex;

	op_pattern_move(int _id, int _index, int _newindex);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
	virtual void finish(zzub::song& song, bool send_events);
};

struct op_pattern_replace : operation {
	int id;
	int index;
	zzub::pattern pattern;

	op_pattern_replace(int _id, int _index, const zzub::pattern& _pattern);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
	virtual void finish(zzub::song& song, bool send_events);
};

struct op_pattern_insert_rows : operation {
	int id;
	int index;
	int row;
	std::vector<int> columns;
	int count;

	op_pattern_insert_rows(int _id, int _index, int _row, std::vector<int> _columns, int _count);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
	virtual void finish(zzub::song& song, bool send_events);
};

struct op_pattern_remove_rows : operation {
	int id;
	int index;
	int row;
	std::vector<int> columns;
	int count;

	op_pattern_remove_rows(int _id, int _index, int _row, std::vector<int> _columns, int _count);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
	virtual void finish(zzub::song& song, bool send_events);
};

struct op_sequencer_replace : operation {
	std::vector<zzub::sequencer_event> song_events;

	op_sequencer_replace(const std::vector<zzub::sequencer_event>& _events);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
	virtual void finish(zzub::song& song, bool send_events);
};

struct op_sequencer_create_track : operation {
	int id;

	op_sequencer_create_track(int _id);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
	virtual void finish(zzub::song& song, bool send_events);
};

struct op_sequencer_remove_track : operation {
	int track;

	op_sequencer_remove_track(int _track);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
	virtual void finish(zzub::song& song, bool send_events);
};

struct op_sequencer_move_track : operation {
	int track, newtrack;

	op_sequencer_move_track(int _track, int _newtrack);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
	virtual void finish(zzub::song& song, bool send_events);
};

struct op_sequencer_set_event : operation {
	int timestamp;
	int track;
	int action;

	op_sequencer_set_event(int _timestamp, int _track, int _action);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
	virtual void finish(zzub::song& song, bool send_events);
};

struct op_midimapping_insert : operation {
	zzub::midimapping midi_mapping;

	op_midimapping_insert(const zzub::midimapping& _midi_mapping);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
};

struct op_midimapping_remove : operation {
	int index;

	op_midimapping_remove(int _index);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
};

struct op_wavetable_wave_replace : operation {
	int wave;
	wave_info data;
	
	op_wavetable_wave_replace(int _wave, const wave_info& _data);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
	virtual void finish(zzub::song& song, bool send_events);
};

struct op_wavetable_add_wavelevel : operation {
	int wave;

	op_wavetable_add_wavelevel(int _wave);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
	virtual void finish(zzub::song& song, bool send_events);
};

struct op_wavetable_remove_wavelevel : operation {
	int wave;
	int level;

	op_wavetable_remove_wavelevel(int _wave, int _level);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
	virtual void finish(zzub::song& song, bool send_events);
};

struct op_wavetable_move_wavelevel : operation {
	int wave;
	int level, newlevel;

	op_wavetable_move_wavelevel(int _wave, int _level, int _newlevel);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
	virtual void finish(zzub::song& song, bool send_events);
};

struct op_wavetable_allocate_wavelevel : operation {
	int wave, level;
	int sample_count, channels, format;

	op_wavetable_allocate_wavelevel(int _wave, int _level, int _sample_count, int _channels, int _format);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
	virtual void finish(zzub::song& song, bool send_events);
};

struct op_wavetable_wavelevel_replace : operation {
	int wave, level;
	wave_level data;
	
	op_wavetable_wavelevel_replace(int _wave, int _level, const wave_level& _data);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
	virtual void finish(zzub::song& song, bool send_events);
};

struct op_wavetable_insert_sampledata : operation {
	int wave;
	int level;
	int pos;
	void* samples;
	int samples_length;
	wave_buffer_type samples_format;
	float samples_scale;
	int samples_channels;

	op_wavetable_insert_sampledata(int _wave, int _level, int _pos);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
	virtual void finish(zzub::song& song, bool send_events);
};

struct op_wavetable_remove_sampledata : operation {
	int wave;
	int level;
	int pos;
	int samples;

	op_wavetable_remove_sampledata(int _wave, int _level, int _pos, int _samples);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
	virtual void finish(zzub::song& song, bool send_events);
};


struct op_wavetable_convert_sampledata : operation {
	int wave;
	int level;

	int channels;
	int bits;

	op_wavetable_convert_sampledata(int _wave, int _level, int _channels, int _bits);
	virtual bool prepare(zzub::song& song);
	virtual bool operate(zzub::song& song);
	virtual void finish(zzub::song& song, bool send_events);
};

} // namespace zzub
