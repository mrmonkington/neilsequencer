/*
kibibu Green Milk
DelayLFO class, an LFO with a pre-delay. Uses look up table + lerping
Copyright (C) 2007  Cameron Foale

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#pragma once

#include "phasor.h"

// an LFO that has a number of sample delay at the start
class DelayLFO
{
public:
	int delay;

	const static int lerp_accuracy = (32-LfoWavebank::table_bits);
	
	float * pWaveform;

	void increment(int samples);
	void trigger();
	float currentValue();
	void pause();
	void resume();
	void skipDelay();
	bool isActive();
	void setScale(float scale);
	void setFrequency(float frequency, float one_over_sr);
	void setPhase(float phs);

	void setUseScale(bool use);
	
	DelayLFO();

	

private:
	int timeLeft;
	bool active;
	
	Phasor<32> phasor;

	float currentVal;

	bool use_scale;
	
	inline void updateFrequency();
	
	float scale;
	float frequency;
	float one_over_sample_rate;

	inline float lerp(float v1, float v2, float u);
	
};
