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

#include "common.h"
#include <deque>
#include <functional>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <sstream>
#include "bmxreader.h"
#include "bmxwriter.h"
#include "timer.h"
#include "dummy.h"
#include "archive.h"
#include "tools.h"

#if defined(POSIX)
#include <dirent.h>
#include <sys/stat.h>

#define strcmpi strcasecmp
#endif

#include "sseoptimization.h"

using std::deque;
using std::cerr;
using std::endl;


namespace {	// duplicate from ccm.h and pattern.cpp

	int midi_to_buzz_note(int value) {
	return ((value / 12) << 4) + (value % 12) + 1;
}

int buzz_to_midi_note(int value) {
	return 12 * (value >> 4) + (value & 0xf) - 1;
}

}

namespace zzub {

struct find_note_column : public std::unary_function<const zzub::parameter*, bool> {
	bool operator()(const zzub::parameter* param) {
		return param->type == zzub::parameter_type_note;
	}
};

struct find_velocity_column : public std::unary_function<const zzub::parameter*, bool> {
	bool operator()(const zzub::parameter* param) {
		return param->description != 0 && ( (strstr(param->description, "Velocity")) || (strstr(param->description, "Volume")) );
	}
};

bool get_note_info(const zzub::info* info, int& note_group, int& note_column) {
	std::vector<const zzub::parameter*>::const_iterator param;

	param = find_if(info->global_parameters.begin(), info->global_parameters.end(), find_note_column());
	if (param != info->global_parameters.end()) {
		note_group = 1;
		note_column = int(param - info->global_parameters.begin());
		return true;
	}

	param = find_if(info->track_parameters.begin(), info->track_parameters.end(), find_note_column());
	if (param != info->track_parameters.end()) {
		note_group = 2;
		note_column = int(param - info->track_parameters.begin());
		return true;
	}

	return false;
}

bool get_velocity_info(const zzub::info* info, int note_group, int& velocity_column) {
	std::vector<const zzub::parameter*>::const_iterator param;

	if(note_group == 1)
	{
		param = find_if(info->global_parameters.begin(), info->global_parameters.end(), find_velocity_column());
		if (param != info->global_parameters.end()) {
			velocity_column = int(param - info->global_parameters.begin());
			return true;
		}
	}

	param = find_if(info->track_parameters.begin(), info->track_parameters.end(), find_velocity_column());
	if (param != info->track_parameters.end()) {
		velocity_column = int(param - info->track_parameters.begin());
		return true;
	}

	return false;
}

bool is_note_playing(int plugin_id, const std::vector<zzub::keyjazz_note>& keyjazz, int note) {
	for (size_t i = 0; i < keyjazz.size(); i++)
		if (keyjazz[i].plugin_id == plugin_id && keyjazz[i].note == note) return true;
	return false;
}


inline bool scanPeakStereo(float* l, float* r, int numSamples, float& maxL, float& maxR, float falloff) {
	bool zero = true;
	while (numSamples--) {
		maxL *= falloff;
		maxR *= falloff;
		maxL = std::max(std::abs(*l), maxL);
		maxR = std::max(std::abs(*r), maxR);
		if (zero && ((*l > SIGNAL_TRESHOLD)||(*l < -SIGNAL_TRESHOLD)||
			(*r > SIGNAL_TRESHOLD)||(*r < -SIGNAL_TRESHOLD)))
			zero = false;
		l++;r++;
	}
	return zero;
}

// http://en.wikipedia.org/wiki/Topological_sorting
void topological_sort_kahn(plugin_map& tg, std::deque<plugin_descriptor>& input, std::vector<plugin_descriptor>& result) {
	while (input.size()) {
		plugin_descriptor n = input.front();
		input.pop_front();
		result.push_back(n);
		graph_traits<plugin_map>::out_edge_iterator out, out_end;
		boost::tie(out, out_end) = out_edges(n, tg);
		deque<plugin_descriptor> adj;
		for (graph_traits<plugin_map>::out_edge_iterator i = out; i != out_end; ++i) {
			adj.push_back(target(*i, tg));
		}
		for (deque<plugin_descriptor>::iterator i = adj.begin(); i != adj.end(); ++i) {
			remove_edge(n, *i, tg);
			graph_traits<plugin_map>::in_edge_iterator in, in_end;
			boost::tie(in, in_end) = in_edges(*i, tg);
			if ((in_end - in) == 0) {
				input.push_back(*i);
			}
		}
	}
}

/***

	song

***/

song::song() {
	state = player_state_muted;

	song_begin = 0;
	song_end = 16;
	song_loop_begin = 0;
	song_loop_end = 16;
	song_loop_enabled = true;

	midi_plugin = -1;

	user_event_queue.resize(4096);
	user_event_queue_read = user_event_queue_write = 0;
}

zzub::metaplugin& song::get_plugin(zzub::plugin_descriptor index) {
	assert(index >= 0 && index < num_vertices(graph));
	int id = graph[index].id;
	assert(id >= 0 && (size_t)id < plugins.size());
	return *plugins[id];
}

int song::get_plugin_count() {
	return (int)num_vertices(graph);
}

zzub::plugin_descriptor song::get_plugin_descriptor(std::string name) {
	plugin_iterator vi;
	for (vi = vertices(graph).first; vi != vertices(graph).second; ++vi)
		if (get_plugin(*vi).name == name) return *vi;

	return graph_traits<plugin_map>::null_vertex();
}

int song::get_plugin_id(zzub::plugin_descriptor index) {
	assert(index >= 0 && index < num_vertices(graph));
	return graph[index].id;
}

int song::plugin_get_parameter_count(int plugin_id, int group, int track) {
	const zzub::info* loader = plugins[plugin_id]->info;
	switch (group) {
		case 0: { // input connections
			connection* conn = plugin_get_input_connection(plugin_id, track);
			return (int)conn->connection_parameters.size();
		}
		case 1: // globals
			return (int)loader->global_parameters.size();
		case 2: // track params
			return (int)loader->track_parameters.size();
		case 3: // controller params
			return (int)loader->controller_parameters.size();
		default:
			return 0;
	}
}

int song::plugin_get_track_count(int plugin_id, int group) {
	const zzub::info* loader = plugins[plugin_id]->info;
	switch (group) {
		case 0:
			return plugin_get_input_connection_count(plugin_id);
		case 1:
			return 1;
		case 2:
			return plugins[plugin_id]->tracks;
		case 3:
		default:
			assert(false);
			return 0;
	}
}


const parameter* song::plugin_get_parameter_info(int plugin_id, int group, int track, int column) {
	const zzub::info* loader = plugins[plugin_id]->info;
	switch (group) {
		case 0: { // input connections
			connection* conn = plugin_get_input_connection(plugin_id, track);
			//std::pair<zzub::connection_descriptor, bool> conndesc = plugin_get_input_connection(plugin_id, track);
			//edge_props& conn = graph[conndesc.first];
			return conn->connection_parameters[column];
		}
		case 1: // globals
			return loader->global_parameters[column];
		case 2: // track params
			return loader->track_parameters[column];
		case 3: // controller params
			return loader->controller_parameters[column];
		default:
			return 0;
	}
}

int song::plugin_get_parameter(int plugin_id, int group, int track, int column) {
	// use state_write or state_last depending on what is freshest
	assert(plugins[plugin_id] != 0);
	assert(group >= 0 && group < plugins[plugin_id]->state_write.groups.size());
	assert(track >= 0 && track < plugins[plugin_id]->state_write.groups[group].size());
	assert(column >= 0 && column < plugins[plugin_id]->state_write.groups[group][track].size());

	int v = plugins[plugin_id]->state_write.groups[group][track][column][0];
	const zzub::parameter* param = plugin_get_parameter_info(plugin_id, group, track, column);
	if (v != param->value_none) return v;

	return plugins[plugin_id]->state_last.groups[group][track][column][0];
}

int song::plugin_get_parameter_direct(int plugin_id, int group, int track, int column) {
	assert(plugins[plugin_id] != 0);
	assert(group >= 0 && group < plugins[plugin_id]->state_write.groups.size());
	assert(track >= 0 && track < plugins[plugin_id]->state_write.groups[group].size());
	assert(column >= 0 && column < plugins[plugin_id]->state_write.groups[group][track].size());

	return plugins[plugin_id]->state_write.groups[group][track][column][0];
}

void song::plugin_set_parameter_direct(int plugin_id, int group, int track, int column, int value, bool record) {
	assert(plugins[plugin_id] != 0);
	assert(group >= 0 && group < plugins[plugin_id]->state_write.groups.size());
	assert(track >= 0 && track < plugins[plugin_id]->state_write.groups[group].size());
	assert(column >= 0 && column < plugins[plugin_id]->state_write.groups[group][track].size());

	plugins[plugin_id]->state_write.groups[group][track][column][0] = value;
	if (record)
		plugins[plugin_id]->state_automation.groups[group][track][column][0] = value;
}

zzub::info* song::create_dummy_info(int flags, std::string pluginUri, int attributes, int globalValues, int trackValues, parameter* params) {

	dummy_info* new_info = new dummy_info();

	new_info->flags |= flags;
	new_info->uri = pluginUri;
	
	for (int i = 0; i < attributes; ++i) {
		attribute& a = new_info->add_attribute();
	}

	// copy incoming params because they wont be valid after loading ends
	for (int i = 0; i < globalValues; ++i) {
		parameter &p = new_info->add_global_parameter();
		p = *params;
		char* name = new char[strlen(params->name)+1];
		strcpy(name, params->name);
		p.name = name;
		params++;
	}
	for (int i = 0; i < trackValues; ++i) {
		parameter &p = new_info->add_track_parameter();
		p = *params;
		char* name = new char[strlen(params->name)+1];
		strcpy(name, params->name);
		p.name = name;
		params++;
	}

	return new_info;
}

int song::plugin_get_input_connection_count(int to_id) {
	assert(to_id >= 0 && to_id < plugins.size());
	assert(plugins[to_id] != 0);

	plugin_descriptor to_plugin = plugins[to_id]->descriptor;
	zzub::out_edge_iterator out, out_end;
	boost::tie(out, out_end) = out_edges(to_plugin, graph);
	return int(out_end - out);
}

int song::plugin_get_input_connection_plugin(int plugin_id, int index) {
	assert(plugin_id >= 0 && plugin_id < plugins.size());
	assert(plugins[plugin_id] != 0);

	plugin_descriptor to_plugin = plugins[plugin_id]->descriptor;
	
	zzub::out_edge_iterator out, out_end;
	boost::tie(out, out_end) = out_edges(to_plugin, graph);
	assert(index < (out_end - out));

	plugin_descriptor from_plugin = target(*(out+index), graph);
	return graph[from_plugin].id;
}

connection_type song::plugin_get_input_connection_type(int plugin_id, int index) {
	connection* conn = plugin_get_input_connection(plugin_id, index);
	assert(conn);
	return conn->type;
}

connection* song::plugin_get_input_connection(int plugin_id, int index) {
	assert(plugin_id >= 0 && plugin_id < plugins.size());
	assert(plugins[plugin_id] != 0);

	plugin_descriptor to_plugin = plugins[plugin_id]->descriptor;
	
	zzub::out_edge_iterator out, out_end;
	boost::tie(out, out_end) = out_edges(to_plugin, graph);
	assert(index < (out_end - out));

	edge_props& c = graph[*(out + index)];
	return c.conn;
}

int song::plugin_get_input_connection_index(int plugin_id, int from_id, connection_type type) {
	assert(plugin_id >= 0 && plugin_id < plugins.size());
	assert(from_id >= 0 && from_id < plugins.size());
	assert(plugins[plugin_id] != 0);
	assert(plugins[from_id] != 0);

	plugin_descriptor to_plugin = plugins[plugin_id]->descriptor;
	plugin_descriptor from_plugin = plugins[from_id]->descriptor;
	out_edge_iterator out, out_end;
	boost::tie(out, out_end) = out_edges(to_plugin, graph);
	for (out_edge_iterator i = out; i != out_end; ++i) {
		if (target(*i, graph) == from_plugin && graph[*i].conn->type == type) return (int)(i - out);
	}
	return -1;
}

int song::plugin_get_output_connection_count(int to_id) {
	assert(to_id >= 0 && to_id < plugins.size());
	assert(plugins[to_id] != 0);

	plugin_descriptor to_plugin = plugins[to_id]->descriptor;
	in_edge_iterator out, out_end;
	boost::tie(out, out_end) = in_edges(to_plugin, graph);
	return int(out_end - out);
}

connection* song::plugin_get_output_connection(int plugin_id, int index) {
	plugin_descriptor to_plugin = plugins[plugin_id]->descriptor;
	
	in_edge_iterator out, out_end;
	boost::tie(out, out_end) = in_edges(to_plugin, graph);
	assert(index < (out_end - out));

	edge_props& c = graph[*(out + index)];
	return c.conn;
}

int song::plugin_get_output_connection_index(int to_id, int from_id, connection_type type) {
	plugin_descriptor to_plugin = plugins[to_id]->descriptor;
	plugin_descriptor from_plugin = plugins[from_id]->descriptor;
	in_edge_iterator out, out_end;
	boost::tie(out, out_end) = in_edges(to_plugin, graph);
	for (in_edge_iterator i = out; i != out_end; ++i) {
		if (graph[*i].conn->type == type) return (int)(i - out);
	}
	return -1;
}

int song::plugin_get_output_connection_plugin(int plugin_id, int index) {
	plugin_descriptor to_plugin = plugins[plugin_id]->descriptor;
	
	zzub::in_edge_iterator out, out_end;
	boost::tie(out, out_end) = in_edges(to_plugin, graph);
	assert(index < (out_end - out));

	plugin_descriptor from_plugin = source(*(out+index), graph);
	return graph[from_plugin].id;
}

connection_type song::plugin_get_output_connection_type(int plugin_id, int index) {
	connection* conn = plugin_get_output_connection(plugin_id, index);
	assert(conn != 0);
	return conn->type;
}

struct feedback_detector : public base_visitor<feedback_detector> {
	struct has_cycle { };
	typedef on_back_edge event_filter;
	vector<connection_descriptor>& back_edges;

