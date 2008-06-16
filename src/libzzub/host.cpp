/*
Copyright (C) 2003-2007 Anders Ervik <calvin@countzero.no>

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

using namespace std;

namespace zzub {

/*! \struct host
	\brief The host interface exposes methods to the plugins.

	Each plugin gets its own pointer to a host object for retreiving
	info from and manipulating the player.
*/

host::host(zzub::player* play, zzub_plugin_t* plug) {
	_player = play;
	_plugin = plug;

	for (int c = 0; c < 2; ++c) {
		auxBuffer[c] = new float[zzub::buffer_size * sizeof(float) * 4];
		memset(auxBuffer[c], 0, zzub::buffer_size * sizeof(float) * 4);
	}
}

host::~host() {
	for (int c = 0; c < 2; ++c) {
		delete[] auxBuffer[c];
	}
}
 
const wave_info* host::get_wave(int const i) {
	if (i<1 || i>0xc8) return 0;
	wave_info_ex* wi = plugin_player->wavetable.waves[i-1];
	return wi;
}

const wave_level* host::get_wave_level(int i, int level) {
	wave_info_ex* waveInfo = (wave_info_ex*)get_wave(i);
	if (waveInfo == 0 || waveInfo->get_levels() <= level) return 0;
	return waveInfo->get_level(level);
}


void host::message(char const *txt) {
//	::MessageBox(0, txt, _metaplugin->name.c_str(), MB_OK);
#if defined(_WIN32)
	::MessageBox(0, txt, "MICallbacks", MB_OK);
#else
	printf(txt);
#endif
}

void host::lock() {
	_player->swap_lock.lock();
}

void host::unlock() {
	_player->swap_lock.unlock();
}

void host::set_swap_mode(bool free) {
	_player->swap_mode = free;
}

int host::get_write_position() {
	return _player->front.work_position;
}

float **host::get_auxiliary_buffer() { 
	return auxBuffer;
}

void host::clear_auxiliary_buffer() { 
	float **buffer = get_auxiliary_buffer();
	memset(buffer[0], 0, zzub::buffer_size * sizeof(float) * 4);
	memset(buffer[1], 0, zzub::buffer_size * sizeof(float) * 4);
}

int host::get_next_free_wave_index() {
	wave_table& wt = plugin_player->wavetable;
	for (size_t i = 0; i < wt.waves.size(); i++) {
		if (wt.waves[i]->get_levels() == 0) return (int)i+1;
	}
	return 0;
}

bool host::allocate_wave(int i, int level, int samples, wave_buffer_type type, bool stereo, char const *name) {
	assert(i > 0);
	wave_table& wt = plugin_player->wavetable;
	wave_info_ex& w = *wt.waves[i - 1];
	w.clear();
	w.name = name;
	w.volume = 1.0;
	w.flags = wave_flag_envelope;	// TODO? stereo or mono??
	w.envelopes.push_back(envelope_entry());
	if (!w.allocate_level(level, samples, type, stereo)) {
		w.clear();
		return false;
	}

	// need to tell someone we updated, so windows can be redrawn
	zzub_event_data event_data = {event_type_wave_allocated};
	plugin_player->plugin_invoke_event(0, event_data);

	return true;
}


/*void host::get_midi_output_names(outstream *pout) {
	// return a list of open midi output devices

	midi_io* driver = _metaplugin->player->midiDriver;
	if (!driver) return ;
	for (size_t i=0; i<driver->getDevices(); i++) {
		if (!driver->isOutput(i)) continue;
		if (!driver->isOpen(i)) continue;

		const char* name = driver->getDeviceName(i);
		pout->write((void*)name, strlen(name)+1);
	}
}

int host::get_midi_device(const char* device_name) {
	midi_io* driver = _metaplugin->player->midiDriver;
	for (size_t i=0; i<driver->getDevices(); i++) {
		if (!driver->isOutput(i)) continue;
		const char* name = driver->getDeviceName(i);
		if (strcmp(device_name, name) == 0) return i;
	}
	return -1;
}

void host::midi_out(int const dev, unsigned int data) {

	// TODO: libzzub midi device indexes are all-device-based, 
	// dev values coming from the buzz wrapper are output-device-based

	midi_io* driver = _metaplugin->player->midiDriver;
	if (!driver) return ;
	
	float latency = _metaplugin->player->workLatency + _metaplugin->player->workBufpos;
	float samples_per_ms = (float)_metaplugin->player->masterInfo.samples_per_second / 1000.0f;

	int time_ms = latency / samples_per_ms;	// get latency and write position in ms from audio driver
	driver->schedule_send(dev, time_ms, data);

	_metaplugin->lastMidiState = true;
}
*/

void host::midi_out(int time, unsigned int data) {
	midi_message msg = { -1, data, time };
	metaplugin& m = *plugin_player->plugins[_plugin->id];
	m.midi_messages.push_back(msg);
}

// envelopes

