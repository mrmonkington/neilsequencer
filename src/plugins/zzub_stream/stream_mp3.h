#pragma once

#include <mad.h>

#define MAD_FRAMESIZE 8192

struct frame_info {
	unsigned int offset;
	int bitrate;
	unsigned int filepos;
	int samples;
};

struct stream_mp3 : stream_plugin {
	std::string fileName;
	FILE* f;
	int samplerate;
	std::vector<float> outbuffer;
	int nchannels;
	unsigned char framebuf[MAD_FRAMESIZE];
	int framepos;
	bool loaded;
	bool triggered;
	bool changedFile;
	bool outOfSync;
	int currentFrame;
	int currentPosition;
	unsigned int currentSample;
	int seekSkipSamples;
	std::vector<frame_info> frames;

	struct mad_stream stream;
	struct mad_frame frame;
	struct mad_synth synth;

	int bad_last_frame;
	stream_mp3();
	virtual ~stream_mp3();

	virtual void init(zzub::archive* pi);
	virtual void load(zzub::archive*);
	virtual void save(zzub::archive*);

	virtual void process_events();
	virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);
	virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) { return false; }
	virtual void command(int);
	virtual void stop();

	bool open();
	void close();

	bool run_frame();

	void seek(unsigned int pos);
	frame_info* get_frame_at_sample(unsigned int pos, int* index = 0);
	void reset_stream();
	void scan_to_frame(unsigned int pos);
	//
	enum mad_flow zzub_mad_input(struct mad_stream *stream);
	enum mad_flow zzub_mad_header(struct mad_header const *header);
	float zzub_mad_scale(mad_fixed_t sample);
	enum mad_flow zzub_mad_output(struct mad_header const *header, struct mad_pcm *pcm);
	enum mad_flow zzub_mad_error(struct mad_stream *stream, struct mad_frame *frame);
};

struct stream_machine_info_mp3 : stream_machine_info {
	stream_machine_info_mp3();
	virtual zzub::plugin* create_plugin() const { return new stream_mp3(); }
	virtual bool store_info(zzub::archive *data) const { return false; }
};


