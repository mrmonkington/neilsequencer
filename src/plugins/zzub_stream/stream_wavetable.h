#pragma once

struct stream_wavetable : stream_plugin {
	int index, level;
	bool triggered;
	unsigned int currentPosition;

	stream_wavetable();
	virtual ~stream_wavetable();

	virtual void init(zzub::archive* pi);
	virtual void save(zzub::archive*);
	virtual void process_events();
	virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);
	virtual void command(int);
	virtual void stop();
	virtual void get_sub_menu(int, zzub::outstream*);

	bool open();
	void close();

};

struct stream_machine_info_wavetable : stream_machine_info {
	stream_machine_info_wavetable();
	virtual zzub::plugin* create_plugin() const { return new stream_wavetable(); }
	virtual bool store_info(zzub::archive *data) const { return false; }
};