int host::get_envelope_size(int wave, int env) {
	wave_info_ex* wi = (wave_info_ex*)get_wave(wave);

	if (env < 0) return 0;
	if (wi == 0) return 0;
	if (env >= (int)wi->envelopes.size()) return 0;
		
	if (wi->envelopes[env].disabled)
		return 0;

	return (int)wi->envelopes[env].points.size();
}

bool host::get_envelope_point(int wave, int env, int i, unsigned short &x, unsigned short &y, int &flags) {
	wave_info_ex* wi = (wave_info_ex*)get_wave(wave);

	if (env < 0) return false;
	if (wi == 0) return false;
	if (env >= (int)wi->envelopes.size()) return false;
	if (i >= (int)wi->envelopes[env].points.size()) return false;

	envelope_point &pt = wi->envelopes[env].points[i];
	x = pt.x;
	y = pt.y;
	flags = pt.flags;
	return true;
}


const wave_level* host::get_nearest_wave_level(int i, int note) {
	wave_info_ex* wi = (wave_info_ex*)get_wave(i);
	if (wi == 0) return 0;
	
	int nearestIndex = -1;
	int nearestNote = 0;
	for (int j = 0; j < wi->get_levels(); j++) {
		int levelNote = wi->get_root_note(j);
		if (abs(note-levelNote) < abs(note-nearestNote)) {
			nearestNote = levelNote;
			nearestIndex = j;
		}
	}

	if (nearestIndex<0) nearestIndex = 0;

	return get_wave_level(i, nearestIndex);
}


// pattern editing - never call any of these in tick or work, only init and command allowed

void host::set_track_count(int const n) {
	assert(false);
}

pattern* host::create_pattern(char const* name, int const length) {
	assert(false);
	return 0;
}

pattern* host::get_pattern(int const index) {
	assert(false);
	return 0;
}

char const* host::get_pattern_name(pattern* ppat) {
	assert(false);
	return "";
}

void host::rename_pattern(char const* oldname, char const* newname) {
	assert(false);
}

void host::delete_pattern(pattern* ppat) {
	assert(false);
}

int host::get_pattern_data(pattern* ppat, int const row, int const group, int const track, int const field) {
	assert(false);
	return 0;
}

void host::set_pattern_data(pattern* ppat, int const row, int const group, int const track, int const field, int const value) {
	assert(false);
}
	
// sequence editing
sequence* host::create_sequence() {
	message("CreateSequence not implemented");
	return 0;
}

void host::delete_sequence(sequence* pseq) {
	message("DeleteSequence not implemented");
}


// special ppat values for GetSequenceData and SetSequenceData 
// empty = NULL
// <break> = (CPattern *)1
// <mute> = (CPattern *)2
// <thru> = (CPattern *)3
pattern *host::get_sequence_data(int const row) {
	message("GetSequenceData not implemented");
	return 0;
}

void host::set_sequence_data(int const row, pattern* ppat) {
	message("SetSequenceData not implemented");
}

	

// buzz v1.2 (MI_VERSION 15) additions start here

// group 1=global, 2=track

void host::_legacy_control_change(int group, int track, int param, int value) {					// set value of parameter
	message("ControlChange__obsolete__ not implemented");
}


// direct calls to audiodriver, used by WaveInput and WaveOutput
// shouldn't be used for anything else
int host::audio_driver_get_channel_count(bool input) {
	if (input) {
		return _player->work_in_device!=0 ? _player->work_in_device->in_channels : 0;
	} else {
		return _player->work_out_device->out_channels;
	}
}

void host::audio_driver_write(int channel, float *psamples, int numsamples) {
	memcpy(_player->front.outputBuffer[channel], psamples, sizeof(float) * numsamples);
}

void host::audio_driver_read(int channel, float *psamples, int numsamples) {
	if (_player->front.inputBuffer[channel] == 0) return ;

	memcpy(psamples, _player->front.inputBuffer[channel], sizeof(float) * numsamples);
}

metaplugin_proxy* host::get_metaplugin() {
	return _plugin;
}

void host::control_change(metaplugin_proxy* pmac, int group, int track, int param, int value, bool record, bool immediate) {
	if (plugin_player->plugins[pmac->id] == 0) return ;
	metaplugin& m = *plugin_player->plugins[pmac->id];
	
	if (group == 2 && track >= m.tracks) return ;

	plugin_player->plugin_set_parameter_direct(pmac->id, group, track, param, value, record);

	char* param_ptr = 0;
	int track_size;
	switch (group) {
		case 0:
			assert(false);
			break;
		case 1:
			param_ptr = (char*)m.plugin->global_values;
			break;
		case 2:
			param_ptr = (char*)m.plugin->track_values;
			track_size = plugin_player->get_plugin_parameter_track_row_bytesize(pmac->id, group, track);
			param_ptr += track * track_size;
			break;
	}
	plugin_player->transfer_plugin_parameter_track_row(pmac->id, group, track, m.state_write, param_ptr, 0, false); 

	//if (immediate)
	//	pmac->tickAsync();
}

