#pragma once

#include <sndfile.h>

struct stream_wav : stream_plugin, stream_provider {
	stereo_resampler resample;
	stream_resampler* resampler;

	std::string fileName;
	SNDFILE *sf;
	SF_INFO sfinfo;
	bool loaded;
	unsigned int currentPosition;
	float* buffer;
	size_t buffer_size;

	stream_wav();
	virtual ~stream_wav();

	virtual void init(zzub::archive* pi);
	virtual void load(zzub::archive*);
	virtual void save(zzub::archive*);
	virtual void process_events();
	virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);
	virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) { return false; }
	virtual void command(int);
	virtual void stop();
	virtual void set_stream_source(const char* resource);
	virtual const char* get_stream_source();
	virtual bool play_wave(int wave, int note, float volume, int offset, int length);

	virtual bool generate_samples(float** buffer, int numsamples);
	virtual int get_target_samplerate();

	bool open();
	void close();

};

struct stream_machine_info_wav : stream_machine_info {
	stream_machine_info_wav();
	virtual zzub::plugin* create_plugin() const { return new stream_wav(); }
	virtual bool store_info(zzub::archive *data) const { return false; }
};


