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
#include <functional>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <sstream>
#include "timer.h"
#include "tools.h"

using namespace std;

namespace zzub {

audio_connection_parameter_volume::audio_connection_parameter_volume() {
	set_word()
	.set_name("Volume")
	.set_description("Volume (0=0%, 4000=100%)")
	.set_value_min(0).set_value_max(0x4000)
	.set_value_none(0xffff)
	.set_state_flag()
	.set_value_default(0x4000);
}

audio_connection_parameter_panning::audio_connection_parameter_panning() {
	set_word()
	.set_name("Panning")
	.set_description("Panning (0=left, 4000=center, 8000=right)")
	.set_value_min(0).set_value_max(0x8000)
	.set_value_none(0xffff)
	.set_state_flag()
	.set_value_default(0x4000);
}

audio_connection_parameter_volume audio_connection::para_volume;
audio_connection_parameter_panning audio_connection::para_panning;

connection::connection() {
}

audio_connection::audio_connection() {
	type = connection_type_audio;
	values.amp = values.pan = 0x4000;
	cvalues = values;
	connection_values = &cvalues;
	connection_parameters.push_back(&para_volume);
	connection_parameters.push_back(&para_panning);
}

void audio_connection::process_events(zzub::song& player, const zzub::connection_descriptor& conn) {
	if (cvalues.amp != para_volume.value_none) 
		values.amp = cvalues.amp;
	if (cvalues.pan != para_panning.value_none) 
		values.pan = cvalues.pan;
}

bool audio_connection::work(zzub::song& player, const zzub::connection_descriptor& conn, int sample_count) {

	metaplugin& plugin_from = player.get_plugin(target(conn, player.graph));
	metaplugin& plugin_to = player.get_plugin(source(conn, player.graph));

	float *plout[] = { &plugin_to.work_buffer[0].front(), &plugin_to.work_buffer[1].front() };
	float *plin[] = { 0, 0 };
	if (plugin_from.last_work_frame != plugin_to.last_work_frame + plugin_to.last_work_buffersize) {
		plin[0] = &plugin_from.callbacks->feedback_buffer[0].front();
		plin[1] = &plugin_from.callbacks->feedback_buffer[1].front();
	} else {
		plin[0] = &plugin_from.work_buffer[0].front();
		plin[1] = &plugin_from.work_buffer[1].front();
	}

	bool plugin_to_does_input_mixing = (plugin_to.info->flags & zzub::plugin_flag_does_input_mixing) != 0;
	bool plugin_to_is_bypassed = plugin_to.is_bypassed || (plugin_to.sequencer_state == sequencer_event_type_thru);
	bool plugin_to_is_muted = plugin_to.is_muted || (plugin_to.sequencer_state == sequencer_event_type_mute);
	bool result = plugin_from.last_work_audio_result && values.amp > 0 && 
		(plugin_from.last_work_max_left > SIGNAL_TRESHOLD || plugin_from.last_work_max_right > SIGNAL_TRESHOLD);

	if (plugin_to_does_input_mixing && !plugin_to_is_bypassed && !plugin_to_is_muted) {
		if (result)
			plugin_to.plugin->input(plin, sample_count, values.amp / (float)0x4000); else
			plugin_to.plugin->input(0, 0, 0);
	} else {
		if (result)
			AddS2SPanMC(plout, plin, sample_count, values.amp / (float)0x4000, values.pan / (float)0x4000);
	}
	return result;
}

event_connection::event_connection() {
	type = connection_type_event;
	connection_values = 0;
}

const zzub::parameter *event_connection::getParam(struct metaplugin *mp, int group, int index)
{
	switch (group) {
		case 0: // input connections
			return connection_parameters[index];
/*		case 1: // globals
			return mp->loader->plugin_info->global_parameters[index];
		case 2: // track params
			return mp->loader->plugin_info->track_parameters[index];
		case 3: // controller params
			return mp->loader->plugin_info->controller_parameters[index];
*/
		default:
			return 0;
	}
}

int event_connection::convert(int value, const zzub::parameter *oldparam, const zzub::parameter *newparam) {
	int result = newparam->value_none;
	if (value != oldparam->value_none) {
		if ((oldparam->type == zzub::parameter_type_note) && (newparam->type == zzub::parameter_type_note))
		{
			result = value;
		}
		else
		{
			float v = oldparam->normalize(value);
			result = newparam->scale(v);
		}
	}
	return result;
}

void event_connection::process_events(zzub::song& player, const zzub::connection_descriptor& conn) {
	const zzub::parameter *param_in;
	const zzub::parameter *param_out;

	int to_id = player.get_plugin_id(source(conn, player.graph));
	int from_id = player.get_plugin_id(target(conn, player.graph));

	std::vector<event_connection_binding>::iterator b;
	for (b = bindings.begin(); b != bindings.end(); ++b) {
		param_in = player.plugin_get_parameter_info(from_id, 3, 0, b->source_param_index);
		param_out = player.plugin_get_parameter_info(to_id, b->target_group_index, b->target_track_index, b->target_param_index);

		int sv = player.plugin_get_parameter_direct(to_id, b->target_group_index, b->target_track_index, b->target_param_index);

		//patterntrack* pt=plugin_out->getStateTrack(b->target_group_index, b->target_track_index);
		//int sv = pt->getValue(0, b->target_param_index);
		//~ int sv = plugin_out->getParameter(b->target_group_index, b->target_track_index, b->target_param_index);
		player.plugin_set_parameter_direct(from_id, 3, 0, b->source_param_index, convert(sv, param_out, param_in), false);
		//plugin_in->setParameter(3, 0, b->source_param_index, convert(sv, param_out, param_in), false);
	}
	
	// transfer controller parameters to live state
	metaplugin& from_m = *player.plugins[from_id];
	//plugin_in->controllerState.applyControlChanges();

	player.transfer_plugin_parameter_track_row(from_id, 3, 0, from_m.state_write, from_m.plugin->controller_values, 0, true);
	player.plugins[from_id]->plugin->process_controller_events();
	//plugin_in->machine->process_controller_events();

	// transfer controller parameters from live to local states
	player.transfer_plugin_parameter_track_row(from_id, 3, 0, from_m.plugin->controller_values, from_m.state_write, 0, false);
	//plugin_in->controllerState.copyBackControlChanges();

	for (b = bindings.begin(); b != bindings.end(); ++b) {
		param_in = player.plugin_get_parameter_info(from_id, 3, 0, b->source_param_index);
		param_out = player.plugin_get_parameter_info(to_id, b->target_group_index, b->target_track_index, b->target_param_index);
		//param_in = getParam(plugin_in,3,b->source_param_index);
		//param_out = getParam(plugin_out,b->target_group_index, b->target_param_index);
		//int sv = plugin_in->getParameter(3, 0, b->source_param_index);
		int sv = player.plugin_get_parameter(from_id, 3, 0, b->source_param_index);

		// TODO: this should write directly to the state, not via the control
		//plugin_out->setParameter(b->target_group_index, b->target_track_index, b->target_param_index, sv, false);
		int cv = convert(sv, param_in, param_out);
		player.plugin_set_parameter_direct(to_id, b->target_group_index, b->target_track_index, b->target_param_index, cv, false);
		//plugin_out->invokeEvent(eventData);

		/*patterntrack* pt=plugin_out->getStateTrack(b->target_group_index, b->target_track_index);
		if (pt) {
			int cv = convert(sv, param_in, param_out);
			pt->setValue(0, b->target_param_index, cv);
			zzub_event_data eventData = { event_type_parameter_changed };
			eventData.change_parameter.group = b->target_group_index;
			eventData.change_parameter.track = b->target_track_index;
			eventData.change_parameter.param = b->target_param_index;
			eventData.change_parameter.value = cv;
			plugin_out->invokeEvent(eventData);
		}*/
	}
}

bool event_connection::work(zzub::song& player, const zzub::connection_descriptor& conn, int sample_count) {

	return true;
}

midi_connection::midi_connection() {
	type = connection_type_midi;
	connection_values = 0;
}

void midi_connection::process_events(zzub::song& player, const zzub::connection_descriptor& conn) {
}

bool midi_connection::work(zzub::song& player, const zzub::connection_descriptor& conn, int sample_count) {
	int to_id = player.get_plugin_id(source(conn, player.graph));
	int from_id = player.get_plugin_id(target(conn, player.graph));

	metaplugin& m = *player.plugins[from_id];
	if (m.midi_messages.size() == 0) return false;

	device = get_midi_device(player, to_id, device_name);
	for (size_t i = 0; i < m.midi_messages.size(); i++) {
		m.midi_messages[i].device = device;
	}

	metaplugin& mout = *player.plugins[to_id];
	mout.plugin->process_midi_events(&m.midi_messages.front(), (int)m.midi_messages.size());

	return true;
}

int midi_connection::get_midi_device(zzub::song& player, int plugin_id, std::string name) {
	_midiouts midiouts;
	player.plugins[plugin_id]->plugin->get_midi_output_names(&midiouts);
	std::vector<string>::iterator i = find(midiouts.names.begin(), midiouts.names.end(), name);
	if (i == midiouts.names.end()) return -1;
	// we really need to find the _global_ device index, not the target machine one.... ? ?? ???
	return (int)(i - midiouts.names.begin());
}

} // namespace zzub
