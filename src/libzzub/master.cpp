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
/*! \struct master_pluginloader
	\brief Master plugin loader
*/

/***

    master_metaplugin

***/

const int NO_MASTER_VOLUME=0xFFFF;
const int NO_MASTER_BPM=0xFFFF;
const int NO_MASTER_TPB=0xFF;

struct master_plugin_info : zzub::info {
	master_plugin_info() {
		this->flags = zzub::plugin_flag_mono_to_stereo | zzub::plugin_flag_is_root | zzub::plugin_flag_has_audio_input;
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
	
	virtual zzub::plugin* create_plugin() const { return 0; }
	virtual bool store_info(zzub::archive *) const { return false; }
} masterInfo;

/***

    master_plugin

***/

master_plugin::master_plugin() {
    global_values=gvals=&dummyValuesUsedWhenNotHacked;
    track_values=0;
    attributes=0;

    samplesPerSecond=0;

    gvals->BPM=125;
    gvals->TPB=4;
    gvals->Volume=0;
}


void master_plugin::process_events() {
    bool changed=false;

    int bpm=gvals->BPM;
    if (bpm==NO_MASTER_BPM)
        bpm=_master_info->beats_per_minute; else
        changed=true;

    int tpb=gvals->TPB;
    if (tpb==NO_MASTER_TPB)
        tpb=_master_info->ticks_per_beat; else
        changed=true;

    if (samplesPerSecond!=_master_info->samples_per_second) {
        samplesPerSecond=_master_info->samples_per_second;
        changed=true;
    }
    // TODO: also test for change in samplesPerSec

    if (changed)
        updateSpeed(bpm, tpb);

    int volume=gvals->Volume;
    if (volume!=NO_MASTER_VOLUME)
        masterVolume=volume;
}


bool master_plugin::process_stereo(float **pin, float **pout, int numSamples, int mode) { 
    using namespace std;
    if (mode==zzub::process_mode_write) return false;

    float db=((float)masterVolume / (float)0x4000)*-80.0f;
    Amp(pout[0], numSamples, dB_to_linear(db));
    Amp(pout[1], numSamples, dB_to_linear(db));

    return true;
}


void master_plugin::updateSpeed(int bpm, int tpb) {
    _master_info->beats_per_minute=bpm;
    _master_info->ticks_per_beat=tpb;
    //_master_info->samples_per_second=player->samplesPerSec;
    
    float frac=(60.0*_master_info->samples_per_second) / ((double)bpm*tpb);
    _master_info->ticks_per_second=(float)_master_info->samples_per_second / frac;

    double n;
    _master_info->samples_per_tick_frac=modf(frac, &n);
    _master_info->samples_per_tick=n;
}

/***

    master pluginloader

***/

master_pluginloader::master_pluginloader() : pluginloader(0, &masterInfo) {
}

plugin* master_pluginloader::createMachine() {
    return new master_plugin();
}

} // namespace zzub