	feedback_detector(vector<connection_descriptor>& result)
		:back_edges(result) { }
	template <class Vertex, class Graph>
	inline void operator()(Vertex u, Graph& g) {
		//cerr << u << " is a back edge!" << endl;
		back_edges.push_back(u);
	}
};


void song::make_work_order() {
	work_order.clear();

	// use a temp graph
	plugin_map tg = graph;
	plugin_iterator v, v_end;

	// find back edges
	vector<connection_descriptor> back_edges;
	depth_first_search(tg, visitor(make_dfs_visitor(feedback_detector(back_edges))));

	// remove back edges from temp graph so we can perform a topological sort
	// (TODO: test with multiple loops - are connection_descriptors stable after removing some other edge??)
	for (vector<connection_descriptor>::iterator i = back_edges.begin(); i != back_edges.end(); ++i)
		remove_edge(*i, tg);

	// find roots in the graph, e.g master, and machines w/o outputs
	vector<plugin_descriptor> roots;
	//plugin_iterator v, v_end;
	for (tie(v, v_end) = vertices(tg); v != v_end; ++v) {
		zzub::in_edge_iterator e, e_end;
		boost::tie(e, e_end) = in_edges(*v, tg);
		if ((e_end - e) == 0) roots.push_back(*v);
	}

	// do a topological sort from each root and ensure plugins related to the master are put last in the final work order
	for (vector<plugin_descriptor>::iterator i = roots.begin(); i != roots.end(); ++i) {
		vector<plugin_descriptor> outputs;
		deque<plugin_descriptor> inputs;
		inputs.push_back(*i);

		metaplugin& m = *plugins[tg[*i].id];

		// find plugins that send sound into this root, in topological order.
		// topological_sort_kahn will remove edges from the temp graph.
		topological_sort_kahn(tg, inputs, outputs);

		// check for master in outputs instead of checking no_output-flag to
		// prevent problems e.g when a recorder records the output of the master(?)
		vector<plugin_descriptor>::iterator master_iterator = find(outputs.begin(), outputs.end(), 0);

		if (master_iterator == outputs.end()) {
			work_order.insert(work_order.begin(), outputs.rbegin(), outputs.rend());
		} else {
			work_order.insert(work_order.end(), outputs.rbegin(), outputs.rend());
		}
	}
/*
	cerr << "-------------------------------------------------------" << endl;
	cerr << "Remaining out-edges:" << endl;
	out_edge_iterator out, out_end;
	for (tie(v, v_end) = vertices(tg); v != v_end; ++v) {
		for (tie(out, out_end) = out_edges(*v, tg); out != out_end; ++out) {
			cerr << source(*out, tg) << " -> " << target(*out, tg) << endl;
		}
	}

	cerr << "Remaining in-edges:" << endl;
	in_edge_iterator in, in_end;
	for (tie(v, v_end) = vertices(tg); v != v_end; ++v) {
		for (tie(in, in_end) = in_edges(*v, tg); in != in_end; ++in) {
			cerr << source(*in, tg) << " -> " << target(*in, tg) << endl;
		}
	}

	cerr << "-------------------------------------------------------" << endl;
	for (std::vector<plugin_descriptor>::iterator i = work_order.begin(); i != work_order.end(); ++i) {
		cerr << *i << ", ";
	}
	cerr << endl;*/
}

// ---------------------------------------------------------------------------
//
// Pattern utility
//
// ---------------------------------------------------------------------------

void song::reset_plugin_parameter_group(zzub::pattern::group& group, const std::vector<const zzub::parameter*>& parameters) {
	for (size_t i = 0; i < group.size(); i++)
		reset_plugin_parameter_track(group[i], parameters);
}

void song::reset_plugin_parameter_track(zzub::pattern::track& track, const std::vector<const zzub::parameter*>& parameters) {
	for (size_t j = 0; j < track.size(); j++)
		for (size_t k = 0; k < track[j].size(); k++)
			track[j][k] = parameters[j]->value_none;
}

void song::default_plugin_parameter_track(zzub::pattern::track& track, const std::vector<const zzub::parameter*>& parameters) {
	for (size_t j = 0; j < track.size(); j++)
		for (size_t k = 0; k < track[j].size(); k++)
			if (parameters[j]->flags & zzub::parameter_flag_state)
				track[j][k] = parameters[j]->value_default; else
				track[j][k] = parameters[j]->value_none;
}

int song::get_plugin_parameter_track_row_bytesize(int plugin_id, int g, int t) {
	int size = 0;
	for (int i = 0; i < plugin_get_parameter_count(plugin_id, g, t); i++) {
		const zzub::parameter* param = plugin_get_parameter_info(plugin_id, g, t, i);
		size += param->get_bytesize();
	}
	return size;
}

void song::transfer_plugin_parameter_track_row(int plugin_id, int g, int t, const zzub::pattern& from_pattern, void* target, int row, bool copy_all) {
	const zzub::pattern::group& group = from_pattern.groups[g];

	char* param_ptr = (char*)target;
	int param_ofs = 0;
	for (int i = 0; i < (int)group[t].size(); i++) {
		int v = group[t][i][row];
		const zzub::parameter* param = plugin_get_parameter_info(plugin_id, g, t, i);
		assert(v == param->value_none || (v >= param->value_min && v <= param->value_max) || (param->type == parameter_type_note && v == note_value_off));
		int size = param->get_bytesize();
		if (copy_all || v != param->value_none) {
			char* value_ptr = (char*)&v;
#if defined(ZZUB_BIG_ENDIAN)
			value_ptr += sizeof(int) - size;
#endif
			memcpy(param_ptr + param_ofs, value_ptr, size);
		}
		param_ofs += size;
	}
}

void song::transfer_plugin_parameter_track_row(int plugin_id, int g, int t, const void* source, zzub::pattern& to_pattern, int row, bool copy_all) {
	zzub::pattern::group& group = to_pattern.groups[g];

	char* param_ptr = (char*)source;
	int param_ofs = 0;
	for (int i = 0; i < (int)group[t].size(); i++) {
		int v = 0;
		const zzub::parameter* param = plugin_get_parameter_info(plugin_id, g, t, i);
		int size = param->get_bytesize();
		char* value_ptr = (char*)&v;
#if defined(ZZUB_BIG_ENDIAN)
		value_ptr += sizeof(int) - size;
#endif
		memcpy(value_ptr, param_ptr + param_ofs, size);
		assert(v == param->value_none || (v >= param->value_min && v <= param->value_max) || (param->type == parameter_type_note && v == note_value_off));
		if (copy_all || v != param->value_none) {
			group[t][i][row] = v;
		}
		param_ofs += size;
	}
}
void song::invoke_plugin_parameter_changes(int plugin_id, int g) {
	const zzub::pattern::group& group = plugins[plugin_id]->state_write.groups[g];
	for (int j = 0; j < (int)group.size(); j++) {
		for (int i = 0; i < (int)group[j].size(); i++) {
			const zzub::parameter* param = plugin_get_parameter_info(plugin_id, g, j, i);
			int v = group[j][i][0];

			if (v != param->value_none) {
				zzub_event_data event_data;
				event_data.type = zzub_event_type_parameter_changed;
				event_data.change_parameter.plugin = plugins[plugin_id]->proxy;
				event_data.change_parameter.group = g;
				event_data.change_parameter.track = j;
				event_data.change_parameter.param = i;
				event_data.change_parameter.value = v;
				plugin_invoke_event(plugin_id, event_data, false);
			}
		}
	}
}

void song::invoke_plugin_parameter_changes(int plugin_id) {
	invoke_plugin_parameter_changes(plugin_id, 0);
	invoke_plugin_parameter_changes(plugin_id, 1);
	invoke_plugin_parameter_changes(plugin_id, 2);
}

void song::transfer_plugin_parameter_row(int plugin_id, int g, const zzub::pattern& from_pattern, zzub::pattern& target_pattern, int from_row, int target_row, bool copy_all) {
	const zzub::pattern::group& source_group = from_pattern.groups[g];
	zzub::pattern::group& target_group = target_pattern.groups[g];

	// make sure we dont write outside the buffer
	int transfer_track_count = (int)std::min(target_group.size(), source_group.size());
	for (int j = 0; j < transfer_track_count; j++) {
		int transfer_column_count = (int)std::min(target_group[j].size(), source_group[j].size());
		for (int i = 0; i < transfer_column_count; i++) {
			const zzub::parameter* param = plugin_get_parameter_info(plugin_id, g, j, i);
			int v = source_group[j][i][from_row];
			if (copy_all || v != param->value_none)
				target_group[j][i][target_row] = v;
		}
	}
}

void song::create_pattern(zzub::pattern& result, int plugin_id, int rows) {
	metaplugin& m = *plugins[plugin_id];

	result.rows = rows;
	result.groups.resize(3);

	// add connection tracks
	zzub::out_edge_iterator out, out_end;
	boost::tie(out, out_end) = out_edges(m.descriptor, graph);

	for(; out != out_end; ++out) {
		edge_props& c = graph[*out];
		add_pattern_connection_track(result, c.conn->connection_parameters);
	}

	// add global tracks
	result.groups[1].resize(1);
	result.groups[1][0].resize(m.info->global_parameters.size());
	for (size_t j = 0; j < m.info->global_parameters.size(); j++)
		result.groups[1][0][j].resize(rows);
	reset_plugin_parameter_group(result.groups[1], m.info->global_parameters);

	// add tracks
	result.groups[2].resize(m.tracks);
	for (int i = 0; i < m.tracks; i++) {
		result.groups[2][i].resize(m.info->track_parameters.size());
		for (size_t j = 0; j < m.info->track_parameters.size(); j++)
			result.groups[2][i][j].resize(rows);
	}
	reset_plugin_parameter_group(result.groups[2], m.info->track_parameters);
}

void song::set_pattern_tracks(zzub::pattern& p, const std::vector<const zzub::parameter*>& parameters, int tracks, bool set_defaults) {
	zzub::pattern::group& g = p.groups[2];

	if ((int)g.size() > tracks) {
		g.erase(g.begin() + tracks, g.end());
	} else {
		zzub::pattern::track t;
		t.resize(parameters.size());
		for (size_t j = 0; j < t.size(); j++) {
			t[j].resize(p.rows);
		}
		if (set_defaults) {
			for (size_t j = 0; j < parameters.size(); j++) {
				int v;
				if (parameters[j]->flags & zzub::parameter_flag_state)
					v = parameters[j]->value_default; else
					v = parameters[j]->value_none;
				for (size_t k = 0; k < t[j].size(); k++) {
					t[j][k] = v;
				}
			}
			//default_plugin_parameter_track(t, parameters); 
		} else
			reset_plugin_parameter_track(t, parameters);
		int diff = int(tracks - g.size());
		for (int j = 0; j < diff; j++) {
			g.push_back(t);
		}
	}
}

void song::set_pattern_length(int plugin_id, zzub::pattern& p, int rows) {
	for (int i = 0; i < (int)p.groups.size(); i++) {
		for (int j = 0; j < (int)p.groups[i].size(); j++) {
			for (int k = 0; k < (int)p.groups[i][j].size(); k++) {
				int prevlength = (int)p.groups[i][j][k].size();
				p.groups[i][j][k].resize(rows);
				const zzub::parameter* param = plugin_get_parameter_info(plugin_id, i, j, k);
				for (int l = prevlength; l < rows; l++) {
					p.groups[i][j][k][l] = param->value_none;
				}
			}
		}
	}
	p.rows = rows;
}


void song::add_pattern_connection_track(zzub::pattern& pattern, const std::vector<const zzub::parameter*>& parameters) {
	pattern.groups[0].push_back(zzub::pattern::track());

	zzub::pattern::track& t = pattern.groups[0].back();
	t.resize(parameters.size());

	for (size_t j = 0; j < parameters.size(); j++)
		t[j].resize(pattern.rows);
	reset_plugin_parameter_track(t, parameters);
}

bool song::plugin_invoke_event(int plugin_id, zzub_event_data data, bool immediate) {
	assert(plugin_id >= 0 && plugin_id < (int)plugins.size());
	assert(plugins[plugin_id] != 0);
	if (plugins[plugin_id] == 0) return false;
	std::vector<event_handler*> handlers = plugins[plugin_id]->event_handlers;
	bool handled = false;
	for (size_t i = 0; i < handlers.size(); i++) {
		if (!immediate) {
			event_message em = { plugin_id, handlers[i], data };
			user_event_queue[user_event_queue_write] = em;
			if (user_event_queue_write == user_event_queue.size() - 1)
				user_event_queue_write = 0; else
				user_event_queue_write++;
			assert(user_event_queue_write != user_event_queue_read);
		} else {
			handled = handlers[i]->invoke(data)||handled;
		}
	}
	return handled;
}

void song::process_plugin_events(int plugin_id) {

	metaplugin& m = *plugins[plugin_id];
	assert(m.descriptor != graph_traits<plugin_map>::null_vertex());

	// transfer state_write to live
	zzub::out_edge_iterator out, out_end;
	boost::tie(out, out_end) = out_edges(m.descriptor, graph);
	int index = 0;
	for(; out != out_end; ++out, index++) {
		assert(source(*out, graph) < num_vertices(graph));
		assert(target(*out, graph) < num_vertices(graph));

		edge_props& c = graph[*out];
		transfer_plugin_parameter_track_row(plugin_id, 0, index, m.state_write, c.conn->connection_values, 0, true);
		c.conn->process_events(*this, *out);
	}

	transfer_plugin_parameter_track_row(plugin_id, 1, 0, m.state_write, m.plugin->global_values, 0, true);
	char* track_ptr = (char*)m.plugin->track_values;
	int track_size = get_plugin_parameter_track_row_bytesize(plugin_id, 2, 0);
	for (int i = 0; i < m.tracks; i++) {
		transfer_plugin_parameter_track_row(plugin_id, 2, i, m.state_write, track_ptr, 0, true);
		track_ptr += track_size;
	}

	// send parameter change notifications
	invoke_plugin_parameter_changes(plugin_id);

	// process plugin
	m.plugin->process_events();

	// transfer state_write to state_last
	transfer_plugin_parameter_row(plugin_id, 0, m.state_write, m.state_last, 0, 0, false);
	transfer_plugin_parameter_row(plugin_id, 1, m.state_write, m.state_last, 0, 0, false);
	transfer_plugin_parameter_row(plugin_id, 2, m.state_write, m.state_last, 0, 0, false);

	// reset connection states
	for (int i = 0; i < plugin_get_input_connection_count(plugin_id); i++) {
		connection* conn = plugin_get_input_connection(plugin_id, i);
		reset_plugin_parameter_track(m.state_write.groups[0][i], conn->connection_parameters);
	}

	// reset global and track states
	reset_plugin_parameter_group(m.state_write.groups[1], m.info->global_parameters);
	reset_plugin_parameter_group(m.state_write.groups[2], m.info->track_parameters);

}



// ---------------------------------------------------------------------------
//
// Plugin utility
//
// ---------------------------------------------------------------------------

std::string song::plugin_describe_value(plugin_descriptor plugindesc, int group, int column, int value) {
	return "Describe value here";
}

int song::sequencer_get_event_at(int track, unsigned long timestamp) {
	int event_count = (int)sequencer_tracks[track].events.size();
	int song_index = 0;
	while (event_count > 0 && song_index < event_count
		&& sequencer_tracks[track].events[song_index].first < (int)timestamp) 
	{
		song_index++;
	}

	if (song_index >= event_count || sequencer_tracks[track].events[song_index].first != timestamp) return -1;

	if (sequencer_tracks[track].events[song_index].first == timestamp)
		return sequencer_tracks[track].events[song_index].second;

	return -1;
}

void song::set_state(player_state newstate) {
	state = newstate;
	master_info.tick_position = 0;
	switch (state) {
		case player_state_playing:
			break;
		case player_state_stopped:
			for (int i = 0; i < get_plugin_count(); i++)
				get_plugin(i).plugin->stop();
			break;
	}
}

/***

	mixer

***/

mixer::mixer() {
	solo_plugin = graph_traits<plugin_map>::null_vertex();
	work_position = 0;
	work_tick_fracs = 0.0f;
	is_recording_parameters = false;
	is_syncing_midi_transport = false;
	song_position = 0;
	last_tick_position = -1;

	mix_buffer.resize(2);
	mix_buffer[0].resize(zzub::buffer_size * 4);
	mix_buffer[1].resize(zzub::buffer_size * 4);
	for (int i = 0; i < audiodriver::MAX_CHANNELS; i++) {
		inputBuffer[i] = 0;
	}

}

void mixer::process_sequencer_events(plugin_descriptor plugin) {
	metaplugin& m = get_plugin(plugin);
	int plugin_id = graph[plugin].id;

	for (size_t i = 0; i < sequencer_tracks.size(); i++) {
		if (plugin_id != sequencer_tracks[i].plugin_id) continue;
		int index = sequencer_indices[i];
		if (index == -1) continue;
		if (sequencer_tracks[i].events.size() > 0 && (size_t)index < sequencer_tracks[i].events.size()) {
			sequencer_track::time_value& e = sequencer_tracks[i].events[index];

			switch (e.second) {
				//case -1:	// none
				case 0:		// mute
				case 1:		// break
				case 2:		// thru
					if (e.first == song_position)
						m.sequencer_state = (sequencer_event_type)e.second;
					break;
				default:	// pattern
					if (size_t(e.second - 0x10) < m.patterns.size()) {
						zzub::pattern& p = *m.patterns[e.second - 0x10];
						int row = song_position - e.first;
						if (row >= 0 && row < p.rows && !m.is_muted && !m.is_bypassed) {
							if (row == 0) m.plugin->play_pattern(e.second - 0x10);
							m.sequencer_state = sequencer_event_type_none;
							transfer_plugin_parameter_row(get_plugin_id(plugin), 0, p, m.state_write, row, 0, false);
							transfer_plugin_parameter_row(get_plugin_id(plugin), 1, p, m.state_write, row, 0, false);
							transfer_plugin_parameter_row(get_plugin_id(plugin), 2, p, m.state_write, row, 0, false);
							//cout << "Pattern row " << row << endl;
						}
					}
					break;
			}
		}
	}
}

int mixer::determine_chunk_size(int sample_count, double& tick_fracs, int& next_tick_position) {
	int chunk_size = master_info.samples_per_tick + (int)floor(tick_fracs) - master_info.tick_position;

	int max_size = std::min((int)buffer_size, master_info.samples_per_tick);	// eg mixing rate of 4000hz = 250 samples per tick
	if (chunk_size > max_size || chunk_size < 0) {
		chunk_size = max_size;
		if (chunk_size > sample_count)
			chunk_size = sample_count;
		next_tick_position = master_info.tick_position + chunk_size;
	} else
	if (chunk_size > sample_count) {
		chunk_size = sample_count;
		next_tick_position = master_info.tick_position + chunk_size;
	} else {
		double n;
		tick_fracs = modf(tick_fracs, &n);
		next_tick_position = 0;
	}
	return chunk_size;
}

void mixer::process_keyjazz_noteoff_events() {
	// check for delayed note offs
	// plugin_update_keyjazz may modify keyjazz so we use a copy
	std::vector<keyjazz_note> keycopy = keyjazz;
	for (size_t i = 0; i < keycopy.size(); i++) {
		if (keycopy[i].delay_off == true) {
			//cerr << "playing delayed off" << endl;
			int plugin_id = keycopy[i].plugin_id;
			int note_group = -1, note_track = -1, note_column = -1;
			int velocity_column = -1;
			plugin_update_keyjazz(plugin_id, note_value_off, keycopy[i].note, 0, note_group, note_track, note_column, velocity_column);
			if (note_group != -1) {
				plugin_set_parameter_direct(plugin_id, note_group, note_track, note_column, note_value_off, true);
			}
		}
	}
}

void mixer::process_sequencer_events() {
	process_keyjazz_noteoff_events();

	bool is_playing = state == player_state_playing;

	if (is_playing && !song_loop_enabled && song_position >= song_loop_end) {
		set_state(player_state_stopped);
		is_playing = false;
		zzub_event_data event_data;
		event_data.type = event_type_player_state_changed;
		event_data.player_state_changed.player_state = player_state_stopped;
		plugin_invoke_event(0, event_data, false);
	}

	if (is_playing) {
		if (song_position >= song_loop_end)
			song_position = song_loop_begin;

		for (size_t i = 0; i < sequencer_indices.size(); i++) {
			sequencer_track& seqtrack = sequencer_tracks[i];
			int song_index = sequencer_indices[i];
			while (seqtrack.events.size() > 0 && (size_t)song_index > 0 && seqtrack.events[song_index - 1].first >= song_position) {
				song_index--;
			}

			while (seqtrack.events.size() > 0 && (size_t)song_index < seqtrack.events.size() && seqtrack.events[song_index].first < song_position) {
				song_index++;
			}

			if (seqtrack.events.size() > 0 && (size_t)song_index < seqtrack.events.size() && seqtrack.events[song_index].first == song_position) {
				sequencer_indices[i] = song_index;
			}
		}
	}

	// write parameters from patterns in the sequencer and tick
	for (size_t i = 0; i < work_order.size(); i++) {
		if (is_playing) process_sequencer_events(work_order[i]);
	}

	last_tick_position = song_position;
}


int mixer::generate_audio(int sample_count) {

	// is state is muted, we abort so the user thread can modify song data freely
	if (state == player_state_muted) {
		int mute_buffer_size = sample_count > buffer_size ? buffer_size : sample_count;
		metaplugin& masterplugin = get_plugin(0);
		assert(mute_buffer_size < masterplugin.work_buffer[0].size());
		assert(mute_buffer_size < masterplugin.work_buffer[1].size());
		memset(&masterplugin.work_buffer[0].front(), 0, mute_buffer_size * sizeof(float));
		memset(&masterplugin.work_buffer[1].front(), 0, mute_buffer_size * sizeof(float));
		return mute_buffer_size;
	}

	double start_time = timer.frame();

	// set tick_position to 0 so we trigger a new tick when song position
	// is changed outside the player. eg in a plugin or from the host
	// TODO: when a re-position occurs, we should make sure all plugins 
	// are re-ticked on the correct song position
	if ((state == player_state_playing) && song_position != last_tick_position) {
		master_info.tick_position = 0;
		sequencer_update_play_pattern_positions();
	}

	if (master_info.tick_position == 0) {

		// read params from sequencer and send to state_write
		process_sequencer_events();

		// process event connections and tick each plugin
		for (size_t i = 0; i < work_order.size(); i++) {
			int plugin_id = get_plugin_id(work_order[i]);
			
			assert(plugin_id >=0 && plugin_id < plugins.size() && plugins[plugin_id] != 0);
			metaplugin& workplugin = *plugins[plugin_id];

			// process events (connections may alter state_write)
			if (!workplugin.is_muted && !workplugin.is_bypassed)
				process_plugin_events(plugin_id);
		}

	}

	// at this point we have called process_events() on all plugins and know bpm/tpb/etc

	// determine number of samples to process until next tick or end of current buffer
	int next_tick_position;
	work_chunk_size = determine_chunk_size(sample_count, work_tick_fracs, next_tick_position);
	assert(next_tick_position <= master_info.samples_per_tick);
	assert(work_chunk_size >= 0 && work_chunk_size <= sample_count);

	// process plugins
	for (size_t i = 0; i < work_order.size(); i++) {

		// process audio
		work_plugin(work_order[i], work_chunk_size);

	}

	// process midi
	for (size_t i = 0; i < work_order.size(); i++) {
		metaplugin& workplugin = get_plugin(work_order[i]);
		workplugin.midi_messages.clear();
	}

	if (master_info.tick_position == 0) {
		last_tick_work_position = work_position;
		work_tick_fracs += master_info.samples_per_tick_frac;
		//if (state == player_state_playing) song_position++;
	}

	if (next_tick_position == 0 && state == player_state_playing)
		song_position++;

	// update internal stuff
	work_position += work_chunk_size;
	master_info.tick_position = next_tick_position;

	return work_chunk_size;
}

bool mixer::get_currently_playing_pattern(int plugin_id, int& pattern_index, int& pattern_row) {
	metaplugin& m = *plugins[plugin_id];

	for (size_t i = 0; i < sequencer_tracks.size(); i++) {
		const sequencer_track& seqtrack = sequencer_tracks[i];
		if (plugin_id != seqtrack.plugin_id) continue;
		int index = sequencer_indices[i];
		if (index == -1) continue;
		if (seqtrack.events.size() > 0 && (size_t)index < seqtrack.events.size()) {
			const sequencer_track::time_value& e = seqtrack.events[index];
			switch (e.second) {
				//case -1:	// none
				case 0:		// mute
				case 1:		// break
				case 2:		// thru
					break;
				default:	// pattern
					if (size_t(e.second - 0x10) < m.patterns.size()) {
						zzub::pattern& p = *m.patterns[e.second - 0x10];
						int row = song_position - e.first;
						if (row >= 0 && row < p.rows) {
							pattern_index = e.second - 0x10;
							pattern_row = row;
							return true;
						}
					}
					break;
			}
		}
	}
	return false;
}

bool mixer::get_currently_playing_pattern_row(int plugin_id, int pattern_index, int& pattern_row) {
	metaplugin& m = *plugins[plugin_id];

	for (size_t i = 0; i < sequencer_tracks.size(); i++) {
		const sequencer_track& seqtrack = sequencer_tracks[i];
		if (plugin_id != seqtrack.plugin_id) continue;
		int index = sequencer_indices[i];
		if (index == -1) continue;
		if (seqtrack.events.size() > 0 && (size_t)index < seqtrack.events.size()) {
			const sequencer_track::time_value& e = seqtrack.events[index];

			if (size_t(e.second - 0x10) == pattern_index) {
				zzub::pattern& p = *m.patterns[e.second - 0x10];
				int row = song_position - e.first;
				if (row >= 0 && row < p.rows) {
					pattern_row = row;
					return true;
				}
			}
		}
	}
	return false;
}

void mixer::sequencer_update_play_pattern_positions() {
	sequencer_indices.resize(sequencer_tracks.size());
	std::fill(sequencer_indices.begin(), sequencer_indices.end(), -1);

	for (size_t i = 0; i < sequencer_tracks.size(); i++) {
		for (size_t j = 0; j < sequencer_tracks[i].events.size(); j++) {
			sequencer_track::time_value& ta = sequencer_tracks[i].events[j];
			if (sequencer_indices[i] == -1 || ta.first <= song_position) 
				sequencer_indices[i] = (int)j;
			if (ta.first >= song_position) break;
		}
	}
}

void mixer::work_plugin(plugin_descriptor plugin, int sample_count) {

	double start_time = timer.frame();

	// process connections
	int plugin_id = get_plugin_id(plugin);
	metaplugin& m = *plugins[plugin_id];
	memset(&m.work_buffer[0].front(), 0, sample_count * sizeof(float));
	memset(&m.work_buffer[1].front(), 0, sample_count * sizeof(float));

	bool result = false;
	zzub::out_edge_iterator out, out_end;
	boost::tie(out, out_end) = out_edges(plugin, graph);
	for(; out != out_end; ++out) {
		assert(source(*out, graph) < num_vertices(graph));
		assert(target(*out, graph) < num_vertices(graph));

		edge_props& c = graph[*out];
		result |= c.conn->work(*this, *out, work_chunk_size);
	}

	// process audio:
	int flags;
	if (((m.info->flags & zzub_plugin_flag_has_audio_output) != 0) &&
		((m.info->flags & zzub_plugin_flag_has_audio_input) == 0)) {
		flags = zzub::process_mode_write;
	} else {

		if (result) {
			bool does_input_mixing = (m.info->flags & zzub::plugin_flag_does_input_mixing) != 0;
			bool has_signals = 
				buffer_has_signals(&m.work_buffer[0].front(), sample_count) || 
				buffer_has_signals(&m.work_buffer[1].front(), sample_count);
			flags = (does_input_mixing || has_signals)? zzub::process_mode_read_write:zzub::process_mode_write;
		} else
			flags = zzub::process_mode_write;
	}

	memcpy(&mix_buffer[0].front(), &m.work_buffer[0].front(), sample_count * sizeof(float));
	memcpy(&mix_buffer[1].front(), &m.work_buffer[1].front(), sample_count * sizeof(float));

	float *plin[] = { &mix_buffer[0].front(), &mix_buffer[1].front() };
	float *plout[] = { &m.work_buffer[0].front(), &m.work_buffer[1].front() };

	if (m.is_muted || m.sequencer_state == sequencer_event_type_mute) {
		m.last_work_audio_result = false;
	} else
	if (m.is_bypassed || m.sequencer_state == sequencer_event_type_thru) {
		m.last_work_audio_result = result;
	} else {
		m.last_work_audio_result = m.plugin->process_stereo(plin, plout, sample_count, flags);
	}

	std::copy(m.callbacks->feedback_buffer[0].begin() + sample_count, m.callbacks->feedback_buffer[0].begin() + buffer_size, m.callbacks->feedback_buffer[0].begin());
	std::copy(m.callbacks->feedback_buffer[1].begin() + sample_count, m.callbacks->feedback_buffer[1].begin() + buffer_size, m.callbacks->feedback_buffer[1].begin());
	std::copy(m.work_buffer[0].begin(), m.work_buffer[0].begin() + sample_count, m.callbacks->feedback_buffer[0].begin() + buffer_size - sample_count);
	std::copy(m.work_buffer[1].begin(), m.work_buffer[1].begin() + sample_count, m.callbacks->feedback_buffer[1].begin() + buffer_size - sample_count);

	float samplerate = float(master_info.samples_per_second);
	float falloff = std::pow(10.0f, (-48.0f / (samplerate * 20.0f))); // vu meter falloff (-48dB/s)
	if (m.last_work_audio_result) {
		if (scanPeakStereo(&m.work_buffer[0].front(), &m.work_buffer[1].front(), sample_count, 
			m.last_work_max_left, m.last_work_max_right, falloff)) {
			// the plugin claims it has generated non-silence, but our scan says otherwise
			m.writemode_errors++;
		}
	} else {
		m.last_work_max_left *= std::pow(falloff, sample_count);
		m.last_work_max_right *= std::pow(falloff, sample_count);
	}

	// write recorded parameters to patterns
	if (is_recording_parameters) {
		int pattern_index, pattern_row;
		if (get_currently_playing_pattern(plugin_id, pattern_index, pattern_row)) {
			zzub::pattern& p = *m.patterns[pattern_index];
			transfer_plugin_parameter_row(plugin_id, 0, m.state_automation, p, 0, pattern_row, false);
			transfer_plugin_parameter_row(plugin_id, 1, m.state_automation, p, 0, pattern_row, false);
			transfer_plugin_parameter_row(plugin_id, 2, m.state_automation, p, 0, pattern_row, false);
		}
	}

	// clear recorded parameters - state_automation is currently written to all the time,
	// ignoring is_recording_parameters, so we need to clear it all the time as well
	reset_plugin_parameter_group(m.state_automation.groups[1], m.info->global_parameters);
	reset_plugin_parameter_group(m.state_automation.groups[2], m.info->track_parameters);

	// update statistics
	m.last_work_time = timer.frame() - start_time;
	m.last_work_buffersize = sample_count;
	m.last_work_frame = work_position;

	// these are used to calculating cpu_load-per-plugin-per-buffer in op_player_get_plugins_load_snapshot::operate()
	m.cpu_load_time += m.last_work_time;
	m.cpu_load_buffersize += sample_count;
}

bool mixer::plugin_update_keyjazz(int plugin_id, int note, int prev_note, int velocity, int& note_group, int& note_track, int& note_column, int& velocity_column) {
	assert(plugin_id >= 0 && plugin_id < plugins.size());
	assert(plugins[plugin_id] != 0);

	metaplugin& m = *plugins[plugin_id];

	note_group = -1;

	// ignore note-off when prevNote wasnt already playing, except if prevNote is -1, where we stop all playing notes
	if (note == note_value_off && prev_note != -1 && !is_note_playing(plugin_id, keyjazz, prev_note)) return false;

	if (!get_note_info(m.info, note_group, note_column)) return false;
	if (!get_velocity_info(m.info, note_group, velocity_column))
		velocity_column = -1;

	if (note_group == 2) {
		// play note on track
		if (note == note_value_off) {
			// find which track this note was played in and play a note-stop
			// if note-off is on the same tick as the note it stops, we wait 
			// until next tick before "comitting" it, so that we dont overwrite
			// notes when recording
			// if timestamp is >= lastTickPos, set keyjazz->delay_off to true 
			// and return. a poller checks the keyjazz-vector each tick and
			// records/plays noteoffs then.
			for (size_t i = 0; i < keyjazz.size(); i++) {
				if (keyjazz[i].plugin_id != plugin_id) continue;
				if (keyjazz[i].note == prev_note || prev_note == -1) {
					
					note_group = keyjazz[i].group;
					note_track = keyjazz[i].track;

					if (keyjazz[i].timestamp >= last_tick_work_position) {
						cerr << "note off on the same tick as note was played!" << endl;
						keyjazz[i].delay_off = true;
						return true;
					}

					keyjazz.erase(keyjazz.begin() + i);
					i--;
					if (prev_note != -1) return true;
				}
			}
		} else {
			int lowest_time = std::numeric_limits<int>::max();
			int lowest_track = -1;
			int found_track = -1;
			size_t lowest_index;

			vector<bool> found_tracks(m.tracks);
			for (size_t i = 0; i < found_tracks.size(); i++)
				found_tracks[i] = false;

			for (size_t j = 0; j < keyjazz.size(); j++) {
				if (keyjazz[j].plugin_id != plugin_id) continue;
				size_t track = keyjazz[j].track;
				if (track >= found_tracks.size()) continue;

				found_tracks[track] = true;
				if (keyjazz[j].timestamp < lowest_time) {
					lowest_time = keyjazz[j].timestamp;
					lowest_track = keyjazz[j].track;
					lowest_index = j;
				}
			}

			for (size_t i = 0; i < found_tracks.size(); i++) {
				if (found_tracks[i] == false) {
					found_track = (int)i;
					break;
				}
			}
			if (found_track == -1) {
				found_track = lowest_track;
				keyjazz.erase(keyjazz.begin() + lowest_index);
			}

			note_track = found_track;

			// find an available track or the one with lowest timestamp
			keyjazz_note ki = { plugin_id, work_position, note_group, found_track, note, false };
			keyjazz.push_back(ki);
			return true;
		}
	} else {
		// play global note - no need for track counting
		if (note == note_value_off) {
			for (size_t i = 0; i < keyjazz.size(); i++) {
				if (keyjazz[i].plugin_id != plugin_id) continue;
				if (keyjazz[i].note == prev_note || prev_note == -1) {
					note_track = 0;

					if (keyjazz[i].timestamp >= last_tick_work_position) {
						cerr << "detected a note off on the same tick as note was played!" << endl;
						keyjazz[i].delay_off = true;
						return true;
					}

					keyjazz.clear();
					return true;
				}
			}
		} else {
			keyjazz_note ki = { plugin_id, work_position, 1, 0, note, false };
			keyjazz.clear();
			keyjazz.push_back(ki);

			note_track = 0;
			return true;
		}
	}
	return false;
}

/*pattern mixer::create_play_note_pattern(int plugin_id, int note, int prevNote, int _velocity, std::vector<keyjazz_note>& keyjazz) {

	assert(plugin_id >= 0 && plugin_id < plugins.size());
	assert(plugins[plugin_id] != 0);

	metaplugin& m = *plugins[plugin_id];

	// ignore note-off when prevNote wasnt already playing, except if prevNote is -1, where we stop all playing notes
	if (note == note_value_off && prevNote != -1 && !is_note_playing(plugin_id, keyjazz, prevNote)) return pattern();

	int note_group, note_column, velocity_column;
	if (!get_note_info(m.info, note_group, note_column)) return pattern();
	if (!get_velocity_info(m.info, note_group, velocity_column))
		velocity_column = -1;

	// create a blank 1-row pattern we're going to play
	zzub::pattern p = create_pattern(plugin_id, 1);// = new zzub::pattern();//plugin->loader->plugin_info, plugin->getConnections(), plugin->getTracks(), 1);

	if (note_group == 2) {
		// play note on track
		if (note == note_value_off) {
			// find which track this note was played in and play a note-stop
			// if note-off is on the same tick as the note it stops, we wait 
			// until next tick before "comitting" it, so that we dont overwrite
			// notes when recording
			// if timestamp is >= lastTickPos, set keyjazz->delay_off to true 
			// and return. a poller checks the keyjazz-vector each tick and
			// records/plays noteoffs then.
			for (size_t i = 0; i < keyjazz.size(); i++) {
				if (keyjazz[i].plugin_id != plugin_id) continue;
				if (keyjazz[i].note == prevNote || prevNote == -1) {
					
					if (keyjazz[i].timestamp >= last_tick_work_position) {
						cerr << "note off on the same tick as note was played!" << endl;
						keyjazz[i].delay_off = true;
						break;
					}

					p.groups[keyjazz[i].group][keyjazz[i].track][note_column][0] = note_value_off;
					keyjazz.erase(keyjazz.begin() + i);
					i--;
					if (prevNote != -1) break;
				}
			}
		} else {
			int lowest_time = std::numeric_limits<int>::max();
			int lowest_track = -1;
			int found_track = -1;
			size_t lowest_index;

			vector<bool> found_tracks(m.tracks);
			for (size_t i = 0; i < found_tracks.size(); i++)
				found_tracks[i] = false;

			for (size_t j = 0; j < keyjazz.size(); j++) {
				if (keyjazz[j].plugin_id != plugin_id) continue;
				size_t track = keyjazz[j].track;
				if (track >= found_tracks.size()) continue;

				found_tracks[track] = true;
				if (keyjazz[j].timestamp < lowest_time) {
					lowest_time = keyjazz[j].timestamp;
					lowest_track = keyjazz[j].track;
					lowest_index = j;
				}
			}

			for (size_t i = 0; i < found_tracks.size(); i++) {
				if (found_tracks[i] == false) {
					found_track = (int)i;
					break;
				}
			}
			if (found_track == -1) {
				found_track = lowest_track;
				keyjazz.erase(keyjazz.begin() + lowest_index);
			}

			p.groups[note_group][found_track][note_column][0] = note;
			if(velocity_column != -1 && _velocity)
				p.groups[note_group][found_track][velocity_column][0] = _velocity;

			// find an available track or the one with lowest timestamp
			keyjazz_note ki = { plugin_id, work_position, note_group, found_track, note, false };
			keyjazz.push_back(ki);
		}
	} else {
		// play global note - no need for track counting
		if (note == note_value_off) {
			for (size_t i = 0; i < keyjazz.size(); i++) {
				if (keyjazz[i].plugin_id != plugin_id) continue;
				if (keyjazz[i].note == prevNote || prevNote == -1) {
					if (keyjazz[i].timestamp >= last_tick_work_position) {
						cerr << "detected a note off on the same tick as note was played!" << endl;
						keyjazz[i].delay_off = true;
						break;
					}

					keyjazz.clear();

					p.groups[note_group][0][note_column][0] = note;
					if(velocity_column != -1 && _velocity)
						p.groups[note_group][0][velocity_column][0] = _velocity;
					break;
				}
			}
		} else {
			keyjazz_note ki = { plugin_id, work_position, 1, 0, note, false };
			keyjazz.clear();
			keyjazz.push_back(ki);

			p.groups[note_group][0][note_column][0] = note;
			if(velocity_column != -1 && _velocity)
				p.groups[note_group][0][velocity_column][0] = _velocity;
		}
	}

	return p;
}

*/
void mixer::midi_event(unsigned short status, unsigned char data1, unsigned char data2) {
	// look up mapping(s) and send value to plugin
	int channel = status&0xF;
	int command = (status & 0xf0) >> 4;

	if ((command == 0xb) || (command == 0xe)) {
		if (command == 0xe) {
			// convert pitchbend to CC
			data1 = 128;
		}

		for (size_t i = 0; i < midi_mappings.size(); i++) {
			midimapping& mm = midi_mappings[i];
			if (mm.channel == channel && mm.controller == data1) {
				const parameter* param = plugin_get_parameter_info(mm.plugin_id, mm.group, mm.track, mm.column);
				float minValue = (float)param->value_min;
				float maxValue = (float)param->value_max;
				float delta = (maxValue - minValue) / 127.0f;

				// TODO: we need a set_parameter_direct
				plugin_set_parameter_direct(mm.plugin_id, mm.group, mm.track, mm.column, (int)ceil(minValue + data2 * delta), true);
				process_plugin_events(mm.plugin_id);
			}
		}
	}
	
	// also send note events to plugins directly
	if ((command == 8) || (command == 9)) {
		int velocity = (int)data2;
		if (command == 8)
			velocity = 0;
		for (int i = 0; i < get_plugin_count(); i++) {
			metaplugin& m = get_plugin(i);
			int plugin_id = get_plugin_id(i);
			m.plugin->midi_note(channel, (int)data1, velocity);

			if (m.midi_input_channel == channel || m.midi_input_channel == 16 || (m.midi_input_channel == 17 && plugin_id == midi_plugin)) {
				// play a recordable note/off, w/optional velocity, delay and cut
				int note, prevNote;
				if (command == 9 && velocity != 0) {
					note = midi_to_buzz_note(data1);
					prevNote = -1;
				} else {
					note = zzub::note_value_off;
					prevNote = midi_to_buzz_note(data1);
				}

				// find note_group, track, column and velocity_group, track and column based on keyjazz-struct
				int note_group = -1, note_track = -1, note_column = -1;
				int velocity_column = -1;
				plugin_update_keyjazz(plugin_id, note, prevNote, velocity, note_group, note_track, note_column, velocity_column);

				if (note_group != -1) {
					plugin_set_parameter_direct(plugin_id, note_group, note_track, note_column, note, true);
					if (velocity_column != -1 && velocity != 0) {
						const parameter* param = plugin_get_parameter_info(plugin_id, note_group, note_track, velocity_column);
						velocity = (velocity * param->value_max) / 127; // Set appropriate velocity
						plugin_set_parameter_direct(plugin_id, note_group, note_track, velocity_column, velocity, true);
					}

					process_plugin_events(plugin_id);
				}
			}
		}
	}
	else if (command == 0xb) {
		for (int i = 0; i < get_plugin_count(); i++) {
			zzub::metaplugin& m = get_plugin(i);
			m.plugin->midi_control_change((int)data1, channel, (int)data2);
		}
	}

	// plus all midi messages should be sent as master-events, so ui's can pick these up

	if (command == 0xe) {
		// convert pitchbend to CC
		command = 0xb;
		status = channel | (command << 4);
		data1 = 128;
	}

	zzub_event_data data = { event_type_midi_control };
	data.midi_message.status = (unsigned char)status;
	data.midi_message.data1 = data1;
	data.midi_message.data2 = data2;

	plugin_invoke_event(0, data);
}

};
