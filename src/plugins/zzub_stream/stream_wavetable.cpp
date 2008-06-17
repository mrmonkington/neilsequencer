#include "main.h"
#include <iostream>
#include <stdio.h>
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
	this->name = "Wavetable Stream";
	this->short_name = "WavetableStream";
	this->author = "Andy Werk";
	this->uri = "@zzub.org/stream/wavetable;1";
	this->commands = "/Select Wave";
}

/***

	stream_wavetable

***/

stream_wavetable::stream_wavetable() {
	currentPosition = 0;
	resampler = 0;
}

stream_wavetable::~stream_wavetable() {
	if (resampler) delete resampler;
}

void stream_wavetable::init(zzub::archive * const pi) {
	this->play_wave = 0;
	this->play_level = 0;

	this->currentPosition = 0;
	this->lastCurrentPosition = 0;
}

void stream_wavetable::load(zzub::archive * const pi) {
}

void stream_wavetable::save(zzub::archive* po) {
}

void stream_wavetable::set_stream_source(const char* resource) {
	play_wave = atoi(resource);
	play_level = 0;

	currentPosition = 0;
	lastCurrentPosition = 0;

	if (resampler) delete resampler;
	resampler = new stream_resampler(this);

	const zzub::wave_level* level = _host->get_wave_level(play_wave, play_level);
	//zzub_wave_t* wave = _host->get_wave(index);
	if (level) {
		//wave->get_level(level);
		resampler->stream_sample_rate = level->samples_per_second;
	}
}

const char* stream_wavetable::get_stream_source() {
	static char src[32];
	sprintf(src, "%i", play_wave);
    return src;
}


void stream_wavetable::attributes_changed() {
}

void stream_wavetable::process_events() {
	if (!resampler) return ;

	lastCurrentPosition = currentPosition;

	bool triggered = false;

	if (gval.note != zzub::note_value_none) {
		resampler->note = buzz_to_midi_note(gval.note);
		triggered = true;
		currentPosition = 0;
	}

	if (gval.offset != 0xFFFFFFFF) {
		currentPosition = get_offset();

		triggered = true;
	}

	if (aval.offsetfromsong) {
		const zzub::wave_info* wave = _host->get_wave(play_wave);
		if (wave) {
			const zzub::wave_level* l = _host->get_wave_level(play_wave, play_level);
			if (l) {
				bool looping = wave->flags&zzub::wave_flag_loop?true:false;
				unsigned int sample_count = l->sample_count;
				double samplespertick = (double)_master_info->samples_per_tick + (double)_master_info->samples_per_tick_frac;
				double samplepos = (double)_host->get_play_position() * samplespertick;
				currentPosition = (int)(samplepos+0.5f);
				triggered = (_host->get_state_flags() & zzub::state_flag_playing)?true:false;
			}
		}
	}

	if (triggered)
		resampler->set_stream_pos(currentPosition);

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

bool stream_wavetable::generate_samples(float** pout, int numsamples) {
	const zzub::wave_info* wave = _host->get_wave(play_wave);
	if (!wave) return false;

	const zzub::wave_level* level = _host->get_wave_level(play_wave, play_level);
	if (!level ) return false;
    
	bool looping = wave->flags&zzub::wave_flag_loop?true:false;
	unsigned int sample_count = level->sample_count;

	int maxread = numsamples;
	if (!looping && currentPosition + maxread > sample_count) 
		maxread = sample_count - currentPosition;
	
	if (maxread<=0) {
		return false;
	}

	float amp = wave->volume;

	char* sample_ptrc = (char*)level->samples;
	int bytes_per_sample = level->get_bytes_per_sample();
	int channels = (wave->flags & zzub::wave_flag_stereo)?2:1;
	zzub::wave_buffer_type format = (zzub::wave_buffer_type)level->format;

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

		if (looping && currentPosition >= level->loop_end - 1) {
			currentPosition = level->loop_start;
			sample_ptrc = (char*)level->samples;
			sample_ptrc += (bytes_per_sample * channels) * currentPosition;
		} else
			currentPosition++;
	}

	return true;
}

int stream_wavetable::get_target_samplerate() {
	return _master_info->samples_per_second;
}

bool stream_wavetable::process_stereo(float **pin, float **pout, int numsamples, int mode) {
	if (mode == zzub::process_mode_read) return false;
	if (mode == zzub::process_mode_no_io) return false;

	if (!resampler || !resampler->playing) return false;

	return resampler->process_stereo(pout, numsamples);
}

void stream_wavetable::command(int index) {
	std::cout << "command " << index << std::endl;
	if (index >=256 && index<=512) {
		index -= 255;
		int selindex = 0;
		for (int i = 0; i < 0xC8; ++i) {
			const zzub::wave_info* wave = _host->get_wave(i+1);
			const zzub::wave_level* level = _host->get_wave_level(i+1, 0);
			if (level->sample_count > 0)
			{
				selindex++;
				if (selindex == index) {
					std::cout << _host->get_wave_name(i + 1) << std::endl;
					if (!level) return ;
					
					resampler->playing = false;
					this->play_wave = i+1;
					this->play_level = 0;
					this->currentPosition = 0;
					char pc[256];
				}
			}
		}
	}
}

void stream_wavetable::get_sub_menu(int index, zzub::outstream* outs) {
	// print out all waves in the wavetable
	switch (index) {
		case 0:
			for (int i = 0; i<0xC8; i++) {
				const zzub::wave_info* wave = _host->get_wave(i+1);
				const zzub::wave_level* level = _host->get_wave_level(i+1, 0);
				if (level->sample_count > 0)
				{
					string name = "Wave " + stringFromInt(i+1, 2, ' ') + (string)": " + _host->get_wave_name(i + 1);
					outs->write(name.c_str());
				}
			}
			break;
	}
}


void stream_wavetable::stop() {
	if (resampler) resampler->playing = false;
}

bool stream_wavetable::open() {
	return false;
}

void stream_wavetable::close() {
}
