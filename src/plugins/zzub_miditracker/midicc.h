#pragma once

namespace midicc {

#pragma pack(1)										// Place to retrieve parameters	

struct gvals {
	unsigned char smooth;
	unsigned char auto_learn;
};

struct tvals {
	unsigned char channel;
	unsigned char cc;
	unsigned char value;
};

#pragma pack()

struct midicc : public zzub::plugin {
	enum {
		max_tracks = 16,
	};

	gvals gval;
	tvals tval[max_tracks];
	int num_tracks;

	int updatecounter;
	
	int auto_learn;
	int smooth;
	
	struct {
		int channel;
		int cc;
		int prevvalue;
		int updated;
	} track[max_tracks];

	midicc();
	virtual ~midicc() { }

	virtual void init(zzub::archive* pi);
	virtual void load(zzub::archive*);
	virtual void save(zzub::archive*);
	virtual void process_events();
	virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);
	virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) { return false; }
	virtual const char * describe_value(int param, int value); 
	virtual void get_sub_menu(int, zzub::outstream*);
	virtual void set_track_count(int i);
	virtual void command(int);
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
	virtual void midi_control_change(int, int, int);
	virtual bool handle_input(int, int, int) { return false; }
	virtual void process_midi_events(zzub::midi_message* pin, int nummessages) {}
	virtual void get_midi_output_names(zzub::outstream *pout) {}
	virtual void set_stream_source(const char* resource) {}
	virtual const char* get_stream_source() { return 0; }

};

struct midicc_info : zzub::info {
	midicc_info() {
		this->flags = zzub::plugin_flag_has_midi_output;
		this->name = "zzub midicc";
		this->short_name = "midicc";
		this->author = "Lauri Koponen <ld0d@iki.fi>";
		this->uri = "@zzub.org/midicc;1";
		//this->commands = "/MIDI Device";
		this->min_tracks = 8;
		this->max_tracks = 16;

		add_global_parameter()
			.set_switch()
			.set_value_default(zzub::switch_value_off)
			.set_name("Smooth")
			.set_description("Smooth changes")
			.set_state_flag();

		add_global_parameter()
			.set_switch()
			.set_value_default(zzub::switch_value_off)
			.set_name("Auto learn")
			.set_description("Auto learn controllers")
			.set_state_flag();

		add_track_parameter()
			.set_byte()
			.set_name("Channel")
			.set_description("Midi channel")
			.set_value_min(1)
			.set_value_max(16)
			.set_value_none(0xff)
			.set_value_default(1)
			.set_state_flag();

		add_track_parameter()
			.set_byte()
			.set_name("CC")
			.set_description("Controller")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_value_default(0)
			.set_state_flag();

		add_track_parameter()
			.set_byte()
			.set_name("Value")
			.set_description("Controller value")
			.set_value_min(0)
			.set_value_max(127)
			.set_value_none(0xff)
			.set_value_default(0)
			.set_state_flag();
	}

	virtual zzub::plugin* create_plugin() const { return new midicc(); }
	virtual bool store_info(zzub::archive *data) const { return false; }
};

extern midicc_info _midicc_info;

}
