#pragma once

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <zzub/plugin.h>

const char* get_open_filename(const char* fileName, const char* filter);
int buzz_to_midi_note(int note);
int find_level_index(const zzub::wave_info* info, const zzub::wave_level* level);
void add_samples(float *pout, float *pin, int numsamples, float amp);
float lognote(int freq);


#pragma pack(1)										// Place to retrieve parameters	

struct gvals {
	unsigned int offset;
	unsigned int length;
};

#pragma pack()

/***

	Stream plugin base class

***/

struct stream_plugin : zzub::plugin {

	stream_plugin();

	// ::zzub::plugin methods
	virtual const char * describe_value(int param, int value) { return 0; }
	virtual void destroy() { delete this; }
	virtual void stop() {}
	virtual void save(zzub::archive*) {}
	virtual void attributes_changed() {}
	virtual void command(int) {}
	virtual void set_track_count(int i) { }
	virtual void mute_track(int) {}
	virtual bool is_track_muted(int) const { return false; }
	virtual void midi_note(int channel, int note, int velocity) {}
	virtual void event(unsigned int) {}
	virtual const zzub::envelope_info** get_envelope_infos() { return 0; }
	virtual bool play_wave(int, int, float) { return false; }
	virtual void stop_wave() {}
	virtual int get_wave_envelope_play_position(int) { return -1; }
	virtual const char* describe_param(int) { return 0; }
	virtual bool set_instrument(const char*) { return false; }
	virtual void get_sub_menu(int, zzub::outstream*) {}
	virtual void add_input(const char*) {}
	virtual void delete_input(const char*) {}
	virtual void rename_input(const char*, const char*) {}
	virtual void input(float**, int, float) {}
	virtual void midi_control_change(int, int, int) {}
	virtual bool handle_input(int, int, int) { return false; }

	unsigned int get_offset() {
		unsigned short low = gval.offset & 0xFFFF;
		unsigned short high = gval.offset >> 16;
		unsigned int offset;
		if (low == 0xFFFF) 
			offset = high << 16; else
		if (high == 0xFFFF)
			offset = low; else
			offset = gval.offset;
		return offset;
	}

	unsigned int get_length() {
		unsigned short low = gval.length & 0xFFFF;
		unsigned short high = gval.length >> 16;
		unsigned int length;
		if (low == 0xFFFF) 
			length = high << 16; else
		if (high == 0xFFFF)
			length = low; else
			length = gval.length;
		return length;
	}
protected:

	gvals gval;
};

struct stream_machine_info : zzub::info {
	stream_machine_info();
};

#include "stream_wav.h"
#include "stream_mp3.h"
#include "stream_wavetable.h"
#include "resample.h"
#include "stream_player.h"
#include "stream_tracker.h"

extern stream_machine_info_wav stream_info_wav;
extern stream_machine_info_mp3 stream_info_mp3;
extern stream_tracker_machine_info stream_tracker_info;
extern stream_player_machine_info stream_player_info;
extern stream_machine_info_wavetable stream_info_wavetable;

/***

	Stream plugin collection

***/

struct streamplugincollection : zzub::plugincollection {
	virtual void initialize(zzub::pluginfactory *factory) { 
		factory->register_info(&stream_info_wav); 
		factory->register_info(&stream_info_mp3); 
		factory->register_info(&stream_info_wavetable); 
		factory->register_info(&stream_tracker_info); 
		factory->register_info(&stream_player_info); 
	}
	virtual const zzub::info *get_info(const char *uri, zzub::archive *data) { return 0; }
	virtual void destroy() { delete this; }
	virtual const char *get_uri() { return 0; }
	virtual void configure(const char *key, const char *value) {}
};