// peerctrl extensions
int host::get_parameter(metaplugin_proxy* _metaplugin, int group, int track, int param) {
	return plugin_player->plugin_get_parameter(_metaplugin->id, group, track, param);
}

plugin *host::get_plugin(metaplugin_proxy* _metaplugin) {
	return plugin_player->plugins[_metaplugin->id]->plugin;//_metaplugin->plugin;
}

// returns pointer to the sequence if there is a pattern playing
sequence* host::get_playing_sequence(metaplugin_proxy* pmacid) {
	message("GetPlayingSequence not implemented");
	return 0;
}


// gets ptr to raw pattern data for row of a track of a currently playing pattern (or something like that)
void* host::get_playing_row(sequence* pseq, int group, int track) {
	message("GetPlayingRow not implemented");
	return 0;
}

// GetStateFlags fixed selecting a VSTi's during playback
int host::get_state_flags() {
	return (zzub_player_state)_player->front.state==zzub_player_state_playing?state_flag_playing:0;
}

void host::set_state_flags(int state) {
	if (state==0)
		_player->set_state(player_state_stopped); else
		_player->set_state(player_state_playing);
}


void host::set_event_handler(metaplugin_proxy* pmac, event_handler* handler) {
	plugin_player->plugins[pmac->id]->event_handlers.push_back(handler);
}

void host::remove_event_handler(metaplugin_proxy* pmac, event_handler* handler) {
	std::vector<event_handler*>& handlers = plugin_player->plugins[pmac->id]->event_handlers;
	std::vector<event_handler*>::iterator i = find(handlers.begin(), handlers.end(), handler);
	if (i == handlers.end()) return ;
	handlers.erase(i);

	// clear events in queue using this handler
	int read_pos = plugin_player->user_event_queue_read;
	while (read_pos != plugin_player->user_event_queue_write) {
		event_message& ev = plugin_player->user_event_queue[read_pos];
		if (ev.event == handler) ev.event = 0;

		if (read_pos == plugin_player->user_event_queue.size() - 1)
			read_pos = 0; else
			read_pos++;
	}
}

char const *host::get_wave_name(int const i) {
	if (i < 1 || (size_t)i >= plugin_player->wavetable.waves.size()) return 0;
	wave_info_ex& we = *plugin_player->wavetable.waves[i-1];
	return we.name.c_str();
}

// i >= 1, NULL name to clear
void host::set_internal_wave_name(metaplugin_proxy* pmac, int const i, char const *name) {
	message("SetInternalWaveName not implemented");
}

void host::get_plugin_names(outstream *pout) {	// should be metapluginnames?
	for (int i = 0; i < plugin_player->get_plugin_count(); i++) {
		metaplugin& m = plugin_player->get_plugin(i);
		pout->write((void*)m.name.c_str(), (int)m.name.length()+1);
	}
}

metaplugin_proxy* host::get_metaplugin(char const *name) {
	if (name == 0) return 0;
	plugin_descriptor plugindesc = plugin_player->get_plugin_descriptor(name);
	if (plugindesc == graph_traits<plugin_map>::null_vertex()) return 0;
	int id = plugin_player->get_plugin_id(plugindesc);
	return plugin_player->plugins[id]->proxy;
}

/*int host::get_metaplugin_by_index(int plugin_desc) {
	return plugin_player->get_plugin_id(plugin_desc);
}*/

const info* host::get_info(metaplugin_proxy* pmac) {
	assert(pmac->id != -1);
	assert(plugin_player->plugins[pmac->id] != 0);
	return plugin_player->plugins[pmac->id]->info;
}

const char* host::get_name(metaplugin_proxy* pmac) {
	assert(pmac->id != -1);
	if (plugin_player->plugins[pmac->id] == 0) return 0; // could happen if a peer controlled machine is deleted
	return plugin_player->plugins[pmac->id]->name.c_str();
}

bool host::get_input(int index, float *psamples, int numsamples, bool stereo, float *extrabuffer) {
	message("GetInput not implemented");
	return false;
}

bool host::get_osc_url(metaplugin_proxy* pmac, char *url) {
	sprintf(url, "osc.udp://localhost:7770/%s", plugin_player->plugins[pmac->id]->name.c_str());
	return true;
}

int host::get_play_position() {
	return _player->front.song_position;
}

void host::set_play_position(int pos) {
	printf("host::set_play_position %i\n", pos);
	_player->front.song_position = pos;
}

int host::get_song_begin() {
	return _player->front.song_begin;
}

void host::set_song_begin(int pos) {
	_player->front.song_begin = pos;
}

int host::get_song_end() {
	return _player->front.song_end;
}

void host::set_song_end(int pos) {
	_player->front.song_end = pos;
}

int host::get_song_begin_loop() {
	return _player->front.song_loop_begin;
}

void host::set_song_begin_loop(int pos) {
	_player->front.song_loop_begin = pos;
}

int host::get_song_end_loop() {
	return _player->front.song_loop_end;
}

void host::set_song_end_loop(int pos) {
	_player->front.song_loop_end = pos;
}

};
