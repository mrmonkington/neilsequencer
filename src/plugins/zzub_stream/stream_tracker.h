#pragma once

/***

	Stream tracker plugin

***/

#pragma pack(1)										// Place to retrieve parameters	

struct trackergvals {
};

struct trackertvals {
	unsigned char note;
	unsigned char wave;
	unsigned char volume;
	unsigned char effect1;
	unsigned char effect1_value;
	unsigned char effect2;
	unsigned char effect2_value;
};

#pragma pack()

struct stream_tracker_plugin;

struct streamtrack {

	zzub::plugin* stream;
	stream_resampler* resampler;

	int note;
	int wave;
	int last_wave;
	int last_level;
	float amp;
	
	trackertvals* tval;
	zzub::host* _host;
	stream_tracker_plugin* _plugin;

	streamtrack();
	void init(stream_tracker_plugin* plugin, int index);
	void process_events();
	bool process_stereo(float **pin, float **pout, int numsamples, int mode);
	virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) { return false; }
	void stop();
	void destroy();

};

struct stream_tracker_plugin : zzub::plugin {

	enum {
		max_tracks = 16,
	};

	int num_tracks;
	streamtrack tracks[max_tracks];
	trackertvals tval[max_tracks];
	trackergvals gval;

	stream_tracker_plugin();

	virtual void init(zzub::archive* pi);
	virtual void load(zzub::archive*);
	virtual void save(zzub::archive*);
	virtual void set_track_count(int i);
	virtual void process_events();
	virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);
	virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) { return false; }
	virtual void command(int);
	virtual void stop();
	virtual void destroy();

	// ::zzub::plugin methods
	virtual void process_controller_events() {}
	virtual const char * describe_value(int param, int value) { return 0; }
	virtual void attributes_changed() {}
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

};

struct stream_tracker_machine_info : zzub::info {
	stream_tracker_machine_info();
	virtual zzub::plugin* create_plugin() const { return new stream_tracker_plugin(); }
	virtual bool store_info(zzub::archive *data) const { return false; }
};
