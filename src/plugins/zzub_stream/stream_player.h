#pragma once

/***

	Stream player plugin

	This uses raw stream plugins (as defined by wave_level::stream_plugin_url and stream_data_url,
	resamples as necessary and should also sync start/stop with the sequencer position.

***/

#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

#include "def.h"
#include "Downsampler2Flt.h"
#include "fnc.h"
#include "Fixed3232.h"
#include "Int16.h"
#include "Int64.h"
#include "InterpFlt.h"
#include "InterpPack.h"
#include "MipMapFlt.h"
#include "ResamplerFlt.h"
#include "StopWatch.h"

#pragma pack(1)										// Place to retrieve parameters	

struct playergvals {
	unsigned char note;
	unsigned char wave;
	unsigned int offset;
	unsigned int length;
};

#pragma pack()

struct stereo_resampler {
	rspl::InterpPack interp_pack;
	rspl::MipMapFlt mip_map;
	rspl::ResamplerFlt	rspl;

	void init(float* samples, int numsamples);
};

struct stream_player_plugin : zzub::plugin {

	enum {
		crossfade_samples = 20,			// number of samples to buffer extra in resampler (resample crossfade_samples/2 before and after extra)
		
		// TODO: is this a good value? must be large to hold sample data for a B-9 played at 192kHz in 33BPM/1TPB/etc
		max_samples_per_tick = 1024*512,	
	};

	float bufferL[max_samples_per_tick];
	float bufferR[max_samples_per_tick];

	float remainderL[crossfade_samples*2];
	float remainderR[crossfade_samples*2];

	zzub::plugin* stream;
	stereo_resampler resampleL, resampleR;
	bool playing;
	bool has_remainder;
	bool work_remainder;
	int note;
	int wave;
	int last_wave;

	stream_player_plugin();

	virtual void init(zzub::archive* pi);
	virtual void save(zzub::archive*);
	virtual void process_events();
	virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);
	virtual void command(int);
	virtual void stop();
	virtual void destroy();

	// ::zzub::plugin methods
	virtual const char * describe_value(int param, int value) { return 0; }
	virtual void attributes_changed() {}
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

	playergvals gval;
};

struct stream_player_machine_info : zzub::info {
	stream_player_machine_info();
	virtual zzub::plugin* create_plugin() const { return new stream_player_plugin(); }
	virtual bool store_info(zzub::archive *data) const { return false; }
};
