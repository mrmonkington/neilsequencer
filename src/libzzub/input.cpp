/*
Copyright (C) 2003-2007 Anders Ervik <calvin@countzero.no>
Copyright (C) 2006-2007 Leonard Ritter

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
#include "driver.h"
#include <sstream>
#include <cassert>

using namespace std;

namespace zzub {

/*! \struct input_plugin
	\brief Built-in recording plugin
*/
/*! \struct input_plugin_info
	\brief Description of a built-in recording plugin
*/
/*! \struct input_plugincollection
	\brief Built-in recording plugin loader
*/

struct input_plugin : plugin {
	int attributeValues[1];
	
	input_plugin() {
		attributes = attributeValues;
		attributeValues[0] = 0;
	}
	
	virtual void destroy() { delete this; }
	virtual void init(zzub::archive *arc) {}
	virtual void process_events() {
		attributes_changed();
	}
	virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) { return false; }
	virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode) {
		if( (mode&zzub::process_mode_write)==0 )
			return false;
		_host->audio_driver_read(attributeValues[0] * 2 + 0, pout[0], numsamples);
		_host->audio_driver_read(attributeValues[0] * 2 + 1, pout[1], numsamples);
		return true;
	}
	virtual void process_controller_events() {}
	virtual void stop() {}
	virtual void load(zzub::archive *arc) {}
	virtual void save(zzub::archive *arc) {}
	virtual void attributes_changed() {
		if (attributeValues[0]<0 || attributeValues[0] >= _host->audio_driver_get_channel_count(true) / 2) {
			attributeValues[0] = 0;
		}
	}
	virtual void command(int index) {
		if (index>=0x100 && index < 0x200) {
			cout << "input command " << index << endl;
			int channel = index - 0x100;
			attributeValues[0] = channel;
			attributes_changed();
		}
	}
	virtual void process_midi_events(midi_message* pin, int nummessages) {}
	virtual void get_midi_output_names(outstream *pout) {}
	virtual void set_track_count(int count) {}
	virtual void mute_track(int index) {}
	virtual bool is_track_muted(int index) const { return false; }
	virtual void midi_note(int channel, int value, int velocity)  {}
	virtual void event(unsigned int data)  {}
	virtual const char * describe_value(int param, int value) { return 0; }
	virtual const zzub::envelope_info ** get_envelope_infos() { return 0; }
	virtual bool play_wave(int wave, int note, float volume) { return false; }
	virtual void stop_wave() {}
	virtual int get_wave_envelope_play_position(int env) { return -1; }
	virtual const char* describe_param(int param) { return 0; }
	virtual bool set_instrument(const char *name) { return false; }
	virtual void get_sub_menu(int index, zzub::outstream *os) { 
		cout << "get_sub_menu index " << index << endl;
		if (index != 0) return ;
		for (int i = 0; i<_host->audio_driver_get_channel_count(true) / 2; i++) {
			std::stringstream strm;
			strm << (i==attributeValues[0]?"*":"") << "Stereo Channel " << (i*2) << " / " << i*2+1;
			os->write((const char*)strm.str().c_str());
		}
		os->write("\0");
	}
	virtual void add_input(const char *name) {}
	virtual void delete_input(const char *name) {}
	virtual void rename_input(const char *oldname, const char *newname) {}
	virtual void input(float **samples, int size, float amp) {}
	virtual void midi_control_change(int ctrl, int channel, int value) {}
	virtual bool handle_input(int index, int amp, int pan) { return false; }
	virtual void set_stream_source(const char* resource) {}
	virtual const char* get_stream_source() { return 0; }
};

zzub::plugin* input_plugin_info::create_plugin() const { 
	return new input_plugin();
}

void input_plugincollection::initialize(zzub::pluginfactory *factory) {
	factory->register_info(&_info);
}

input_plugincollection the_input_plugincollection;

plugincollection *get_input_plugincollection(audiodriver *ad) {
	return &the_input_plugincollection;
}

} // namespace zzub
