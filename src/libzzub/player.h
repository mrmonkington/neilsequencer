/*
Copyright (C) 2003-2008 Anders Ervik <calvin@countzero.no>

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

using std::pair;
using std::string;
using std::vector;
using std::stack;

namespace zzub {


struct player : undo_manager, audioworker, midiworker {
	int work_buffer_position;						// sample position in current buffer

	input_plugincollection inputPluginCollection;
	output_plugincollection outputPluginCollection;
	recorder_plugincollection recorderPluginCollection;
	vector<pluginlib*> plugin_libraries;
	vector<string> plugin_folders;
	vector<const zzub::info*> plugin_infos;
	host_info hostinfo;
	thread_id_t user_thread_id;						// thread id as returned by GetCurrentThreadId or pthread_self

	player();
	virtual ~player(void);

	// initialization
	bool initialize();
	void initialize_plugin_libraries();
	void initialize_plugin_directory(string folder);
	void load_plugin_library(const string &fullpath);

	// audioworker
	virtual void work_stereo(int sample_count);
	virtual void audio_enabled();
	virtual void audio_disabled();
	virtual void samplerate_changed();
	
	// midiworker
	void midiEvent(unsigned short status, unsigned char data1, unsigned char data2);

	// helpers for plugin_flag_no_undo
	stack<bool> no_undo_stack;
	void begin_plugin_operation(int plugin_id);
	void end_plugin_operation(int plugin_id);

	// user methods (should be, but arent supported by begin_/commit_operation)
	void clear();
	void process_user_event_queue();
	void set_state(player_state state);
	void set_state_direct(player_state state);
	void plugin_set_parameter(int plugin_id, int group, int track, int column, int value, bool record, bool immediate, bool undoable);
	void play_plugin_note(int plugin_id, int note, int prevNote, int velocity);
	void reset_keyjazz();

	// user methods for creating compound, undoable operations
	// any of these must be enclosed by calls to begin_operation() and commit_operation().
	// all these methods operate on the intermediate backbuffer_song, which is committed
	// to the running graph after calling commit_operation().
	void clear_plugin(int id);
	void add_midimapping(int plugin_id, int group, int track, int param, int channel, int controller);
	void remove_midimapping(int plugin_id, int group, int track, int param);
	int create_plugin(std::vector<char>& bytes, string name, const zzub::info* loader, int flags);
	string plugin_get_new_name(string uri);
	const zzub::info* plugin_get_info(string uri);

	void plugin_destroy(int plugin_id);
	void plugin_set_name(int plugin_id, std::string name);
	void plugin_set_position(int plugin_id, float x, float y);
	void plugin_set_track_count(int plugin_id, int count);
	void plugin_add_pattern(int plugin_id, const zzub::pattern& pattern);
	void plugin_remove_pattern(int plugin_id, int pattern);
	void plugin_move_pattern(int plugin_id, int pattern, int newindex);
	void plugin_update_pattern(int plugin_id, int index, const zzub::pattern& pattern);
	void plugin_set_pattern_name(int plugin_id, int index, std::string name);
	void plugin_set_pattern_length(int plugin_id, int index, int rows);
	void plugin_set_pattern_value(int plugin_id, int index, int group, int track, int column, int row, int value);
	void plugin_insert_pattern_rows(int plugin_id, int pattern, int* column_indices, int num_indices, int start, int rows);
	void plugin_remove_pattern_rows(int plugin_id, int pattern, int* column_indices, int num_indices, int start, int rows);
	bool plugin_add_input(int to_id, int from_id, connection_type type);
	void plugin_delete_input(int to_id, int from_id, connection_type type);
	void plugin_set_midi_connection_device(int to_id, int from_id, std::string name);
	void plugin_add_event_connection_binding(int to_id, int from_id, int sourceparam, int targetgroup, int targettrack, int targetparam);
	void plugin_remove_event_connection_binding(int to_id, int from_id, int index);
	void plugin_set_stream_source(int plugin_id, std::string data_url);
	void sequencer_add_track(int plugin_id);
	void sequencer_remove_track(int index);
	void sequencer_move_track(int index, int newindex);
	void sequencer_set_event(int track, int pos, int value);
	void sequencer_insert_events(int track, int start, int ticks);
	void sequencer_remove_events(int track, int start, int ticks);

	void wave_set_name(int wave, std::string name);
	void wave_set_path(int wave, std::string name);
	void wave_set_volume(int wave, float volume);
	void wave_set_flags(int wave, int flags);
	void wave_add_level(int wave);
	void wave_remove_level(int wave, int level);
	void wave_move_level(int wave, int level, int newlevel);
	void wave_allocate_level(int wave, int level, int sample_count, int channels, wave_buffer_type format);
	void wave_clear_level(int wave, int level);
	void wave_clear(int wave);
	void wave_set_samples(int wave, int level, int sample_count, int channels, int format, void* bytes);
	void wave_insert_samples(int wave, int level, int target_offset, int sample_count, int channels, wave_buffer_type format, void* bytes);
	void wave_remove_samples(int wave, int level, int target_offset, int sample_count);
	void wave_set_root_note(int wave, int level, int note);
	void wave_set_samples_per_second(int wave, int level, int sps);
	void wave_set_loop_begin(int wave, int level, int loop_begin);
	void wave_set_loop_end(int wave, int level, int loop_end);

	int wave_load_sample(int wave, int level, int offset, bool clear, std::string name, zzub::instream* datastream);
	void wave_set_envelopes(int wave, const vector<zzub::envelope_entry>& _envelopes);
};


};
