#include "common.h"
#include <functional>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <iostream>
#include "bmxreader.h"
#include "bmxwriter.h"
#include "timer.h"
#include "dummy.h"
#include "archive.h"
#include "tools.h"

using std::cerr;
using std::endl;

namespace zzub {

// ---------------------------------------------------------------------------
//
// op_state_change
//
// ---------------------------------------------------------------------------

op_state_change::op_state_change(zzub::player_state _state) {
	state = _state;
}

bool op_state_change::prepare(zzub::song& song) {
	event_data.type = event_type_player_state_changed;
	event_data.player_state_changed.player_state = state;
	return true;
}

bool op_state_change::operate(zzub::song& song) {
	song.state = state;
	song.master_info.tick_position = 0;
	switch (state) {
		case player_state_playing:
			break;
		case player_state_stopped:
			for (int i = 0; i < song.get_plugin_count(); i++)
				song.get_plugin(i).plugin->stop();
			break;
	}
	return true;
}

void op_state_change::finish(zzub::song& song, bool send_events) {
	if (send_events) song.plugin_invoke_event(0, event_data, true);
}

// ---------------------------------------------------------------------------
//
// op_plugin_create
//
// ---------------------------------------------------------------------------

op_plugin_create::op_plugin_create(zzub::player* _player, int _id, std::string _name, std::vector<char>& _bytes, const zzub::info* _loader) {
	assert(_id != -1);
	assert(_name != "");
	assert(_loader);

	player = _player;
	id = _id;
	bytes = _bytes;
	name = _name;
	loader = _loader;

	copy_flags.copy_graph = true;
	copy_flags.copy_work_order = true;
	copy_flags.copy_plugins = true;
}

bool op_plugin_create::prepare(zzub::song& song) {

	zzub::plugin* instance = loader->create_plugin();
	if (instance == 0) return false;

	assert(id != -1);

	if (song.plugins.size() <= id) song.plugins.resize(id + 1);

	assert(song.plugins[id] == 0);
	song.plugins[id] = new metaplugin();

	plugin_descriptor descriptor = add_vertex(song.graph);
	song.graph[descriptor].id = id;

	metaplugin& plugin = song.get_plugin(descriptor);

	plugin.name = name;
	plugin.plugin = instance;
	plugin.descriptor = descriptor;
	plugin.info = loader;
	plugin.tracks = 0;
	plugin.midi_input_channel = 17;	// 17 = play if selected
	plugin.is_bypassed = false;
	plugin.is_muted = false;
	plugin.sequencer_state = sequencer_event_type_none;
	plugin.x = 0;
	plugin.y = 0;
	plugin.last_work_audio_result = false;
	plugin.last_work_midi_result = false;
	plugin.last_work_buffersize = 0;
	plugin.last_work_max_left = 0;
	plugin.last_work_max_right = 0;
	plugin.last_work_time = 0.0f;
	plugin.cpu_load = 0.0f;
	plugin.cpu_load_buffersize = 0;
	plugin.cpu_load_time = 0.0f;

	plugin.initialized = false;
	plugin.work_buffer.resize(2);
	plugin.work_buffer[0].resize(buffer_size * 4);
	plugin.work_buffer[1].resize(buffer_size * 4);
	plugin.proxy = new metaplugin_proxy(player, id);
	plugin.callbacks = new host(player, plugin.proxy);
	plugin.callbacks->plugin_player = &song;
	plugin.plugin->_host = plugin.callbacks;
	plugin.plugin->_master_info = &player->front.master_info;

	// setting default attributes before init() makes some stereo wrapped machines work - instead of crashing in first attributesChanged 
	if (instance->attributes) 
		for (size_t i = 0; i < loader->attributes.size(); i++) 
			instance->attributes[i] = loader->attributes[i]->value_default;

	// create state patterns before init(), in case plugins try to call 
	// host::control_change in their init() (e.g Farbrasch V2)
	plugin.tracks = loader->min_tracks;
	// create state patterns with no-values
	plugin.state_write = song.create_pattern(id, 1);
	plugin.state_last = song.create_pattern(id, 1);
	plugin.state_automation = song.create_pattern(id, 1);

	// NOTE: some plugins' init() may call methods on the host to retreive info about other plugins.
	// we handle this by setting callbacks->plugin_player to the backbuffer song until the plugin
	// is swapped into the running graph
	if (bytes.size() > 0) {
		mem_archive arc;
		arc.get_outstream("")->write(&bytes.front(), (int)bytes.size());
		instance->init(&arc);
	} else {
		instance->init(0);
	}

	const char* plugin_stream_source = instance->get_stream_source();
	plugin.stream_source = plugin_stream_source ? plugin_stream_source : "";

	// add states for controller columns
	if (plugin.info->flags & zzub::plugin_flag_has_event_output) {
		plugin.state_write.groups.push_back(pattern::group());
		plugin.state_last.groups.push_back(pattern::group());
		plugin.state_automation.groups.push_back(pattern::group());

		for (size_t i = 0; i < plugin.info->controller_parameters.size(); i++) {
			plugin.state_write.groups.back().push_back(pattern::track());
			plugin.state_write.groups.back().back().push_back(pattern::column());
			plugin.state_write.groups.back().back().back().push_back(plugin.info->controller_parameters[i]->value_none);

			plugin.state_last.groups.back().push_back(pattern::track());
			plugin.state_last.groups.back().back().push_back(pattern::column());
			plugin.state_last.groups.back().back().back().push_back(plugin.info->controller_parameters[i]->value_none);

			plugin.state_automation.groups.back().push_back(pattern::track());
			plugin.state_automation.groups.back().back().push_back(pattern::column());
			plugin.state_automation.groups.back().back().back().push_back(plugin.info->controller_parameters[i]->value_none);
		}
	}

	// fill state pattern with default values and copy to live
	song.default_plugin_parameter_track(plugin.state_write.groups[1][0], loader->global_parameters);
	//song.transfer_plugin_parameter_track_row(id, 1, 0, plugin.state_write, (char*)plugin.plugin->global_values, 0, true);

	//char* track_ptr = (char*)plugin.plugin->track_values;
	//int track_size = song.get_plugin_parameter_track_row_bytesize(id, 2, 0);
	for (int j = 0; j < plugin.tracks; j++) {
		song.default_plugin_parameter_track(plugin.state_write.groups[2][j], loader->track_parameters);
		//song.transfer_plugin_parameter_track_row(id, 2, j, plugin.state_write, track_ptr, 0, true);
		//track_ptr += track_size;
	}

	instance->set_track_count(plugin.tracks);
	instance->attributes_changed();
	song.process_plugin_events(id);

	song.make_work_order();

	event_data.type = event_type_new_plugin;
	event_data.new_plugin.plugin = plugin.proxy;

	return true;
}

bool op_plugin_create::operate(zzub::song& song) {

	metaplugin& plugin = *song.plugins[id];
	plugin.callbacks->plugin_player = &song;
	plugin.initialized = true;
	return true;
}

void op_plugin_create::finish(zzub::song& song, bool send_events) {
	assert(id >= 0 && id <= song.plugins.size());

	if (song.plugins[id] == 0) return ;	// plugin was deleted before it was inserted in the graph

	if (send_events) song.plugin_invoke_event(0, event_data, true);
}

// ---------------------------------------------------------------------------
//
// op_plugin_delete
//
// ---------------------------------------------------------------------------

op_plugin_delete::op_plugin_delete(zzub::player* _player, int _id) {
	player = _player;
	id = _id;

	copy_flags.copy_graph = true;
	copy_flags.copy_work_order = true;
	copy_flags.copy_plugins = true;
	copy_flags.copy_plugins_deep = true;
	copy_flags.copy_sequencer_track_order = true;
}

bool op_plugin_delete::prepare(zzub::song& song) {

	metaplugin& mpl = *song.plugins[id];

	plugin_descriptor plugin = mpl.descriptor;
	assert(plugin != graph_traits<plugin_map>::null_vertex());


	// send delete notification now - before any changes occur in any back/front buffers
	event_data.type = event_type_pre_delete_plugin;
	event_data.delete_plugin.plugin = mpl.proxy;
	song.plugin_invoke_event(0, event_data, true);

	clear_vertex(plugin, song.graph);
	remove_vertex(plugin, song.graph);

	mpl.descriptor = graph_traits<plugin_map>::null_vertex();
	song.make_work_order();

	// adjust all descriptors in plugins and song_events

	for (size_t i = 0; i < song.sequencer_tracks.size(); i++) {
		if (song.sequencer_tracks[i] > plugin)
			song.sequencer_tracks[i]--;
	}

	for (size_t i = 0; i < song.plugins.size(); i++) {
		if (song.plugins[i] != 0 && song.plugins[i]->descriptor != graph_traits<plugin_map>::null_vertex() && song.plugins[i]->descriptor > plugin)
			song.plugins[i]->descriptor--;
	}

	event_data.type = event_type_delete_plugin;
	event_data.delete_plugin.plugin = mpl.proxy;

	return true;
}

bool op_plugin_delete::operate(zzub::song& song) {

	plugin = song.plugins[id];
	song.plugins[id] = 0;

	int read_pos = song.user_event_queue_read;
	while (read_pos != song.user_event_queue_write) {
		event_message& ev = song.user_event_queue[read_pos];
		if (ev.plugin_id == id) ev.event = 0;
		if (read_pos == song.user_event_queue.size() - 1)
			read_pos = 0; else
			read_pos++;
	}
	return true;
}

void op_plugin_delete::finish(zzub::song& song, bool send_events) {
	// the plugin is now assumed to be completely cleaned out of the graph and the swapping is done

	assert(plugin != 0);

	if (send_events) song.plugin_invoke_event(0, event_data, true);

	plugin->plugin->destroy();
	int ptn = plugin->patterns.size();
	delete plugin->callbacks;
	delete plugin->proxy;
	delete plugin;
	plugin = 0;

}

// ---------------------------------------------------------------------------
//
// op_plugin_set_position
//
// ---------------------------------------------------------------------------

op_plugin_replace::op_plugin_replace(int _id, const metaplugin& _plugin) {
	id = _id;
	plugin = _plugin;
	copy_flags.copy_plugins = true;

	operation_copy_plugin_flags pluginflags;
	pluginflags.plugin_id = id;
	pluginflags.copy_plugin = true;
	copy_flags.plugin_flags.push_back(pluginflags);
}

bool op_plugin_replace::prepare(zzub::song& song) {
	event_data.type = event_type_plugin_changed;
	event_data.plugin_changed.plugin = song.plugins[id]->proxy;
	return true;
}

bool op_plugin_replace::operate(zzub::song& song) {

	if (song.plugins[id] == 0) return true;

	song.plugins[id]->name = plugin.name;
	song.plugins[id]->x = plugin.x;
	song.plugins[id]->y = plugin.y;
	song.plugins[id]->is_muted = plugin.is_muted;
	song.plugins[id]->is_bypassed = plugin.is_bypassed;
	song.plugins[id]->midi_input_channel = plugin.midi_input_channel;
	return true;
}

void op_plugin_replace::finish(zzub::song& song, bool send_events) {
	if (send_events) song.plugin_invoke_event(0, event_data, true);
}
// ---------------------------------------------------------------------------
//
// op_plugin_connect
//
// ---------------------------------------------------------------------------

op_plugin_connect::op_plugin_connect(int _from_id, int _to_id, zzub::connection_type _type) {
	from_id = _from_id;
	to_id = _to_id;
	type = _type;

	copy_flags.copy_graph = true;
	copy_flags.copy_work_order = true;
	copy_flags.copy_plugins = true;

	operation_copy_plugin_flags pluginflags;
	pluginflags.plugin_id = to_id;
	pluginflags.copy_plugin = true;
	pluginflags.copy_patterns = true;
	copy_flags.plugin_flags.push_back(pluginflags);
}

struct cyclic_connection : public base_visitor<cyclic_connection> {
	struct has_cycle { };
	typedef on_back_edge event_filter;

