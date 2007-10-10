#include "main.h"

/***

	streaming of sampledata in wavetable

	handles extended buzz wavetable - supports 16/24/f32/s32 bits buffer types

***/

using namespace std;

std::string stringFromInt(int i, int len, char fillChar) {
	char pc[16];
	sprintf(pc, "%i", i);
	std::string s=pc;
	while (s.length()<(size_t)len)
		s=fillChar+s;

	return s;
}

stream_machine_info_wavetable stream_info_wavetable;

stream_machine_info_wavetable::stream_machine_info_wavetable() {
	this->name = "zzub Stream - Wavetable (raw)";
	this->short_name = "WavetableStream";
	this->author = "Andy Werk";
	this->uri = "@zzub.org/stream/wavetable;1";
	this->commands = "/Select Wave";
}

/***

	stream_wavetable

***/

stream_wavetable::stream_wavetable() {
	triggered = false;
	currentPosition = 0;
}

stream_wavetable::~stream_wavetable() {
}

void stream_wavetable::init(zzub::archive * const pi) {
	// the format of initialization instreams is defined for stream plugins
	if (!pi) return ;
	zzub::instream* strm = pi->get_instream("");
	if (!strm) return ;

	std::string fileName;
	if (!strm->read(fileName)) return ;

	this->index = atoi(fileName.c_str());
	this->level = 0;

	this->triggered = false;
	this->currentPosition = 0;
}

void stream_wavetable::save(zzub::archive* po) {
	zzub::outstream* strm = po->get_outstream("");
	strm->write(stringFromInt(index, 0, ' ').c_str());
}

void stream_wavetable::process_events() {
	if (gval.offset != 0xFFFFFFFF) {
		unsigned int offset = get_offset();

		currentPosition = offset;
		triggered = true;
	}
}

inline float sample_scale(zzub::wave_buffer_type format, void* buffer) {
	unsigned int i;
	switch (format) {
		case zzub::wave_buffer_type_si16:
			return static_cast<float>(*(short*)buffer) / 0x7fff;
		case zzub::wave_buffer_type_si24:
			i = (*(unsigned int*)buffer) & 0x00ffffff;
			if (i & 0x00800000) i = i | 0xFF000000;
			return static_cast<float>((int)i) / 0x007fffff;
		case zzub::wave_buffer_type_si32:
			return static_cast<float>(*(int*)buffer) / 0x7fffffff;
		case zzub::wave_buffer_type_f32:
			return *(float*)buffer;
		default:
			return 0;
	}
}

bool stream_wavetable::process_stereo(float **pin, float **pout, int numsamples, int mode) {
	if (mode == zzub::process_mode_read) return false;
	if (mode == zzub::process_mode_no_io) return false;

	if (!triggered) return false;

	const zzub::wave_info* wave = _host->get_wave(index);
	if (!wave) {
		return false;
	}

	zzub::wave_level* l = wave->get_level(level);
	if (!l) {
		return false;
	}
    
	int maxread = numsamples;
	if (currentPosition + maxread > wave->get_sample_count(level)) 
		maxread = wave->get_sample_count(level) - currentPosition;
	
	if (maxread<=0) {
		return false;
	}

	float amp = wave->volume;

	char* sample_ptrc = (char*)wave->get_sample_ptr(level);
	int bytes_per_sample = wave->get_bytes_per_sample(level);
	int channels = wave->get_stereo()?2:1;
	zzub::wave_buffer_type format = wave->get_wave_format(level);

	sample_ptrc += (bytes_per_sample * channels) * currentPosition;

	for (int i = 0; i<maxread; i++) {
		pout[0][i] = sample_scale(format, sample_ptrc) * amp;
		sample_ptrc += bytes_per_sample;

		if (channels == 1) {
			pout[1][i] = pout[0][i]; 
		} else {
			pout[1][i] = sample_scale(format, sample_ptrc) * amp;
			sample_ptrc += bytes_per_sample;
		}
	}
	currentPosition += maxread;

	return true;
}

void stream_wavetable::command(int index) {
	if (index >=256 && index<=512) {
		int waveIndex = index-255;

		const zzub::wave_info* wave = _host->get_wave(waveIndex);
		if (!wave) return ;

		zzub::wave_level* level = wave->get_level(0);
		if (!level) return ;
        
		this->triggered = false;
		this->index = waveIndex;
		this->level = 0;
		this->currentPosition = 0;
		char pc[256];
	}
}

void stream_wavetable::get_sub_menu(int index, zzub::outstream* outs) {
	// print out all waves in the wavetable
	switch (index) {
		case 0:
			for (int i = 0; i<0xC8; i++) {
				// TODO: list only waves where samples>0
				const zzub::wave_info* wave = _host->get_wave(i+1);
				string name = "Wave " + stringFromInt(i+1, 2, ' ') + (string)": " + wave->name;
				outs->write(name.c_str());
			}
			break;
	}
}


void stream_wavetable::stop() {
	triggered = false;
}

bool stream_wavetable::open() {
	return false;
}

void stream_wavetable::close() {
}
