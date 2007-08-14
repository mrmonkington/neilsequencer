#pragma once

#include <sndfile.h>

struct stream_wav : stream_plugin {
	std::string fileName;
	SNDFILE *sf;
	SF_INFO sfinfo;
	bool loaded;
	bool triggered;
	unsigned int currentPosition;

	stream_wav();
	virtual ~stream_wav();

	virtual void init(zzub::archive* pi);
	virtual void save(zzub::archive*);
	virtual void process_events();
	virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);
	virtual void command(int);
	virtual void stop();

	bool open();
	void close();

};

struct stream_machine_info_wav : stream_machine_info {
	stream_machine_info_wav();
	virtual zzub::plugin* create_plugin() const { return new stream_wav(); }
	virtual bool store_info(zzub::archive *data) const { return false; }
};

