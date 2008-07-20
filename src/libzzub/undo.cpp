#include "common.h"

namespace zzub {

/***

	operation_copy_flags

***/

operation_copy_plugin_flags& operation_copy_flags::get_plugin_flags(int id) {
	for (size_t i = 0; i < plugin_flags.size(); i++) {
		if (plugin_flags[i].plugin_id == id) 
			return plugin_flags[i];
	}
	operation_copy_plugin_flags flags;
	flags.plugin_id = id;
	plugin_flags.push_back(flags);
	return plugin_flags.back();
}

const operation_copy_plugin_flags& operation_copy_flags::get_plugin_flags(int id) const {
	for (size_t i = 0; i < plugin_flags.size(); i++) {
		if (plugin_flags[i].plugin_id == id) 
			return plugin_flags[i];
	}
	static operation_copy_plugin_flags flags;
	flags.plugin_id = id;
	return flags;
}

operation_copy_pattern_flags& operation_copy_flags::get_pattern_flags(int id, int index) {
	for (size_t i = 0; i < pattern_flags.size(); i++)
		if (pattern_flags[i].plugin_id == id && pattern_flags[i].index == index) 
			return pattern_flags[i];
	operation_copy_pattern_flags flags;
	flags.plugin_id = id;
	flags.index = index;
	pattern_flags.push_back(flags);
	return pattern_flags.back();
}

operation_copy_wavelevel_flags& operation_copy_flags::get_wavelevel_flags(int wave, int level) {
	for (size_t i = 0; i < wavelevel_flags.size(); i++) {
		if (wavelevel_flags[i].wave == wave && wavelevel_flags[i].level == level) 
			return wavelevel_flags[i];
	}
	operation_copy_wavelevel_flags flags;
	flags.wave = wave;
	flags.level = level;
	wavelevel_flags.push_back(flags);
	return wavelevel_flags.back();
}

operation_copy_wave_flags& operation_copy_flags::get_wave_flags(int wave) {
	for (size_t i = 0; i < wave_flags.size(); i++) {
		if (wave_flags[i].wave == wave) 
			return wave_flags[i];
	}
	operation_copy_wave_flags flags;
	flags.wave = wave;
	wave_flags.push_back(flags);
	return wave_flags.back();
}

void operation_copy_flags::merge(const operation_copy_flags& flags) {
	copy_graph |= flags.copy_graph;
	copy_keyjazz |= flags.copy_keyjazz;
	copy_midi_mappings |= flags.copy_midi_mappings;
	copy_plugins |= flags.copy_plugins;
	copy_plugins_deep |= flags.copy_plugins_deep;
	copy_sequencer_tracks |= flags.copy_sequencer_tracks;
	copy_song_variables |= flags.copy_song_variables;
	copy_wavetable |= flags.copy_wavetable;
	copy_work_order |= flags.copy_work_order;

	for (size_t i = 0; i < flags.plugin_flags.size(); i++) {
		const operation_copy_plugin_flags& sflags = flags.plugin_flags[i];
		operation_copy_plugin_flags& tflags = get_plugin_flags(sflags.plugin_id);
		tflags.copy_plugin |= sflags.copy_plugin;
		tflags.copy_patterns |= sflags.copy_patterns;
	}

	for (size_t i = 0; i < flags.wave_flags.size(); i++) {
		const operation_copy_wave_flags& sflags = flags.wave_flags[i];
		operation_copy_wave_flags& tflags = get_wave_flags(sflags.wave);
		tflags.copy_wave |= sflags.copy_wave;
	}

	for (size_t i = 0; i < flags.wavelevel_flags.size(); i++) {
		const operation_copy_wavelevel_flags& sflags = flags.wavelevel_flags[i];
		operation_copy_wavelevel_flags& tflags = get_wavelevel_flags(sflags.wave, sflags.level);
		tflags.copy_samples |= sflags.copy_samples;
	}

	for (size_t i = 0; i < flags.pattern_flags.size(); i++) {
		const operation_copy_pattern_flags& sflags = flags.pattern_flags[i];
		operation_copy_pattern_flags& tflags = get_pattern_flags(sflags.plugin_id, sflags.index);
		tflags.copy_pattern |= sflags.copy_pattern;
	}
}


/***

	undo_manager

***/

undo_manager::undo_manager() {
	swap_mode = false;
	is_flushing = false;
	ignore_undo = false;
}

undo_manager::~undo_manager() {
	clear_history();
}

void undo_manager::free_operations(undoableoperation& op) {
	for (size_t i = 0; i < (int)op.size(); i++) {
		for (size_t j = 0; j < op[i].first.size(); j++) {
			delete op[i].first[j];
		}
		for (size_t j = 0; j < op[i].second.size(); j++) {
			delete op[i].second[j];
		}
		delete op[i].redo_event;
		delete op[i].undo_event;
	}
	op.clear();
}

void undo_manager::poll_operations() {
	if (swap_operations_commit) {
		write_swap_song(back, backbuffer_flags);
		for (size_t i = 0; i < backbuffer_operations.size(); i++) {
			backbuffer_operations[i]->operate(front);
		}

		swap_operations_commit = false;
		swap_operations_signal.signal();
	}
}

bool undo_manager::execute_operation_redo(undoableoperation& undoableop) {
	bool result = false;

	for (size_t i = 0; i < undoableop.size(); i++) {
		operationgroup& io = undoableop[i];
		for (size_t j = 0; j < io.first.size(); j++) {
			merge_backbuffer_flags(io.first[j]->copy_flags);
			if (io.first[j]->prepare(back)) {
				result = true;
				backbuffer_operations.push_back(io.first[j]);
			} else {
				// redo must never fail
				assert(false);
			}
		}
	}

	wait_swap_song_pointers();

	// now that all changes are written, we can invoke events in operation::finish().
	// these events can in turn begin and commit new operations, so we need to clear the
	// backbuffer and prepare for new operations before invoking operation::finish():

	reset();

	for (size_t i = 0; i < undoableop.size(); i++) {
		operationgroup& io = undoableop[i];
		for (size_t j = 0; j < io.first.size(); j++) {
			io.first[j]->finish(front, io.redo_event == 0);
		}
		if (io.redo_event) front.plugin_invoke_event(0, *io.redo_event, true);
	}

	return result;
}


bool undo_manager::execute_operation_undo(undoableoperation& undoableop) {
	bool result = false;

//	begin_operation();

	for (size_t i = 0; i < undoableop.size(); i++) {
		operationgroup& io = undoableop[undoableop.size() - 1 - i];
		for (size_t j = 0; j < io.second.size(); j++) {
			merge_backbuffer_flags(io.second[j]->copy_flags);
			if (io.second[j]->prepare(back)) {
				result = true;
				backbuffer_operations.push_back(io.second[j]);
			}
		}
	}
	wait_swap_song_pointers();

	// now that all changes are written, we can invoke events in operation::finish().
	// these events can in turn begin and commit new operations, so we need to clear the
	// backbuffer and prepare for new operations before invoking operation::finish():

	reset();

	for (size_t i = 0; i < undoableop.size(); i++) {
		operationgroup& io = undoableop[undoableop.size() - 1 - i];
		for (size_t j = 0; j < io.second.size(); j++) {
			io.second[j]->finish(front, io.undo_event == 0);
		}
		if (io.undo_event) front.plugin_invoke_event(0, *io.undo_event, true);
	}

	return result;
}

void undo_manager::wait_swap_song_pointers() {
	if (swap_mode) {
		swap_operations_commit = true;
		swap_operations_signal.wait();
		clear_swap_song(back, backbuffer_flags);
	} else {
		swap_lock.lock();
		write_swap_song(back, backbuffer_flags);
		for (size_t i = 0; i < backbuffer_operations.size(); i++) {
			backbuffer_operations[i]->operate(front);
		}
		clear_swap_song(back, backbuffer_flags);
		swap_lock.unlock();
	}
}

void undo_manager::commit_to_history(std::string description) {
	
	if (current_undoableoperation.op.empty()) return ;

	if (history_position != history.end()) {
		history.erase(history_position, history.end());
		history_position = history.end();
	}

	current_undoableoperation.description = description;
	history.push_back(current_undoableoperation);

	current_undoableoperation.op.clear();
	current_undoableoperation.description = "";

	history_position = history.end();
}

void undo_manager::flush_from_history() {

	if (current_undoableoperation.op.empty()) return ;

	// same as commit_to_history, except the last operations arent undoable
	if (history_position != history.end()) {
		int start = (int)(history_position - history.begin());
		for (size_t i = start; i < history.size(); i++) {
			free_operations(history[i].op);
		}
		history.erase(history_position, history.end());
	}

	free_operations(current_undoableoperation.op);
	current_undoableoperation.op.clear();
	current_undoableoperation.description = "";

	history_position = history.end();
}

void undo_manager::clear_history() {
	free_operations(current_undoableoperation.op);
	current_undoableoperation.op.clear();
	current_undoableoperation.description = "";

	for (size_t i = 0; i < history.size(); i++) {
		free_operations(history[i].op);
	}
	history.clear();
	history_position = history.begin();
}

void undo_manager::undo() {
	if (current_undoableoperation.op.size() > 0)
		commit_to_history("(leftover from last operation)");

	if (history_position == history.begin()) return ;
	history_position--;

	execute_operation_undo(history_position->op);
}

void undo_manager::redo() {
	if (history_position == history.end()) return ;

	execute_operation_redo(history_position->op);
	history_position++;
}

void undo_manager::reset() {
	back = zzub::song();
	backbuffer_flags = operation_copy_flags();
	backbuffer_opgroup.first.clear();
	backbuffer_opgroup.second.clear();
	backbuffer_opgroup.redo_event = 0;
	backbuffer_opgroup.undo_event = 0;

	backbuffer_operations.clear();
}


bool undo_manager::prepare_operation_redo(operation* singleop) {
	// check for new backbuffer_flags and copy data if necessary!
	merge_backbuffer_flags(singleop->copy_flags);

	// run the preparatory parts of this operation now
	if (!singleop->prepare(back))
		return false;

	// make sure this operation is added to the redo-buffer
	if (!ignore_undo)
		backbuffer_opgroup.first.push_back(singleop);

	// make sure the remaining parts of this operation is invoked on the player thread
	backbuffer_operations.push_back(singleop);
	return true;
}

void undo_manager::prepare_operation_undo(operation* singleop) {
	// make sure this operation is added to the undo-buffer
	// we assume undo operations are added in reverse order, so we put each element IN FRONT
	// we _could_ have looped backwards, but right now we have two mechanisms for this, better not get in the way
	if (!ignore_undo)
		backbuffer_opgroup.second.insert(backbuffer_opgroup.second.begin(), singleop);
}

// this is now flush_operations
void undo_manager::flush_operations(zzub_event_data_t* do_event, zzub_event_data_t* redo_event, zzub_event_data_t* undo_event) {
	assert(!is_flushing);
	is_flushing = true;
	if (backbuffer_operations.size() > 0) {
		// completes the last compound operation by swapping song pointers and stuff
		wait_swap_song_pointers();
	}

	// now that all changes are written, we can invoke events in operation::finish().
	// these events can in turn begin and commit new operations, so we need to clear the
	// backbuffer and prepare for new operations before invoking operation::finish():

	// make a copy of the operations we just swapped in
	ops backbuffer_operations_copy = backbuffer_operations;

	if (backbuffer_opgroup.first.size() > 0 || backbuffer_opgroup.second.size() > 0) {

		// add to undo-buffer
		backbuffer_opgroup.redo_event = redo_event;
		backbuffer_opgroup.undo_event = undo_event;
		current_undoableoperation.op.push_back(backbuffer_opgroup);
	}

	// clear stuff
	reset();

	is_flushing = false;

	// invoke the alternate redo-event if one were given
	if (do_event) {
		front.plugin_invoke_event(0, *do_event, true);
	}

	// finish here:
	for (size_t j = 0; j < backbuffer_operations_copy.size(); j++) {
		backbuffer_operations_copy[j]->finish(front, do_event == 0);
	}
}

void undo_manager::merge_backbuffer_flags(operation_copy_flags flags) {

	if (!backbuffer_flags.copy_graph && flags.copy_graph)
		back.graph = front.graph;

//	if (!backbuffer_flags.copy_keyjazz && flags.copy_keyjazz)
//		back.keyjazz = front.keyjazz;

	if (!backbuffer_flags.copy_midi_mappings && flags.copy_midi_mappings)
		back.midi_mappings = front.midi_mappings;

	if (!backbuffer_flags.copy_plugins && flags.copy_plugins)
		back.plugins = front.plugins;

	if (!backbuffer_flags.copy_sequencer_tracks && flags.copy_sequencer_tracks) {
		back.sequencer_tracks = front.sequencer_tracks;
	}

	if (!backbuffer_flags.copy_song_variables && flags.copy_song_variables) {
		back.song_begin = front.song_begin;
		back.song_end = front.song_end;
		back.song_loop_begin = front.song_loop_begin;
		back.song_loop_end = front.song_loop_end;
		back.song_loop_enabled = front.song_loop_enabled;
	}

	if (!backbuffer_flags.copy_wavetable && flags.copy_wavetable)
		back.wavetable = front.wavetable;

	if (!backbuffer_flags.copy_work_order && flags.copy_work_order)
		back.work_order = front.work_order;

	// if player_flags_copy_plugins_deep is set we generate flags to copy all the plugins
	if (!backbuffer_flags.copy_plugins_deep && flags.copy_plugins_deep) {
		assert(flags.copy_plugins);
		for (int i = 0; i < (int)front.plugins.size(); i++)
			if (front.plugins[i] != 0) 
				flags.get_plugin_flags(i).copy_plugin = true;
	}

	for (size_t i = 0; i < flags.plugin_flags.size(); i++) {
		const operation_copy_plugin_flags& pflags = flags.plugin_flags[i];
		operation_copy_plugin_flags& bflags = backbuffer_flags.get_plugin_flags(pflags.plugin_id);

		assert(flags.copy_plugins);
		assert(pflags.copy_plugin);	// must be set on incoming plugin flags

		// front has a null plugin during creation here which is ok
		if (pflags.plugin_id >= front.plugins.size() || front.plugins[pflags.plugin_id] == 0) continue;

		if (!bflags.copy_plugin && pflags.copy_plugin) {
			const metaplugin& sp = *front.plugins[pflags.plugin_id];
			back.plugins[pflags.plugin_id] = new metaplugin(sp);
		}

		metaplugin& tp = *back.plugins[pflags.plugin_id];

		if (!bflags.copy_patterns && pflags.copy_patterns) {
			// duplicate all patterns
			for (size_t i = 0; i < tp.patterns.size(); i++) {
				tp.patterns[i] = new pattern(*tp.patterns[i]);
			}
		}
	}

	// in case a wavelevel is flagged, we also add flags to copy the wave_info
	for (size_t i = 0; i < flags.wavelevel_flags.size(); i++) {
		assert(flags.copy_wavetable);
		operation_copy_wavelevel_flags& wlflags = flags.wavelevel_flags[i];
		assert(wlflags.wave != -1);
		flags.get_wave_flags(wlflags.wave).copy_wave = true;
	}

	for (size_t i = 0; i < flags.wave_flags.size(); i++) {
		const operation_copy_wave_flags& wflags = flags.wave_flags[i];
		operation_copy_wave_flags& bflags = backbuffer_flags.get_wave_flags(wflags.wave);

		assert(wflags.wave != -1);
		assert(flags.copy_wavetable);
		assert(wflags.copy_wave);

		if (!bflags.copy_wave && wflags.copy_wave) {
			const wave_info_ex& sw = *front.wavetable.waves[wflags.wave];
			back.wavetable.waves[wflags.wave] = new wave_info_ex(sw);
		}
	}

	for (size_t i = 0; i < flags.wavelevel_flags.size(); i++) {
		const operation_copy_wavelevel_flags& wflags = flags.wavelevel_flags[i];
		operation_copy_wavelevel_flags& bflags = backbuffer_flags.get_wavelevel_flags(wflags.wave, wflags.level);

		assert(flags.copy_wavetable);
		assert(wflags.copy_samples);

		if (!bflags.copy_samples && wflags.copy_samples) {
			// make a copy of the sample data here, access via the backbuffer since we have forced a copy of it via wave_flags
			wave_info_ex& sw = *back.wavetable.waves[wflags.wave];
			if (wflags.level < sw.levels.size()) {
				int bytes_per_sample = sw.get_bytes_per_sample(wflags.level);
				int channels = sw.get_stereo() ? 2 : 1;
				int extended_bytes = sw.get_extended() ? 8 : 0;
				int numsamples = sw.get_sample_count(wflags.level);
				int sample_bytes = bytes_per_sample * channels * numsamples + extended_bytes;
				void* newsamples = new char[sample_bytes];
				memcpy(newsamples, sw.levels[wflags.level].legacy_sample_ptr, sample_bytes);
				sw.levels[wflags.level].legacy_sample_ptr = (short*)newsamples;
				sw.levels[wflags.level].samples = (short*)(((char*)newsamples) + extended_bytes);
			}
		}
	}

	for (size_t i = 0; i < flags.pattern_flags.size(); i++) {
		const operation_copy_pattern_flags& pflags = flags.pattern_flags[i];
		operation_copy_pattern_flags& bflags = backbuffer_flags.get_pattern_flags(pflags.plugin_id, pflags.index);

		// check if this pattern was already copied via the plugin before we do anything
		operation_copy_plugin_flags& pluginflags = backbuffer_flags.get_plugin_flags(pflags.plugin_id);
		if (pluginflags.copy_patterns) continue;

		metaplugin& tp = *back.plugins[pflags.plugin_id];
		if (!bflags.copy_pattern && pflags.copy_pattern) {
			tp.patterns[pflags.index] = new pattern(*tp.patterns[pflags.index]);
		}
	}

	backbuffer_flags.merge(flags);
}

void undo_manager::write_swap_song(zzub::song& song, const operation_copy_flags& flags) {
	if (flags.copy_graph)
		front.graph = song.graph;

//	if (flags.copy_keyjazz)
//		front.keyjazz.swap(song.keyjazz);

	if (flags.copy_midi_mappings)
		front.midi_mappings.swap(song.midi_mappings);

	if (flags.copy_plugins)
		front.plugins.swap(song.plugins);

	bool update_seq_pos = false;
	if (flags.copy_sequencer_tracks) {
		front.sequencer_tracks.swap(song.sequencer_tracks);
		update_seq_pos = true;
	}

	if (update_seq_pos) {
		// the sequencer was modified - update internal sequencer states
		front.sequencer_update_play_pattern_positions();
		for (size_t i = 0; i < front.sequencer_indices.size(); i++) {
			if (front.sequencer_indices[i] >= front.sequencer_tracks[i].events.size())
				front.sequencer_indices[i] = front.sequencer_tracks[i].events.size() - 1;
		}
	}

	if (flags.copy_song_variables) {
		front.song_begin = song.song_begin;
		front.song_end = song.song_end;
		front.song_loop_begin = song.song_loop_begin;
		front.song_loop_end = song.song_loop_end;
		front.song_loop_enabled = song.song_loop_enabled;
	}

	if (flags.copy_wavetable) {
		front.wavetable.waves.swap(song.wavetable.waves);
	}

	if (flags.copy_work_order)
		front.work_order.swap(song.work_order);
}

void undo_manager::clear_swap_song(zzub::song& song, const operation_copy_flags& flags) {

	// TODO: the following code is cleanup code and does not belong in the player thread!
	// maybe in reset()? or in wait_swap_pointers(), or maybe all these belong in one method?

	for (size_t i = 0; i < flags.plugin_flags.size(); i++) {
		assert(flags.copy_plugins);

		const operation_copy_plugin_flags& pflags = flags.plugin_flags[i];

		// the copy flag can be set for a newly created plugin:
		if (pflags.plugin_id >= song.plugins.size() || song.plugins[pflags.plugin_id] == 0) continue;

		const metaplugin& sp = *song.plugins[pflags.plugin_id];

		if (pflags.copy_patterns) {
			// delete (swapped) duplicated patterns
			for (size_t i = 0; i < sp.patterns.size(); i++) {
				delete sp.patterns[i];
			}
		}

		delete &sp;
	}

	for (size_t i = 0; i < flags.wavelevel_flags.size(); i++) {
		assert(flags.copy_wavetable);

		const operation_copy_wavelevel_flags& wflags = flags.wavelevel_flags[i];
		if (wflags.copy_samples)
			if (wflags.level < song.wavetable.waves[wflags.wave]->levels.size())
				delete[] song.wavetable.waves[wflags.wave]->levels[wflags.level].legacy_sample_ptr;
	}

	for (size_t i = 0; i < flags.wave_flags.size(); i++) {
		assert(flags.copy_wavetable);

		const operation_copy_wave_flags& wflags = flags.wave_flags[i];
		if (wflags.copy_wave)
			delete song.wavetable.waves[wflags.wave];
	}

	for (size_t i = 0; i < flags.pattern_flags.size(); i++) {
		const operation_copy_pattern_flags& pflags = flags.pattern_flags[i];
		//operation_copy_pattern_flags& bflags = flags.get_pattern_flags(pflags.plugin_id, pflags.index);

		// check if this pattern was already copied via the plugin before we do anything
		const operation_copy_plugin_flags& pluginflags = flags.get_plugin_flags(pflags.plugin_id);
		if (pluginflags.copy_patterns) continue;

		metaplugin& tp = *back.plugins[pflags.plugin_id];
		delete tp.patterns[pflags.index];
	}

}

};
