/*
Copyright (C) 2003-2007 Anders Ervik <calvin@countzero.no>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

/*#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
*/
namespace zzub {

struct wave_proxy {
	player* _player;
	int wave;

	wave_proxy(player* _playr, int _wave):_player(_playr), wave(_wave) { }
};

struct wavelevel_proxy {
	player* _player;
	int wave;
	int level;

	wavelevel_proxy(player* _playr, int _wave, int _level)
		:_player(_playr), wave(_wave), level(_level) { }
};

struct wave_info_ex;

struct wave_level_ex : wave_level {
	wavelevel_proxy* proxy;

	wave_level_ex() {
		proxy = 0;
		samples = 0;
		sample_count = 0;
		loop_start = 0;
		loop_end = 0;
		samples_per_second = 0;
		legacy_sample_ptr = 0;
		legacy_sample_count = 0;
		legacy_loop_start = 0;
		legacy_loop_end = 0;
		format = wave_buffer_type_si16;
	}
};

struct wave_info_ex : wave_info {

	std::string fileName;
	std::string name;
	std::vector<envelope_entry> envelopes;
	std::vector<wave_level_ex> levels;
	wave_proxy* proxy;

	wave_info_ex();
	wave_info_ex(const wave_info_ex& w);
	~wave_info_ex();

	int get_levels() const {
		return (int)levels.size();
	}

	wave_level_ex* get_level(int level) {
		if (level < 0 || (size_t)level >= levels.size()) return 0;
		return &levels[level];
	}

	bool get_extended() const {
		return flags&wave_flag_extended?true:false;
	}

	bool get_stereo() const {
		return flags&zzub::wave_flag_stereo?true:false;
	}

	void set_stereo(bool state) {
		unsigned f = ((unsigned)flags)&(0xFFFFFFFF^zzub::wave_flag_stereo);

		if (state)
			flags = f|zzub::wave_flag_stereo; else
			flags = f;
	}

	void* get_sample_ptr(int level, int offset=0) {
		wave_level* l = get_level(level);
		if (!l) return 0;
		offset *= get_bytes_per_sample(level) * (get_stereo()?2:1);
		if (get_extended()) {
			return (char*)&l->samples[4] + offset;
		} else
			return (char*)l->samples + offset;
	}

	int get_bits_per_sample(int level) {
		wave_level* l = get_level(level);
		if (!l) return 0;

		if (!get_extended()) return 16;

		switch (l->samples[0]) {
			case zzub::wave_buffer_type_si16:
				return 16;
			case zzub::wave_buffer_type_si24:
				return 24;
			case zzub::wave_buffer_type_f32:
			case zzub::wave_buffer_type_si32:
				return 32;
			default:
				//std::cerr << "Unknown extended sample format:" << l->samples[0] << std::endl;
				return 16;
		}
	}

	inline int get_bytes_per_sample(int level) {
		return get_bits_per_sample(level) / 8;
	}

	inline unsigned int get_extended_samples(int level, int samples) {
		int channels = get_stereo()?2:1;
		return ((samples-(4/channels)) *2 ) / get_bytes_per_sample(level);
	}

	inline unsigned int get_unextended_samples(int level, int samples) {
		int channels = get_stereo()?2:1;
		return ((samples * get_bytes_per_sample(level)) / 2) + (4/channels);
	}

	unsigned int get_sample_count(int level) {
		wave_level* l = get_level(level);
		if (!l) return 0;
		if (get_extended())
			return get_extended_samples(level, l->sample_count); else
			return l->sample_count;
	}

	unsigned int get_loop_start(int level) {
		wave_level* l = get_level(level);
		if (!l) return 0;
		if (get_extended())
			return get_extended_samples(level, l->loop_start); else
			return l->loop_start;
	}

	unsigned int get_loop_end(int level) {
		wave_level* l = get_level(level);
		if (!l) return 0;
		if (get_extended())
			return get_extended_samples(level, l->loop_end); else
			return l->loop_end;
	}

	void set_loop_start(int level, unsigned int value) {
		wave_level* l = get_level(level);
		if (!l) return ;
		if (get_extended()) {
			l->loop_start = get_unextended_samples(level, value);
		} else {
			l->loop_start = value;
		}
	}

	void set_loop_end(int level, int value) {
		wave_level* l = get_level(level);
		if (!l) return ;
		if (get_extended()) {
			l->loop_end = get_unextended_samples(level, value);
		} else {
			l->loop_end = value;
		}
	}

	wave_buffer_type get_wave_format(int level) {
		wave_level* l = get_level(level);
		if (!l) return wave_buffer_type_si16;
		if (get_extended() && l->sample_count > 0) {
			return (wave_buffer_type)l->samples[0];
		} else
			return wave_buffer_type_si16;
	}
	void clear();

	bool allocate_level(size_t level, size_t samples, zzub::wave_buffer_type waveFormat, bool stereo);
	bool reallocate_level(size_t level, size_t samples);
	void remove_level(size_t level);
	int get_root_note(size_t level);
	size_t get_samples_per_sec(size_t level) ;
	void set_root_note(size_t level, size_t value);
	void set_samples_per_sec(size_t level, size_t value);
	bool create_wave_range(size_t level, size_t fromSample, size_t numSamples, void** sampleData);
	bool silence_wave_range(size_t level, size_t fromSample, size_t numSamples);
	bool remove_wave_range(size_t level, size_t fromSample, size_t numSamples);
	bool stretch_wave_range(size_t level, size_t fromSample, size_t numSamples, size_t newSize);
	bool insert_wave_at(size_t level, size_t atSample, void* sampleData, size_t channels, int waveFormat, size_t numSamples);
	size_t get_level_index(wave_level* level);
	void set_looping(bool state);
	void set_bidir(bool state);
	bool get_looping();
	bool get_bidir();
	void set_extended();
};

struct wave_table {
	std::vector<wave_info_ex*> waves;
	wave_info_ex monitorwave; // for prelistening

	wave_table(void);
	~wave_table(void);
	void clear();
};

};
