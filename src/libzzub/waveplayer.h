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

namespace zzub {

struct wave_info_ex;

struct wave_player {
	zzub::player* player;
	wave_info_ex* info;
	size_t level;
	synchronization::critical_section critial;
	float currentSample;
	float sampleDelta;
	int note;
	float amp;

	wave_player();
	~wave_player();
	void initialize(zzub::player* player);
	void play(wave_info_ex* info, size_t level, int note);
	void stop();
	void work(float** samples, size_t numSamples, bool stereo);
};

}
