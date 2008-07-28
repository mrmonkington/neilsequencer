#pragma once

struct stream_wavetable : stream_plugin, stream_provider {
	stereo_resampler resample;
	stream_resampler* resampler;

	int play_wave, play_level;
	unsigned int currentPosition;
	unsigned int lastCurrentPosition;

	stream_wavetable();
	virtual ~stream_wavetable();
	
	void reinit_resampler();

	virtual void init(zzub::archive* pi);
	virtual void load(zzub::archive*);
	virtual void save(zzub::archive*);
	virtual void process_events();
	virtual void attributes_changed();
	virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);
	virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) { return false; }
	virtual void command(int);
	virtual void stop();
	virtual void get_sub_menu(int, zzub::outstream*);
	virtual void set_stream_source(const char* resource);
	virtual const char* get_stream_source();

	virtual bool generate_samples(float** buffer, int numsamples);
	virtual int get_target_samplerate();

	bool open();
	void close();

};

struct stream_machine_info_wavetable : stream_machine_info {
	stream_machine_info_wavetable();
	virtual zzub::plugin* create_plugin() const { return new stream_wavetable(); }
	virtual bool store_info(zzub::archive *data) const { return false; }
};


