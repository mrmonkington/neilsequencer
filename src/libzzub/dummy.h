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

#pragma once

namespace zzub {

struct dummy_plugin : public plugin {
	char* globalVals;
	char* trackVals;
	vector<char> data;

	dummy_plugin(const zzub::info* info);

	void process_events();
	virtual void process_controller_events() {}
	//virtual bool process_audio(float *psamples, int numsamples, int const mode);
	virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);
	virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) { return false; }
	virtual void process_midi_events(midi_message* pin, int nummessages) {}
	virtual void get_midi_output_names(outstream *pout) {}
	
	// ::zzub::plugin methods
	virtual void destroy();
	virtual void init(zzub::archive* arc);
	virtual void load(zzub::archive* arc);
	virtual void save(zzub::archive* arc);
	virtual void stop() {}
	virtual void attributes_changed() {}
	virtual void command(int) {}
	virtual void set_track_count(int) {}
	virtual void mute_track(int) {}
	virtual bool is_track_muted(int) const { return false; }
	virtual void midi_note(int, int, int) {}
	virtual void event(unsigned int) {}
	virtual const char* describe_value(int, int) { return 0; }
	virtual const zzub::envelope_info** get_envelope_infos() { return 0; }
	virtual bool play_wave(int, int, float) { return false; }
	virtual void stop_wave() {}
	virtual int get_wave_envelope_play_position(int) { return -1; }
	virtual const char* describe_param(int) { return 0; }
	virtual bool set_instrument(const char*) { return false; }
	virtual void get_sub_menu(int, zzub::outstream*) {}
	virtual void add_input(const char*, zzub::connection_type type) {}
	virtual void delete_input(const char*, zzub::connection_type type) {}
	virtual void rename_input(const char*, const char*) {}
	virtual void input(float**, int, float) {}
	virtual void midi_control_change(int, int, int) {}
	virtual bool handle_input(int, int, int) { return false; }
	virtual void set_stream_source(const char* resource) {}
	virtual const char* get_stream_source() { return 0; }
};

struct dummy_info : zzub::info {
	dummy_info();
	virtual zzub::plugin* create_plugin() const;
	virtual bool store_info(zzub::archive *arc) const;
};

};
