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

using std::pair;
using std::string;
using std::vector;

namespace zzub {

// graph serializing flags

struct operation_copy_plugin_flags {
	int plugin_id;
	bool copy_plugin;
	bool copy_patterns;

	operation_copy_plugin_flags() {
		plugin_id = -1;
		copy_plugin = false;
		copy_patterns = false;
	}
};

struct operation_copy_pattern_flags {
	int plugin_id;
	int index;
	bool copy_pattern;

	operation_copy_pattern_flags() {
		plugin_id = -1;
		index = -1;
		copy_pattern = false;
	}
};

struct operation_copy_wave_flags {
	int wave;
	bool copy_wave;

	operation_copy_wave_flags() {
		wave = -1;
		copy_wave = false;
	}
};

struct operation_copy_wavelevel_flags {
	int wave;
	int level;
	bool copy_samples;

	operation_copy_wavelevel_flags() {
		wave = -1;
		level = -1;
		copy_samples = false;
	}
};

struct operation_copy_flags {
	bool copy_graph;
	bool copy_work_order;
	bool copy_keyjazz;
	bool copy_midi_mappings;
	bool copy_sequencer_tracks;
	bool copy_wavetable;
	bool copy_song_variables;
	bool copy_plugins;
	bool copy_plugins_deep;

	vector<operation_copy_plugin_flags> plugin_flags;
	vector<operation_copy_pattern_flags> pattern_flags;
	vector<operation_copy_wave_flags> wave_flags;
	vector<operation_copy_wavelevel_flags> wavelevel_flags;

	operation_copy_flags() {
		copy_graph = false;
		copy_work_order = false;
		copy_keyjazz = false;
		copy_midi_mappings = false;
		copy_sequencer_tracks = false;
		copy_wavetable = false;
		copy_song_variables = false;
		copy_plugins = false;
		copy_plugins_deep = false;
	}

	void merge(const operation_copy_flags& flags);
	operation_copy_plugin_flags& get_plugin_flags(int id);
	const operation_copy_plugin_flags& get_plugin_flags(int id) const;
	operation_copy_pattern_flags& get_pattern_flags(int id, int index);
	operation_copy_wavelevel_flags& get_wavelevel_flags(int wave, int index);
	operation_copy_wave_flags& get_wave_flags(int wave);
};

struct undo_manager {
	typedef vector<operation*> ops;

	struct operationgroup {
		ops first;
		ops second;
		zzub_event_data_t* redo_event;
		zzub_event_data_t* undo_event;
	};

	typedef vector<operationgroup> undoableoperation;

	struct historyoperation {
		undoableoperation op;
		string description;
	};

	mixer front;
	song back;

	bool swap_mode;	// true = commit/execute are direct, false = commit/execute waits for poll
	synchronization::critical_section swap_lock;	// used when swap_mode = false
	vector<historyoperation> history;
	vector<historyoperation>::iterator history_position;
	historyoperation current_undoableoperation;
	ops backbuffer_operations;
	operationgroup backbuffer_opgroup;
	operation_copy_flags backbuffer_flags;
	synchronization::event swap_operations_signal;
	volatile bool swap_operations_commit;

	bool is_flushing;

	undo_manager();
	~undo_manager();
	void reset();
	void merge_backbuffer_flags(operation_copy_flags flags);
	bool prepare_operation_redo(operation* singleop);
	void prepare_operation_undo(operation* singleop);
	void flush_operations(zzub_event_data_t* do_event, zzub_event_data_t* redo_event, zzub_event_data_t* undo_event);

	bool execute_operation_redo(undoableoperation& undoableop);
	bool execute_operation_undo(undoableoperation& undoableop);
	//void execute_single_operation(operation* singleop);
	void write_swap_song(zzub::song& song, const operation_copy_flags& flags);
	void wait_swap_song_pointers();
	void clear_swap_song(zzub::song& song, const operation_copy_flags& flags);

	void commit_to_history(std::string description);
	void flush_from_history();
	void clear_history();
	void undo();
	void redo();

	void poll_operations();

	void free_operations(undoableoperation& op);
};

};
