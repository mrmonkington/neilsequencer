#pragma once
#pragma pack(1)										// Place to retrieve parameters	

struct gvals {
};

struct tvals {
	unsigned char note;
	unsigned char wave;
	unsigned char volume;
	unsigned char effect;
	unsigned char effect_value;
};

#pragma pack()

struct module_plugin : zzub::plugin {
	enum {
		max_tracks = 64
	};

	module_plugin();
	virtual ~module_plugin();

	virtual void init(zzub::archive* pi);
	virtual void process_events();
	virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);
	virtual const char * describe_value(int param, int value); 
	virtual void command(int);

	// ::zzub::plugin methods
	virtual void destroy() { delete this; }
	virtual void stop() {}
	virtual void save(zzub::archive*) {}
	virtual void attributes_changed() {}
	virtual void set_track_count(int i);
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

	tvals tval[max_tracks];
private:
	void importModule(std::string fileName);
};

struct module_plugin_info : zzub::info {
	module_plugin_info();
	virtual zzub::plugin* create_plugin() const { return new module_plugin(); }
	virtual bool store_info(zzub::archive *data) const { return false; }
} _machine_info;

struct moduleplugincollection : zzub::plugincollection {
	virtual void initialize(zzub::pluginfactory *factory) { factory->register_info(&_machine_info); }
	virtual const zzub::info *get_info(const char *uri, zzub::archive *data) { return 0; }
	virtual void destroy() { delete this; }
	virtual const char *get_uri() { return 0; }
	virtual void configure(const char *key, const char *value) {}
};
