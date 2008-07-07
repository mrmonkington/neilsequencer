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
#include "tools.h"

using namespace std;

namespace zzub {

/*! \struct master_plugin_info
	\brief Master plugin description
*/
/*! \struct master_plugin
	\brief Master plugin implementation
*/

	/***

    master_metaplugin

***/

const int NO_MASTER_VOLUME = 0xFFFF;
const int NO_MASTER_BPM = 0xFFFF;
const int NO_MASTER_TPB = 0xFF;


master_plugin_info::master_plugin_info() {
	this->flags = zzub::plugin_flag_is_root | zzub::plugin_flag_has_audio_output | zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_midi_input;
	this->name = "Master";
	this->short_name = "Master";
	this->author = "n/a";
	this->uri = "@zzub.org/master";

	add_global_parameter()
		.set_word()
		.set_name("Volume")
		.set_description("Master Volume (0=0 dB, 4000=-80 dB)")
		.set_value_min(0)
		.set_value_max(0x4000)
		.set_value_none(NO_MASTER_VOLUME)
		.set_state_flag()
		.set_value_default(0);

	add_global_parameter()
		.set_word()
		.set_name("BPM")
		.set_description("Beats Per Minute (10-200 hex)")
		.set_value_min(16)
		.set_value_max(512)
		.set_value_none(NO_MASTER_BPM)
		.set_state_flag()
		.set_value_default(126);

	add_global_parameter()
		.set_byte()
		.set_name("TPB")
		.set_description("Ticks Per Beat (1-20 hex)")
		.set_value_min(1)
		.set_value_max(32)
		.set_value_none(NO_MASTER_TPB)
		.set_state_flag()
		.set_value_default(4);

}

zzub::plugin* master_plugin_info::create_plugin() const { 
	return new master_plugin(); 
}

bool master_plugin_info::store_info(zzub::archive *) const { 
	return false; 
}


/***

    master_plugin

***/

master_plugin::master_plugin() {
	global_values = gvals = &dummy;
	track_values = 0;
	attributes = 0;

	samples_per_second = 0;

	gvals->bpm = 125;
	gvals->tpb = 4;
	gvals->volume = 0;
}

void master_plugin::init(zzub::archive*) {
	update_midi_devices();
}

void master_plugin::process_events() {
	bool changed = false;

	int bpm = gvals->bpm;
	if (bpm == NO_MASTER_BPM)
		bpm = _master_info->beats_per_minute; else
		changed = true;

	int tpb = gvals->tpb;
	if (tpb == NO_MASTER_TPB)
		tpb = _master_info->ticks_per_beat; else
		changed = true;

	if (samples_per_second != _master_info->samples_per_second) {
		samples_per_second = _master_info->samples_per_second;
		changed = true;
	}

	if (changed) update_tempo(bpm, tpb);

	int volume = gvals->volume;
	if (volume != NO_MASTER_VOLUME) master_volume = volume;
}


bool master_plugin::process_stereo(float **pin, float **pout, int numSamples, int mode) { 
	using namespace std;
	if (mode==zzub::process_mode_write) return false;

	float db = ((float)master_volume / (float)0x4000)*-80.0f;
	float amp = dB_to_linear(db);
	for (int i = 0; i < numSamples; i++) {
		pout[0][i] *= amp;
		pout[1][i] *= amp;
	}
	return true;
}


void master_plugin::update_tempo(int bpm, int tpb) {
	_master_info->beats_per_minute = bpm;
	_master_info->ticks_per_beat = tpb;
	
	float frac = ((float)60.0*_master_info->samples_per_second) / ((float)bpm*tpb);
	_master_info->ticks_per_second = (float)_master_info->samples_per_second / frac;

	float n;
	_master_info->samples_per_tick_frac = modf(frac, &n);
	_master_info->samples_per_tick = (int)n;
	_master_info->tick_position = 0;
}

void master_plugin::process_midi_events(midi_message* pin, int nummessages) {
	midi_io* driver = _host->_player->midiDriver;
	if (!driver) return ;
	
	for (int i = 0; i < nummessages; i++) {
		float latency = (_host->_player->work_latency / 2) + _host->_player->work_buffer_position + (float)pin[i].timestamp;
		float samples_per_ms = (float)_host->plugin_player->master_info.samples_per_second / 1000.0f;

		int time_ms = (int)(latency / samples_per_ms);	// get latency and write position in ms from audio driver
		int device = midi_devices[pin[i].device].first;
		// this device is relative to which one is output and open
		driver->schedule_send(device, time_ms, pin[i].message);
	}
}

void master_plugin::update_midi_devices() {
	midi_devices.clear();
	
	midi_io* driver = _host->_player->midiDriver;
	if (!driver) return ;

	for (unsigned int i=0; i<driver->getDevices(); i++) {
		if (!driver->isOutput(i)) continue;
		if (!driver->isOpen(i)) continue;

		const char* name = driver->getDeviceName(i);
		midi_devices.push_back(std::pair<int, std::string>(i, name));
	}
}

void master_plugin::get_midi_output_names(outstream *pout) {

	update_midi_devices();

	for (size_t i = 0; i < midi_devices.size(); i++) {
		string name = midi_devices[i].second;
		pout->write((void*)name.c_str(), (unsigned int)(name.length()) + 1);
	}
	
}

} // namespace zzub
