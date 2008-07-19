#pragma once

namespace miditime {

#pragma pack(1)										// Place to retrieve parameters	

struct gvals {
	int dummy;
};

#pragma pack()

struct miditimemachine;


struct miditimemachine : public zzub::plugin {
	enum {
		max_tracks = 16,
	};

	gvals gval;
	int playing;
	int last_play_pos;

	miditimemachine();
	virtual ~miditimemachine() { }

	virtual void init(zzub::archive* pi);
	virtual void load(zzub::archive*);
	virtual void save(zzub::archive*);
	virtual void process_events();
	virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);
	virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) { return false; }
	virtual const char * describe_value(int param, int value); 
	virtual void get_sub_menu(int, zzub::outstream*) {}
	virtual void set_track_count(int i);
	virtual void command(int) {}
	virtual void stop();

	// ::zzub::plugin methods
	virtual void process_controller_events() {}
	virtual void destroy() { delete this; }
	virtual void attributes_changed() {}
	virtual void mute_track(int) {}
	virtual bool is_track_muted(int) const { return false; }
	virtual void midi_note(int channel, int note, int velocity);
	virtual void event(unsigned int) {}
	virtual const zzub::envelope_info** get_envelope_infos() { return 0; }
	virtual bool play_wave(int, int, float) { return false; }
	virtual void stop_wave() {}
	virtual int get_wave_envelope_play_position(int) { return -1; }
	virtual const char* describe_param(int) { return 0; }
	virtual bool set_instrument(const char*) { return false; }
	virtual void add_input(const char*, zzub::connection_type type) {}
	virtual void delete_input(const char*, zzub::connection_type type) {}
	virtual void rename_input(const char*, const char*) {}
	virtual void input(float**, int, float) {}
	virtual void midi_control_change(int, int, int) {}
	virtual bool handle_input(int, int, int) { return false; }
	virtual void process_midi_events(zzub::midi_message* pin, int nummessages) {}
	virtual void get_midi_output_names(zzub::outstream *pout) {}
	virtual void set_stream_source(const char* resource) {}
	virtual const char* get_stream_source() { return 0; }
	virtual void play_pattern(int index) {}
	virtual void configure(const char *key, const char *value) {}

};

struct miditime_info : zzub::info {
	miditime_info() {
		this->flags = zzub::plugin_flag_has_midi_output;
		this->name = "zzub miditime";
		this->short_name = "miditime";
		this->author = "Lauri Koponen <ld0d@iki.fi>";
		this->uri = "@zzub.org/miditime;1";
		this->commands = "";
		this->min_tracks = 0;
		this->max_tracks = 0;
	}
	virtual zzub::plugin* create_plugin() const { return new miditimemachine(); }
	virtual bool store_info(zzub::archive *data) const { return false; }
};

extern miditime_info _miditime_info;

}
