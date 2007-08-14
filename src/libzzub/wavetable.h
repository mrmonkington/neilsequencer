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

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace zzub {

struct wave_info_ex;

struct wave_info_ex : wave_info {

	wave_info_ex();
	~wave_info_ex();
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
	bool insert_wave_at(size_t level, size_t atSample, void* sampleData, size_t channels, int waveFormat, size_t numSamples);
	size_t get_level_index(wave_level* level);
	void set_looping(bool state);
	void set_bidir(bool state);
	bool get_looping();
	bool get_bidir();
	void set_extended();
};

struct wave_table {
	std::vector<wave_info_ex> waves;
	wave_info_ex monitorwave; // for prelistening

	wave_table(void);
	~wave_table(void);
	void clear();
};

};