	template <class Vertex, class Graph>
	inline void operator()(Vertex u, Graph& g) {
		throw has_cycle();
	}
};

bool op_plugin_connect::prepare(zzub::song& song) {

	plugin_descriptor to_plugin = song.plugins[to_id]->descriptor;
	plugin_descriptor from_plugin = song.plugins[from_id]->descriptor;
	assert(to_plugin != graph_traits<plugin_map>::null_vertex());
	assert(from_plugin != graph_traits<plugin_map>::null_vertex());

	// check for duplicate connection
	if (song.plugin_get_input_connection_index(to_id, from_id, type) != -1) {
		cerr << "duplicate connection" << endl;
		return false;
	}


	// check if the plugins support requested flags
	int to_flags = song.plugins[to_id]->info->flags;
	int from_flags = song.plugins[from_id]->info->flags;
	bool to_has_audio = to_flags & plugin_flag_has_audio_input;
	bool from_has_audio = from_flags & plugin_flag_has_audio_output;
	bool to_has_midi = to_flags & plugin_flag_has_midi_input;
	bool from_has_midi = from_flags & plugin_flag_has_midi_output;
	bool to_has_event = to_flags & plugin_flag_has_event_input;
	bool from_has_event = from_flags & plugin_flag_has_event_output;

	if (type == connection_type_audio && !(to_has_audio && from_has_audio)) {
		cerr << "plugins dont support audio connection, " << song.plugins[from_id]->name << " -> " << song.plugins[to_id]->name << endl;
		return false;
	} else
	if (type == connection_type_midi && !(to_has_midi && from_has_midi)) {
		cerr << "plugins dont support midi connection" << song.plugins[from_id]->name << " -> " << song.plugins[to_id]->name << endl;
		return false;
	} else
	if (type == connection_type_event && !(from_has_event)) {
		cerr << "plugins dont support event connection" << song.plugins[from_id]->name << " -> " << song.plugins[to_id]->name << endl;
		return false;
	}

	// check for cyclic connection = run depth_first_search and break if there is a back_edge. we must insert the edge first
	std::pair<connection_descriptor, bool> edge_instance_p = add_edge(to_plugin, from_plugin, song.graph);
    
	try {
		depth_first_search(song.graph, visitor(make_dfs_visitor(cyclic_connection())));
	} catch (cyclic_connection::has_cycle) {
		remove_edge(edge_instance_p.first, song.graph);
		cerr << "detected cycle!" << endl;
		return false;
	}

	// validating done, remove the edge again before invoking the pre-event so the graph isnt bogus
	remove_edge(edge_instance_p.first, song.graph);

	metaplugin& to_mpl = *song.plugins[to_id];
	metaplugin& from_mpl = *song.plugins[from_id];

	// invoke pre-event so hacked plugins can lock the player
	event_data.type = event_type_pre_connect;
	event_data.connect_plugin.to_plugin = to_mpl.proxy;
	event_data.connect_plugin.from_plugin = from_mpl.proxy;
	event_data.connect_plugin.type = type;
	song.plugin_invoke_event(0, event_data, true);

	// re-add the edge
	edge_instance_p = add_edge(to_plugin, from_plugin, song.graph);
	edge_props& c = song.graph[edge_instance_p.first];

	switch (type) {
		case connection_type_audio:
			c.conn = new audio_connection();
			break;
		case connection_type_midi:
			c.conn = new midi_connection();
			((midi_connection*)c.conn)->device_name = midi_device;
			break;
		case connection_type_event:
			c.conn = new event_connection();
			((event_connection*)c.conn)->bindings = bindings;
			break;
	}

	for (size_t i = 0; i < to_mpl.patterns.size(); i++) {
		song.add_pattern_connection_track(*to_mpl.patterns[i], c.conn->connection_parameters);
	}

	song.add_pattern_connection_track(to_mpl.state_write, c.conn->connection_parameters);
	song.default_plugin_parameter_track(to_mpl.state_write.groups[0].back(), c.conn->connection_parameters);
	for (size_t i = 0; i < values.size() && i < c.conn->connection_parameters.size(); i++) {
		to_mpl.state_write.groups[0].back()[i][0] = values[i];
	}

	song.add_pattern_connection_track(to_mpl.state_last, c.conn->connection_parameters);

	song.add_pattern_connection_track(to_mpl.state_automation, c.conn->connection_parameters);

	song.make_work_order();

	from_name = from_mpl.name;

	event_data.type = event_type_connect;
	event_data.connect_plugin.to_plugin = to_mpl.proxy;
	event_data.connect_plugin.from_plugin = from_mpl.proxy;

	return true;
}

bool op_plugin_connect::operate(zzub::song& song) {
	song.plugins[to_id]->plugin->add_input(from_name.c_str(), type);
	return true;
}

void op_plugin_connect::finish(zzub::song& song, bool send_events) {
	if (send_events) song.plugin_invoke_event(0, event_data, true);

	event_data.type = event_type_post_connect;
	song.plugin_invoke_event(0, event_data, true);
}

// ---------------------------------------------------------------------------
//
// op_plugin_disconnect
//
// ---------------------------------------------------------------------------

op_plugin_disconnect::op_plugin_disconnect(int _from_id, int _to_id, zzub::connection_type _type) {
	from_id = _from_id;
	to_id = _to_id;
	type = _type;

	copy_flags.copy_graph = true;
	copy_flags.copy_work_order = true;
	copy_flags.copy_plugins = true;

	operation_copy_plugin_flags pluginflags;
	pluginflags.plugin_id = to_id;
	pluginflags.copy_plugin = true;
	pluginflags.copy_patterns = true;
	copy_flags.plugin_flags.push_back(pluginflags);

	conn = 0;
}

bool op_plugin_disconnect::prepare(zzub::song& song) {

	metaplugin& to_mpl = *song.plugins[to_id];
	metaplugin& from_mpl = *song.plugins[from_id];

	plugin_descriptor to_plugin = to_mpl.descriptor;
	plugin_descriptor from_plugin = from_mpl.descriptor;
	assert(to_plugin != graph_traits<plugin_map>::null_vertex());
	assert(from_plugin != graph_traits<plugin_map>::null_vertex());

	// invoke pre-event so hacked plugins can lock the player
	event_data.type = event_type_pre_disconnect;
	event_data.disconnect_plugin.to_plugin = to_mpl.proxy;
	event_data.disconnect_plugin.from_plugin = from_mpl.proxy;
	event_data.disconnect_plugin.type = type;
	song.plugin_invoke_event(0, event_data, true);

	int track = song.plugin_get_input_connection_index(to_id, from_id, type);
	assert(track >= 0);

	from_name = song.get_plugin(from_plugin).name;

	// remove connection tracks in patterns

	for (size_t i = 0; i < song.get_plugin(to_plugin).patterns.size(); i++) {
		zzub::pattern& p = *song.get_plugin(to_plugin).patterns[i];
		zzub::pattern::group& g = p.groups[0];
		g.erase(g.begin() + track);
	}

	// remove connection tracks in state
	zzub::pattern::group& writeg = song.get_plugin(to_plugin).state_write.groups[0];
	assert(track < writeg.size());
	writeg.erase(writeg.begin() + track);

	zzub::pattern::group& lastg = song.get_plugin(to_plugin).state_last.groups[0];
	assert(track < lastg.size());
	lastg.erase(lastg.begin() + track);

	zzub::pattern::group& autog = song.get_plugin(to_plugin).state_automation.groups[0];
	assert(track < autog.size());
	autog.erase(autog.begin() + track);

	out_edge_iterator out, out_end;
	boost::tie(out, out_end) = out_edges(to_plugin, song.graph);
	connection_descriptor conndesc = *(out + track);
	conn = song.graph[conndesc].conn;
	remove_edge(conndesc, song.graph);

	event_data.type = event_type_disconnect;
	event_data.disconnect_plugin.to_plugin = to_mpl.proxy;
	event_data.disconnect_plugin.from_plugin = from_mpl.proxy;

	return true;
}

bool op_plugin_disconnect::operate(zzub::song& song) {
	metaplugin& m = *song.plugins[to_id];
	m.plugin->delete_input(from_name.c_str(), type);
	return true;
}

void op_plugin_disconnect::finish(zzub::song& song, bool send_events) {
	if (send_events) song.plugin_invoke_event(0, event_data, true);
	assert(conn != 0);
	delete conn;
	conn = 0;
}

// ---------------------------------------------------------------------------
//
// op_plugin_set_midi_connection_device
//
// ---------------------------------------------------------------------------

op_plugin_set_midi_connection_device::op_plugin_set_midi_connection_device(int _to_id, int _from_id, std::string _name) {
	to_id = _to_id;
	from_id = _from_id;
	device = _name;

	copy_flags.copy_graph = true;
	copy_flags.copy_plugins = true;
}

bool op_plugin_set_midi_connection_device::prepare(zzub::song& song) {
	return true;
}

bool op_plugin_set_midi_connection_device::operate(zzub::song& song) {
	int conn_index = song.plugin_get_input_connection_index(to_id, from_id, connection_type_midi);
	// since we are in operate(), this operation could be a part of a compound operation
	// where the connection has been removed and is no longer valid. ultimately, the connection
	// operations should work on swapped-out connection copies in the future:
	if (conn_index == -1) return true;
	assert(conn_index != -1);

	midi_connection* conn = (midi_connection*)song.plugin_get_input_connection(to_id, conn_index);
	conn->device_name = device;
	return true;
}

void op_plugin_set_midi_connection_device::finish(zzub::song& song, bool send_events) {
}


// ---------------------------------------------------------------------------
//
// op_plugin_add_event_connection_binding
//
// ---------------------------------------------------------------------------

op_plugin_add_event_connection_binding::op_plugin_add_event_connection_binding(int _to_id, int _from_id, event_connection_binding _binding) {
	to_id = _to_id;
	from_id = _from_id;
	binding = _binding;
	copy_flags.copy_graph = true;
	copy_flags.copy_plugins = true;
}

bool op_plugin_add_event_connection_binding::prepare(zzub::song& song) {
	return true;
}

bool op_plugin_add_event_connection_binding::operate(zzub::song& song) {
	int conn_index = song.plugin_get_input_connection_index(to_id, from_id, connection_type_event);
	assert(conn_index != -1);
	event_connection* conn = (event_connection*)song.plugin_get_input_connection(to_id, conn_index);
	conn->bindings.push_back(binding);
	return true;
}

void op_plugin_add_event_connection_binding::finish(zzub::song& song, bool send_events) {
}


// ---------------------------------------------------------------------------
//
// op_plugin_remove_event_connection_binding
//
// ---------------------------------------------------------------------------

op_plugin_remove_event_connection_binding::op_plugin_remove_event_connection_binding(int _to_id, int _from_id, int _index) {
	to_id = _to_id;
	from_id = _from_id;
	index = _index;
	copy_flags.copy_graph = true;
	copy_flags.copy_plugins = true;
}

bool op_plugin_remove_event_connection_binding::prepare(zzub::song& song) {
	return true;
}

bool op_plugin_remove_event_connection_binding::operate(zzub::song& song) {
	
	int conn_index = song.plugin_get_input_connection_index(to_id, from_id, connection_type_event);
	if (conn_index == -1) return true;	// plugin was deleted
	assert(conn_index != -1);
	event_connection* conn = (event_connection*)song.plugin_get_input_connection(to_id, conn_index);
	if (index == -1) index = conn->bindings.size() -1;
	conn->bindings.erase(conn->bindings.begin() + index);
	return true;
}

void op_plugin_remove_event_connection_binding::finish(zzub::song& song, bool send_events) {
}

// ---------------------------------------------------------------------------
//
// op_plugin_sequencer_set_tracks
//
// ---------------------------------------------------------------------------

op_plugin_set_track_count::op_plugin_set_track_count(int _id, int _tracks) {
	id = _id;
	tracks = _tracks;

	copy_flags.copy_graph = true;
	copy_flags.copy_work_order = true;
	copy_flags.copy_plugins = true;

	operation_copy_plugin_flags pluginflags;
	pluginflags.plugin_id = id;
	pluginflags.copy_plugin = true;
	pluginflags.copy_patterns = true;
	copy_flags.plugin_flags.push_back(pluginflags);
}

bool op_plugin_set_track_count::prepare(zzub::song& song) {

	assert(song.plugins[id]);

	metaplugin& m = *song.plugins[id];

	assert(tracks >= m.info->min_tracks && tracks <= m.info->max_tracks);

	// invoke pre-event so hacked plugins can lock the player
	event_data.type = event_type_pre_set_tracks;
	event_data.set_tracks.plugin = m.proxy;
	song.plugin_invoke_event(0, event_data, true);

	for (size_t i = 0; i < m.patterns.size(); i++) {
		song.set_pattern_tracks(*m.patterns[i], m.info->track_parameters, tracks, false);
	}

	song.set_pattern_tracks(m.state_write, m.info->track_parameters, tracks, true);
	song.set_pattern_tracks(m.state_last, m.info->track_parameters, tracks, false);
	song.set_pattern_tracks(m.state_automation, m.info->track_parameters, tracks, false);

	m.tracks = tracks;

	event_data.type = event_type_set_tracks;
	event_data.set_tracks.plugin = m.proxy;

	return true;
}

bool op_plugin_set_track_count::operate(zzub::song& song) {
	assert(id >= 0 && id <= song.plugins.size());
	assert(song.plugins[id] != 0);

	metaplugin& m = *song.plugins[id];
	
	// check if plugin was already deleted
	if (m.descriptor == graph_traits<plugin_map>::null_vertex()) return false;

	char* track_ptr = (char*)m.plugin->track_values;
	int track_size = song.get_plugin_parameter_track_row_bytesize(id, 2, 0);
	for (int i = 0; i < m.tracks; i++) {
		song.transfer_plugin_parameter_track_row(id, 2, i, m.state_write, track_ptr, 0, true);
		track_ptr += track_size;
	}
	m.plugin->set_track_count(tracks);
	return true;
}

void op_plugin_set_track_count::finish(zzub::song& song, bool send_events) {
	if (send_events) song.plugin_invoke_event(0, event_data, true);

	event_data.type = event_type_post_set_tracks;
	song.plugin_invoke_event(0, event_data, true);
}

// ---------------------------------------------------------------------------
//
// op_plugin_set_parameters_and_tick
//
// ---------------------------------------------------------------------------

op_plugin_set_parameters_and_tick::op_plugin_set_parameters_and_tick(int _id, zzub::pattern& _pattern, int _row, bool _no_process) {
	id = _id;
	pattern = _pattern;
	row = _row;
	no_process = _no_process;
	record = false;
}

bool op_plugin_set_parameters_and_tick::prepare(zzub::song& song) {
	event_data.type = -1;
	return true;
}

bool op_plugin_set_parameters_and_tick::operate(zzub::song& song) {
	// TODO: we could move all this into prepare and add a copy-plugin-flag instead
	// - the target plugin could have changed before this point, such having as added/removed a connection
	// - should make the boundary-check inside transfer_plugin_parameter_row redundant
	assert(song.plugins[id] != 0);

	metaplugin& m = *song.plugins[id];

	// check if the plugin was deleted
	if (m.descriptor == graph_traits<plugin_map>::null_vertex()) return true;

	m.sequencer_state = sequencer_event_type_none;

	song.transfer_plugin_parameter_row(id, 0, pattern, m.state_write, row, 0, false);
	song.transfer_plugin_parameter_row(id, 1, pattern, m.state_write, row, 0, false);
	song.transfer_plugin_parameter_row(id, 2, pattern, m.state_write, row, 0, false);

	if (record) {
		song.transfer_plugin_parameter_row(id, 0, pattern, m.state_automation, row, 0, false);
		song.transfer_plugin_parameter_row(id, 1, pattern, m.state_automation, row, 0, false);
		song.transfer_plugin_parameter_row(id, 2, pattern, m.state_automation, row, 0, false);
	}

	if (!no_process) song.process_plugin_events(id);
	return true;
}

op_plugin_play_note::op_plugin_play_note(int _id, int _note, int _prev_note, int _velocity) {
	id = _id;
	note = _note;
	prev_note = _prev_note;
	velocity = _velocity;
}

bool op_plugin_play_note::prepare(zzub::song& song) {
	return true;
}

bool op_plugin_play_note::operate(zzub::song& song) {
	assert(song.plugins[id] != 0);

	metaplugin& m = *song.plugins[id];

	// check if the plugin was deleted
	if (m.descriptor == graph_traits<plugin_map>::null_vertex()) return true;

	// find note_group, track, column and velocity_group, track and column based on keyjazz-struct
	int note_group = -1, note_track = -1, note_column = -1;
	int velocity_column = -1;
	song.plugin_update_keyjazz(id, note, prev_note, velocity, note_group, note_track, note_column, velocity_column);

	if (note_group != -1) {
		song.plugin_set_parameter_direct(id, note_group, note_track, note_column, note, true);
		if (velocity_column != -1 && velocity != 0)
			song.plugin_set_parameter_direct(id, note_group, note_track, velocity_column, velocity, true);

		song.process_plugin_events(id);
	}

	return true;
}

// ---------------------------------------------------------------------------
//
// op_plugin_set_event_handlers
//
// ---------------------------------------------------------------------------

op_plugin_set_event_handlers::op_plugin_set_event_handlers(std::string _name, std::vector<event_handler*>& _handlers) {
	name = _name;
	handlers = _handlers;
}

bool op_plugin_set_event_handlers::prepare(zzub::song& song) {
	event_data.type = -1;
	return true;
}

bool op_plugin_set_event_handlers::operate(zzub::song& song) {
	plugin_descriptor plugin = song.get_plugin_descriptor(name);
	assert(plugin != graph_traits<plugin_map>::null_vertex());

	song.get_plugin(plugin).event_handlers.swap(handlers);
	return true;
}



// ---------------------------------------------------------------------------
//
// op_plugin_set_event_handlers
//
// ---------------------------------------------------------------------------

op_plugin_set_stream_source::op_plugin_set_stream_source(int _id, std::string _data_url) {
	id = _id;
	data_url = _data_url;

	copy_flags.copy_plugins = true;

	operation_copy_plugin_flags pluginflags;
	pluginflags.plugin_id = id;
	pluginflags.copy_plugin = true;
	copy_flags.plugin_flags.push_back(pluginflags);

}

bool op_plugin_set_stream_source::prepare(zzub::song& song) {
	song.plugins[id]->stream_source = data_url;
	return true;
}

bool op_plugin_set_stream_source::operate(zzub::song& song) {
	assert(id >= 0);
	assert(song.plugins[id]);
	
	metaplugin& m = *song.plugins[id];
	m.plugin->set_stream_source(data_url.c_str());

	return true;
}


void op_plugin_set_stream_source::finish(zzub::song& song, bool send_events) {
	event_data.type = -1;
}

// ---------------------------------------------------------------------------
//
// op_pattern_edit
//
// ---------------------------------------------------------------------------

op_pattern_edit::op_pattern_edit(int _id, int _index, int _group, int _track, int _column, int _row, int _value) {
	id = _id;
	index = _index;

	group = _group;
	track = _track;
	column = _column;
	row = _row;
	value = _value;

	copy_flags.copy_plugins = true;	// TODO: read-only!!
}

bool op_pattern_edit::prepare(zzub::song& song) {

	event_data.type = event_type_edit_pattern;
	event_data.edit_pattern.plugin = song.plugins[id]->proxy;
	event_data.edit_pattern.index = index;
	event_data.edit_pattern.group = group;
	event_data.edit_pattern.track = track;
	event_data.edit_pattern.column = column;
	event_data.edit_pattern.row = row;
	event_data.edit_pattern.value = value;

	return true;
}

bool op_pattern_edit::operate(zzub::song& song) {

	assert(id < song.plugins.size());
	assert(song.plugins[id] != 0);
	assert(index >= 0 && (size_t)index < song.plugins[id]->patterns.size());
	assert(group >= 0 && group < song.plugins[id]->patterns[index]->groups.size());
	assert(track >= 0 && track < song.plugins[id]->patterns[index]->groups[group].size());
	assert(column >= 0 && column < song.plugins[id]->patterns[index]->groups[group][track].size());
	assert(row >= 0 && row < song.plugins[id]->patterns[index]->groups[group][track][column].size());

	const zzub::parameter* param = song.plugin_get_parameter_info(id, group, track, column);
	assert((value >= param->value_min && value <= param->value_max) || value == param->value_none  || (param->type == zzub::parameter_type_note && value == zzub::note_value_off));

	song.plugins[id]->patterns[index]->groups[group][track][column][row] = value;

	return true;
}

void op_pattern_edit::finish(zzub::song& song, bool send_events) {
	if (send_events) song.plugin_invoke_event(0, event_data, true);
}

// ---------------------------------------------------------------------------
//
// op_pattern_insert
//
// ---------------------------------------------------------------------------

op_pattern_insert::op_pattern_insert(int _id, int _index, zzub::pattern _pattern) {
	id = _id;
	index = _index;
	pattern = _pattern;
	copy_flags.copy_graph = true;
	copy_flags.copy_plugins = true;

	operation_copy_plugin_flags pluginflags;
	pluginflags.plugin_id = id;
	pluginflags.copy_plugin = true;
	pluginflags.copy_patterns = true;
	copy_flags.plugin_flags.push_back(pluginflags);
}

bool op_pattern_insert::prepare(zzub::song& song) {
	metaplugin& m = *song.plugins[id];

	if (index == -1)
		m.patterns.push_back(new zzub::pattern(pattern)); else
		m.patterns.insert(m.patterns.begin() + index, new zzub::pattern(pattern));

	event_data.type = event_type_new_pattern;
	event_data.new_pattern.plugin = m.proxy;
	event_data.new_pattern.index = m.patterns.size() - 1;
	return true;
}

bool op_pattern_insert::operate(zzub::song& song) {
	return true;
}

void op_pattern_insert::finish(zzub::song& song, bool send_events) {
	if (send_events) song.plugin_invoke_event(0, event_data, true);
}

// ---------------------------------------------------------------------------
//
// op_pattern_remove
//
// ---------------------------------------------------------------------------

op_pattern_remove::op_pattern_remove(int _id, int _index) {
	id = _id;
	index = _index;
	copy_flags.copy_graph = true;
	copy_flags.copy_plugins = true;
	copy_flags.copy_song_events = true;
	copy_flags.copy_sequencer_track_order = true;

	operation_copy_plugin_flags pluginflags;
	pluginflags.plugin_id = _id;
	pluginflags.copy_plugin = true;
	pluginflags.copy_patterns = true;
	copy_flags.plugin_flags.push_back(pluginflags);
}

bool op_pattern_remove::prepare(zzub::song& song) {
	assert(song.plugins[id] != 0 && id >= 0 && id < song.plugins.size());
	
	metaplugin& m = *song.plugins[id];

	if (index == -1) index = m.patterns.size() - 1;

	assert(index >= 0 && (size_t)index < m.patterns.size());

	event_data.type = event_type_pre_delete_pattern;
	event_data.delete_pattern.plugin = m.proxy;
	event_data.delete_pattern.index = index;
	song.plugin_invoke_event(0, event_data, true);

	event_data.type = event_type_delete_pattern;

	// remove pattern from pattern list
	delete m.patterns[index];
	m.patterns.erase(m.patterns.begin() + index);

	// adjust pattern indices in the sequencer

	for (size_t i = 0; i < song.song_events.size(); i++) {
		sequencer_event& ev = song.song_events[i];
		for (size_t j = 0; j < ev.actions.size(); j++) {
			sequencer_event::track_action& ta = ev.actions[j];
			if (song.sequencer_tracks[ta.first] == m.descriptor) {
				if (ta.second >= 0x10) {
					int pattern = ta.second - 0x10;
					//if (index == pattern) {
					//	remove_ops.push_back(op_sequencer_set_event(ev.timestamp, ta.first, -1));
					//} else
					assert(pattern != index);	// you should have deleted these already
					if (index < pattern) {
						ta.second--;
					}
				}
			}
		}
	}

	return true;
}

bool op_pattern_remove::operate(zzub::song& song) {
	return true;
}

void op_pattern_remove::finish(zzub::song& song, bool send_events) {
	if (send_events) song.plugin_invoke_event(0, event_data, true);
}

// ---------------------------------------------------------------------------
//
// op_pattern_move
//
// ---------------------------------------------------------------------------

op_pattern_move::op_pattern_move(int _id, int _index, int _newindex) {
	id = _id;
	index = _index;
	newindex = _newindex;
	copy_flags.copy_graph = true;
	copy_flags.copy_plugins = true;
	copy_flags.copy_song_events = true;
	copy_flags.copy_sequencer_track_order = true;

	operation_copy_plugin_flags pluginflags;
	pluginflags.plugin_id = id;
	pluginflags.copy_plugin = true;
	copy_flags.plugin_flags.push_back(pluginflags);
}

bool op_pattern_move::prepare(zzub::song& song) {
	metaplugin& m = *song.plugins[id];

	plugin_descriptor plugin = m.descriptor;
	assert(plugin != graph_traits<plugin_map>::null_vertex());

	std::vector<zzub::pattern*>& patterns = m.patterns;

	if (index == -1) index = patterns.size() - 1;

	assert(index >= 0 && (size_t)index < patterns.size());
	assert(newindex >= 0 && (size_t)newindex < patterns.size());

	zzub::pattern* patterncopy = patterns[index];
	patterns.erase(patterns.begin() + index);
	if (index < newindex)
		patterns.insert(patterns.begin() + newindex - 1, patterncopy); else
		patterns.insert(patterns.begin() + newindex, patterncopy);

	// update sequencer events
	for (size_t i = 0; i < song.song_events.size(); i++) {
		sequencer_event& ev = song.song_events[i];
		for (size_t j = 0; j < ev.actions.size(); j++) {
			sequencer_event::track_action& ta = ev.actions[j];
			if (song.sequencer_tracks[ta.first] == plugin) {
				if (ta.second >= 0x10) {
					int pattern = ta.second - 0x10;
					if (pattern == index) {
						ta.second = newindex;
					} else
					if (newindex < index && pattern >= newindex && pattern < index) {
						ta.second++;
					} else
					if (newindex > index && pattern > index && pattern <= newindex) {
						ta.second--;
					}
				}
			}
		}
	}

	event_data.type = event_type_plugin_changed;
	event_data.plugin_changed.plugin = m.proxy;
	return true;
}

bool op_pattern_move::operate(zzub::song& song) {
	return true;
}

void op_pattern_move::finish(zzub::song& song, bool send_events) {
	if (send_events) song.plugin_invoke_event(0, event_data, true);
}

// ---------------------------------------------------------------------------
//
// op_pattern_replace
//
// ---------------------------------------------------------------------------

op_pattern_replace::op_pattern_replace(int _id, int _index, const zzub::pattern& _pattern) {
	id = _id;
	index = _index;
	pattern = _pattern;

	copy_flags.copy_graph = true;
	copy_flags.copy_plugins = true;

	operation_copy_plugin_flags pluginflags;
	pluginflags.plugin_id = id;
	pluginflags.copy_plugin = true;
	pluginflags.copy_patterns = true;
	copy_flags.plugin_flags.push_back(pluginflags);

}

bool op_pattern_replace::prepare(zzub::song& song) {
	assert(id < song.plugins.size());
	assert(song.plugins[id] != 0);
	
	metaplugin& m = *song.plugins[id];

	delete m.patterns[index];
	m.patterns[index] = new zzub::pattern(pattern);

	event_data.type = event_type_pattern_changed;
	event_data.pattern_changed.plugin = m.proxy;
	event_data.pattern_changed.index = index;
	return true;
}

bool op_pattern_replace::operate(zzub::song& song) {
	return true;
}

void op_pattern_replace::finish(zzub::song& song, bool send_events) {
	if (send_events) song.plugin_invoke_event(0, event_data, true);
}

// ---------------------------------------------------------------------------
//
// op_pattern_insert_rows
//
// ---------------------------------------------------------------------------

op_pattern_insert_rows::op_pattern_insert_rows(int _id, int _index, int _row, std::vector<int> _columns, int _count) {
	id = _id;
	index = _index;
	columns = _columns;
	row = _row;
	count = _count;

	copy_flags.copy_graph = true;
	copy_flags.copy_plugins = true;

	operation_copy_plugin_flags pluginflags;
	pluginflags.plugin_id = id;
	pluginflags.copy_plugin = true;
	pluginflags.copy_patterns = true;
	copy_flags.plugin_flags.push_back(pluginflags);

}

bool op_pattern_insert_rows::prepare(zzub::song& song) {
	assert(id < song.plugins.size());
	assert(song.plugins[id] != 0);
	
	metaplugin& m = *song.plugins[id];

	zzub::pattern& p = *m.patterns[index];

	for (int i = 0; i < columns.size() / 3; i++) {
		int group = columns[i * 3 + 0];
		int track = columns[i * 3 + 1];
		int column = columns[i * 3 + 2];
		zzub::pattern::column& patterncolumn = p.groups[group][track][column];
		const zzub::parameter* param = song.plugin_get_parameter_info(id, group, track, column);
		patterncolumn.insert(patterncolumn.begin() + row, count, param->value_none);
		patterncolumn.erase(patterncolumn.begin() + p.rows, patterncolumn.end());
	}

	event_data.type = event_type_pattern_insert_rows;
	event_data.pattern_insert_rows.plugin = m.proxy;
	event_data.pattern_insert_rows.column_indices = &columns.front();
	event_data.pattern_insert_rows.indices = columns.size();
	event_data.pattern_insert_rows.index = index;
	event_data.pattern_insert_rows.row = row;
	event_data.pattern_insert_rows.rows = count;
	return true;
}

bool op_pattern_insert_rows::operate(zzub::song& song) {
	return true;
}

void op_pattern_insert_rows::finish(zzub::song& song, bool send_events) {

	if (send_events) song.plugin_invoke_event(0, event_data, true);
}


// ---------------------------------------------------------------------------
//
// op_pattern_remove_rows
//
// ---------------------------------------------------------------------------

op_pattern_remove_rows::op_pattern_remove_rows(int _id, int _index, int _row, std::vector<int> _columns, int _count) {
	id = _id;
	index = _index;
	columns = _columns;
	row = _row;
	count = _count;

	copy_flags.copy_graph = true;
	copy_flags.copy_plugins = true;

	operation_copy_plugin_flags pluginflags;
	pluginflags.plugin_id = id;
	pluginflags.copy_plugin = true;
	pluginflags.copy_patterns = true;
	copy_flags.plugin_flags.push_back(pluginflags);

	operation_copy_pattern_flags patternflags;
	patternflags.plugin_id = id;
	patternflags.index = index;
	copy_flags.pattern_flags.push_back(patternflags);

}

bool op_pattern_remove_rows::prepare(zzub::song& song) {
	assert(id < song.plugins.size());
	assert(song.plugins[id] != 0);
	
	metaplugin& m = *song.plugins[id];

	zzub::pattern& p = *m.patterns[index];

	for (int i = 0; i < columns.size() / 3; i++) {
		int group = columns[i * 3 + 0];
		int track = columns[i * 3 + 1];
		int column = columns[i * 3 + 2];
		zzub::pattern::column& patterncolumn = p.groups[group][track][column];
		const zzub::parameter* param = song.plugin_get_parameter_info(id, group, track, column);
		patterncolumn.erase(patterncolumn.begin() + row, patterncolumn.begin() + row + count);
		patterncolumn.insert(patterncolumn.end(), count, param->value_none);
	}

	event_data.type = event_type_pattern_remove_rows;
	event_data.pattern_remove_rows.plugin = m.proxy;
	event_data.pattern_remove_rows.column_indices = &columns.front();
	event_data.pattern_remove_rows.indices = columns.size();
	event_data.pattern_remove_rows.row = row;
	event_data.pattern_remove_rows.rows = count;
	event_data.pattern_remove_rows.index = index;

	return true;
}

bool op_pattern_remove_rows::operate(zzub::song& song) {
	return true;
}

void op_pattern_remove_rows::finish(zzub::song& song, bool send_events) {
	if (send_events) song.plugin_invoke_event(0, event_data, true);
}

// ---------------------------------------------------------------------------
//
// op_sequencer_set_event
//
// ---------------------------------------------------------------------------

op_sequencer_set_event::op_sequencer_set_event(int _timestamp, int _track, int _action) {
	timestamp = _timestamp;
	track = _track;
	action = _action;

	copy_flags.copy_plugins = true;
	copy_flags.copy_graph = true;
	copy_flags.copy_song_events = true;
}

bool op_sequencer_set_event::prepare(zzub::song& song) {

	std::vector<zzub::sequencer_event>::iterator pos = song.song_events.begin();
	int index = 0;

	while (song.song_events.size() && (size_t)index < song.song_events.size() && pos->timestamp < timestamp) {
		pos++;
		index++;
	}

	if (song.song_events.size() == 0 || pos == song.song_events.end()) {
		if (action != -1) {
			sequencer_event se;
			se.timestamp = timestamp;
			se.actions.push_back(sequencer_event::track_action(track, action));//[pt] = action;
			song.song_events.push_back(se);
		}
	} else
	if (pos->timestamp == timestamp) {
		bool found = false;
		for (size_t i = 0; i < pos->actions.size(); i++) {
			if (pos->actions[i].first == track) {
				if (action == -1) {
					pos->actions.erase(pos->actions.begin() + i);
				} else {
					pos->actions[i].second = action;
				}
				found = true;
				break;
			}
		}
		if (action != -1 && !found) {
			pos->actions.push_back(sequencer_event::track_action(track, action));//[pt] = action;
		} else
		if (action == -1 && found) {
			if (pos->actions.size() == 0) {
				song.song_events.erase(pos);
			}
		}
		//pos->actions[pt] = action;
	} else {
		if (action != -1 && pos != song.song_events.end()) {
			sequencer_event se;
			se.timestamp = timestamp;
			se.actions.push_back(sequencer_event::track_action(track, action));//[pt] = action;
			song.song_events.insert(pos, se);
		}
	}

	return true;
}

bool op_sequencer_set_event::operate(zzub::song& song) {
	return true;
}

void op_sequencer_set_event::finish(zzub::song& song, bool send_events) {
	event_data.type = event_type_set_sequence_event;
	event_data.set_sequence_event.plugin = 0;
	event_data.set_sequence_event.track = track;
	event_data.set_sequence_event.time = timestamp;
	song.plugin_invoke_event(0, event_data, true);
}

// ---------------------------------------------------------------------------
//
// op_sequencer_create_track
//
// ---------------------------------------------------------------------------

op_sequencer_create_track::op_sequencer_create_track(int _id) {
	id = _id;
	copy_flags.copy_graph = true;
	copy_flags.copy_plugins = true;
	copy_flags.copy_sequencer_track_order = true;
}

bool op_sequencer_create_track::prepare(zzub::song& song) {
	plugin_descriptor plugin = song.plugins[id]->descriptor;
	assert(plugin != graph_traits<plugin_map>::null_vertex());

	song.sequencer_tracks.push_back(plugin);
	return true;
}

bool op_sequencer_create_track::operate(zzub::song& song) {
	return true;
}

void op_sequencer_create_track::finish(zzub::song& song, bool send_events) {
	event_data.type = event_type_set_sequence_tracks;
	if (send_events) song.plugin_invoke_event(0, event_data, true);
}


// ---------------------------------------------------------------------------
//
// op_sequencer_remove_track
//
// ---------------------------------------------------------------------------

op_sequencer_remove_track::op_sequencer_remove_track(int _track) {
	track = _track;
	copy_flags.copy_song_events = true;
	copy_flags.copy_sequencer_track_order = true;
}

bool op_sequencer_remove_track::prepare(zzub::song& song) {
	if (track == -1) track = song.sequencer_tracks.size() - 1;
	assert(track >= 0 && track < song.sequencer_tracks.size());

	// adjust track indices
	for (size_t i = 0; i < song.song_events.size(); i++) {
		zzub::sequencer_event& ev = song.song_events[i];
		for (size_t j = 0; j < ev.actions.size(); j++) {
			zzub::sequencer_event::track_action& ta = ev.actions[j];
			if (ta.first > track)
				ta.first--;
		}
	}

	// after removing the events, finally let go of the plugin data
	song.sequencer_tracks.erase(song.sequencer_tracks.begin() + track);

	return true;
}

bool op_sequencer_remove_track::operate(zzub::song& song) {
	return true;
}

void op_sequencer_remove_track::finish(zzub::song& song, bool send_events) {
	event_data.type = zzub_event_type_sequencer_remove_track;
	if (send_events) song.plugin_invoke_event(0, event_data, true);
}


// ---------------------------------------------------------------------------
//
// op_sequencer_move_track
//
// ---------------------------------------------------------------------------

op_sequencer_move_track::op_sequencer_move_track(int _track, int _newtrack) {
	track = _track;
	newtrack = _newtrack;
	copy_flags.copy_song_events = true;
	copy_flags.copy_sequencer_track_order = true;
}

bool op_sequencer_move_track::prepare(zzub::song& song) {
	if (track == -1) track = song.sequencer_tracks.size() - 1;
	if (newtrack == -1) newtrack = song.sequencer_tracks.size() - 1;
	assert(track >= 0 && track < song.sequencer_tracks.size());
	assert(newtrack >= 0 && newtrack < song.sequencer_tracks.size());
	
	if (track == newtrack) return true;

	// TODO: move track in song.sequencer_tracks 

	plugin_descriptor trackcopy = song.sequencer_tracks[track];
	song.sequencer_tracks.erase(song.sequencer_tracks.begin() + track);
	if (song.sequencer_tracks.empty())
		song.sequencer_tracks.push_back(trackcopy); else
		song.sequencer_tracks.insert(song.sequencer_tracks.begin() + newtrack, trackcopy);

	// adjust all track indices in song.song_events
	for (size_t i = 0; i < song.song_events.size(); i++) {
		sequencer_event& ev = song.song_events[i];
		for (size_t j = 0; j < ev.actions.size(); j++) {
			sequencer_event::track_action& ta = ev.actions[j];
			if (ta.first == track) {
				ta.first = newtrack;
			} else
			if (newtrack < track && ta.first >= newtrack && ta.first < track) {
				ta.first++;
			} else
			if (newtrack > track && ta.first > track && ta.first <= newtrack) {
				ta.first--;
			}
		}
	}

	return true;
}

bool op_sequencer_move_track::operate(zzub::song& song) {
	return true;
}

void op_sequencer_move_track::finish(zzub::song& song, bool send_events) {
	event_data.type = event_type_set_sequence_tracks;
	if (send_events) song.plugin_invoke_event(0, event_data, true);
}


// ---------------------------------------------------------------------------
//
// op_sequencer_replace
//
// ---------------------------------------------------------------------------

op_sequencer_replace::op_sequencer_replace(const std::vector<sequencer_event>& _events) {
	song_events = _events;
	copy_flags.copy_song_events = true;
}

bool op_sequencer_replace::prepare(zzub::song& song) {
	song.song_events = song_events;
	return true;
}

bool op_sequencer_replace::operate(zzub::song& song) {
	return true;
}

void op_sequencer_replace::finish(zzub::song& song, bool send_events) {
	event_data.type = event_type_sequencer_changed;
	if (send_events) song.plugin_invoke_event(0, event_data, true);
}


// ---------------------------------------------------------------------------
//
// op_midimapping_insert
//
// ---------------------------------------------------------------------------

op_midimapping_insert::op_midimapping_insert(const zzub::midimapping& _midi_mapping) {
	midi_mapping = _midi_mapping;

	copy_flags.copy_midi_mappings = true;
}

bool op_midimapping_insert::prepare(zzub::song& song) {
	song.midi_mappings.push_back(midi_mapping);
	return true;
}

bool op_midimapping_insert::operate(zzub::song& song) {
	return true;
}


// ---------------------------------------------------------------------------
//
// op_midimapping_remove
//
// ---------------------------------------------------------------------------

op_midimapping_remove::op_midimapping_remove(int _index) {
	index = _index;

	copy_flags.copy_midi_mappings = true;
}

bool op_midimapping_remove::prepare(zzub::song& song) {
	song.midi_mappings.erase(song.midi_mappings.begin() + index);
	return true;
}

bool op_midimapping_remove::operate(zzub::song& song) {
	return true;
}


// ---------------------------------------------------------------------------
//
// op_wavetable_allocate_wavelevel
//
// ---------------------------------------------------------------------------

op_wavetable_allocate_wavelevel::op_wavetable_allocate_wavelevel(int _wave, int _level, int _sample_count, int _channels, int _format) {
	wave = _wave;
	level = _level;
	sample_count = _sample_count;
	channels = _channels;
	format = _format;

	copy_flags.copy_wavetable = true;
	operation_copy_wavelevel_flags wavelevel_flags;
	wavelevel_flags.wave = wave;
	wavelevel_flags.level = level;
	wavelevel_flags.copy_samples = true;
	copy_flags.wavelevel_flags.push_back(wavelevel_flags);
}

bool op_wavetable_allocate_wavelevel::prepare(zzub::song& song) {
	wave_info_ex& w = *song.wavetable.waves[wave];
	w.allocate_level(level, sample_count, (wave_buffer_type)format, channels == 2 ? true : false);
	return true;
}

bool op_wavetable_allocate_wavelevel::operate(zzub::song& song) {
	return true;
}

void op_wavetable_allocate_wavelevel::finish(zzub::song& song, bool send_events) {
}


// ---------------------------------------------------------------------------
//
// op_wavetable_add_wavelevel
//
// ---------------------------------------------------------------------------

op_wavetable_add_wavelevel::op_wavetable_add_wavelevel(zzub::player* _player, int _wave) {
	player = _player;
	wave = _wave;
	copy_flags.copy_wavetable = true;
	operation_copy_wave_flags wave_flags;
	wave_flags.wave = wave;
	wave_flags.copy_wave = true;
	copy_flags.wave_flags.push_back(wave_flags);
}

bool op_wavetable_add_wavelevel::prepare(zzub::song& song) {
	wave_info_ex& w = *song.wavetable.waves[wave];
	wave_level_ex wl;
	wl.proxy = new wavelevel_proxy(player, wave, w.levels.size());
	w.levels.push_back(wl);
	return true;
}

bool op_wavetable_add_wavelevel::operate(zzub::song& song) {
	return true;
}

void op_wavetable_add_wavelevel::finish(zzub::song& song, bool send_events) {
}


// ---------------------------------------------------------------------------
//
// op_wavetable_remove_wavelevel
//
// ---------------------------------------------------------------------------

op_wavetable_remove_wavelevel::op_wavetable_remove_wavelevel(int _wave, int _level) {
	wave = _wave;
	level = _level;
	copy_flags.copy_wavetable = true;
	operation_copy_wave_flags wave_flags;
	wave_flags.wave = wave;
	wave_flags.copy_wave = true;
	copy_flags.wave_flags.push_back(wave_flags);
}

bool op_wavetable_remove_wavelevel::prepare(zzub::song& song) {
	wave_info_ex& w = *song.wavetable.waves[wave];

	if (level == -1) level = w.levels.size() - 1;


	w.levels.erase(w.levels.begin() + level);

	event_data.type = event_type_delete_wave;
	event_data.delete_wave.wave = song.wavetable.waves[wave]->proxy;

	return true;
}

bool op_wavetable_remove_wavelevel::operate(zzub::song& song) {
	return true;
}

void op_wavetable_remove_wavelevel::finish(zzub::song& song, bool send_events) {
	//event_data.delete_wave.level = level;
	if (send_events) song.plugin_invoke_event(0, event_data, true);
}


// ---------------------------------------------------------------------------
//
// op_wavetable_move_wavelevel
//
// ---------------------------------------------------------------------------

op_wavetable_move_wavelevel::op_wavetable_move_wavelevel(int _wave, int _level, int _newlevel) {
	wave = _wave;
	level = _level;
	newlevel = _newlevel;
	copy_flags.copy_wavetable = true;
	operation_copy_wave_flags wave_flags;
	wave_flags.wave = wave;
	wave_flags.copy_wave = true;
	copy_flags.wave_flags.push_back(wave_flags);
}

bool op_wavetable_move_wavelevel::prepare(zzub::song& song) {
	wave_info_ex& w = *song.wavetable.waves[wave];

	wave_level_ex copylevel = w.levels[level];

	w.levels.erase(w.levels.begin() + level);
	if (w.levels.empty())
		w.levels.push_back(copylevel); else
		w.levels.insert(w.levels.begin() + newlevel, copylevel);
	return true;
}

bool op_wavetable_move_wavelevel::operate(zzub::song& song) {
	return true;
}

void op_wavetable_move_wavelevel::finish(zzub::song& song, bool send_events) {
}

// ---------------------------------------------------------------------------
//
// op_wavetable_wave_replace
//
// ---------------------------------------------------------------------------

op_wavetable_wave_replace::op_wavetable_wave_replace(int _wave, const wave_info_ex& _data) {
	wave = _wave;
	data = _data;

	copy_flags.copy_wavetable = true;
	operation_copy_wave_flags wave_flags;
	wave_flags.wave = wave;
	wave_flags.copy_wave = true;
	copy_flags.wave_flags.push_back(wave_flags);
}

bool op_wavetable_wave_replace::prepare(zzub::song& song) {
	// if stereo flag changes, we must reallocate all wavelevels so they are in the same format
	// but; this is very special
	wave_info_ex& w = *song.wavetable.waves[wave];
	w.fileName = data.fileName;
	w.name = data.name;
	w.volume = data.volume;
	w.flags = data.flags;
	w.envelopes = data.envelopes;
	//w.levels = data.levels;	// ikke så lurt tror jeg
	// flags er bidir, stereo, og evt annet sinnsykt

	event_data.type = event_type_wave_changed;
	event_data.change_wave.wave = song.wavetable.waves[wave]->proxy;

	return true;
}

bool op_wavetable_wave_replace::operate(zzub::song& song) {
	return true;
}

void op_wavetable_wave_replace::finish(zzub::song& song, bool send_events) {
	if (send_events) song.plugin_invoke_event(0, event_data, true);
}


// ---------------------------------------------------------------------------
//
// op_wavetable_wavelevel_replace
//
// ---------------------------------------------------------------------------

op_wavetable_wavelevel_replace::op_wavetable_wavelevel_replace(int _wave, int _level, const wave_level_ex& _data) {
	wave = _wave;
	level = _level;
	data = _data;
	copy_flags.copy_wavetable = true;
	operation_copy_wave_flags wave_flags;
	wave_flags.wave = wave;
	wave_flags.copy_wave = true;
	copy_flags.wave_flags.push_back(wave_flags);
}

bool op_wavetable_wavelevel_replace::prepare(zzub::song& song) {
	wave_level_ex& l = song.wavetable.waves[wave]->levels[level];
	l.loop_start = data.loop_start;
	l.loop_end = data.loop_end;
	l.root_note = data.root_note;
	l.samples_per_second = data.samples_per_second;

	// update legacy values
	wave_info_ex& w = *song.wavetable.waves[wave];
	bool is_extended = w.get_extended();
	if (is_extended) {
		l.legacy_loop_start = w.get_unextended_samples(level, data.loop_start);
		l.legacy_loop_end = w.get_unextended_samples(level, data.loop_end);
	} else {
		l.legacy_loop_start = data.loop_start;
		l.legacy_loop_end = data.loop_end;
	}

	event_data.type = event_type_wave_changed;
	event_data.change_wave.wave = song.wavetable.waves[wave]->proxy;

	return true;
}

bool op_wavetable_wavelevel_replace::operate(zzub::song& song) {
	return true;
}

void op_wavetable_wavelevel_replace::finish(zzub::song& song, bool send_events) {
	if (send_events) song.plugin_invoke_event(0, event_data, true);
}


// ---------------------------------------------------------------------------
//
// op_wavetable_insert_sampledata
//
// ---------------------------------------------------------------------------

op_wavetable_insert_sampledata::op_wavetable_insert_sampledata(int _wave, int _level, int _pos) {
	wave = _wave;
	level = _level;
	pos = _pos;
	copy_flags.copy_wavetable = true;
	operation_copy_wavelevel_flags wavelevel_flags;
	wavelevel_flags.wave = wave;
	wavelevel_flags.level = level;
	wavelevel_flags.copy_samples = true;
	copy_flags.wavelevel_flags.push_back(wavelevel_flags);
}

bool op_wavetable_insert_sampledata::prepare(zzub::song& song) {
	// we shall reallocate the backbuffer wave and insert the sample datas given to us
	wave_info_ex& w = *song.wavetable.waves[wave];
	int numsamples = w.get_sample_count(level);
	int newsamples = w.get_sample_count(level) + samples_length;
	int channels = w.get_stereo() ? 2 : 1;
	int bytes_per_sample = w.get_bytes_per_sample(level);
	wave_buffer_type format = w.get_wave_format(level);

	void* copybuffer = new char[bytes_per_sample * channels * numsamples];
	memcpy(copybuffer, w.get_sample_ptr(level), bytes_per_sample * channels * numsamples);
	
	bool allocw = w.allocate_level(level, newsamples, format, channels == 2 ? true : false);
	assert(allocw);

	CopySamples(copybuffer, w.get_sample_ptr(level), pos, format, format, samples_channels, channels, 0, 0);
	CopySamples(samples, w.get_sample_ptr(level), samples_length, samples_format, format, samples_channels, channels, 0, pos * channels);
	CopySamples(copybuffer, w.get_sample_ptr(level), numsamples - pos, format, format, samples_channels, channels, pos * samples_channels, (pos + samples_length) * channels);

	if (channels == 2) {
		CopySamples(copybuffer, w.get_sample_ptr(level), pos, format, format, samples_channels, channels, 1, 1);

		if (samples_channels == 2) {
			// copy stereo sample to stereo sample
			CopySamples(samples, w.get_sample_ptr(level), samples_length, samples_format, format, samples_channels, channels, 1, pos * channels + 1);
		} else {
			// copy mono sample to stereo sample
			CopySamples(samples, w.get_sample_ptr(level), samples_length, samples_format, format, samples_channels, channels, 0, pos * channels + 1);
		}

		CopySamples(copybuffer, w.get_sample_ptr(level), numsamples - pos, format, format, samples_channels, channels, pos * samples_channels + 1, (pos + samples_length) * channels + 1);

	}

	delete[] copybuffer;

	event_data.type = event_type_wave_allocated;
	event_data.allocate_wavelevel.wavelevel = song.wavetable.waves[wave]->levels[level].proxy;

	return true;
}

bool op_wavetable_insert_sampledata::operate(zzub::song& song) {
	return true;
}

void op_wavetable_insert_sampledata::finish(zzub::song& song, bool send_events) {
	//event_data.allocate_wavelevel.level = level;
	if (send_events) song.plugin_invoke_event(0, event_data, true);
}



// ---------------------------------------------------------------------------
//
// op_wavetable_remove_sampledata
//
// ---------------------------------------------------------------------------

op_wavetable_remove_sampledata::op_wavetable_remove_sampledata(int _wave, int _level, int _pos, int _samples) {
	wave = _wave;
	level = _level;
	pos = _pos;
	samples = _samples;

	copy_flags.copy_wavetable = true;
	operation_copy_wavelevel_flags wavelevel_flags;
	wavelevel_flags.wave = wave;
	wavelevel_flags.level = level;
	wavelevel_flags.copy_samples = true;
	copy_flags.wavelevel_flags.push_back(wavelevel_flags);
}

bool op_wavetable_remove_sampledata::prepare(zzub::song& song) {
	wave_info_ex& w = *song.wavetable.waves[wave];

	int bytes_per_sample = w.get_bytes_per_sample(level);
	int newsamples = w.get_sample_count(level) - samples;
	int numsamples = w.get_sample_count(level);
	int channels = w.get_stereo() ? 2 : 1;
	int format = w.get_wave_format(level);

	void* copybuffer = new char[numsamples * bytes_per_sample * channels];
	memcpy(copybuffer, w.get_sample_ptr(level), numsamples * bytes_per_sample * channels);

	// NOTE: this will delete[] live sample data unless wavelevel_copy_flags.copy_samples is set to true
	w.reallocate_level(level, newsamples);

	// copy non-erased parts of copybuffer back to sampledata
	CopySamples(copybuffer, w.get_sample_ptr(level), pos, format, format, channels, channels, 0, 0);
	CopySamples(copybuffer, w.get_sample_ptr(level), numsamples - (pos + samples), format, format, channels, channels, (pos + samples) * channels, pos * channels);

	if (channels == 2) {
		CopySamples(copybuffer, w.get_sample_ptr(level), pos, format, format, channels, channels, 1, 1);
		CopySamples(copybuffer, w.get_sample_ptr(level), numsamples - (pos + samples), format, format, channels, channels, (pos + samples) * channels + 1, pos * channels + 1);
	}
	delete[] copybuffer;

	event_data.type = event_type_wave_allocated;
	event_data.allocate_wavelevel.wavelevel = song.wavetable.waves[wave]->levels[level].proxy;

	return true;
}

bool op_wavetable_remove_sampledata::operate(zzub::song& song) {
	return true;
}

void op_wavetable_remove_sampledata::finish(zzub::song& song, bool send_events) {
	//event_data.allocate_wavelevel.level = level;
	if (send_events) song.plugin_invoke_event(0, event_data, true);
}

} // namespace zzub
