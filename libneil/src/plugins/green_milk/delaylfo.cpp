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

#include "delaylfo.h"
#include "green_milk.h"

#include <assert.h>

DelayLFO::DelayLFO()
{
	delay = 0;
	timeLeft = 0;
	pWaveform = LfoWavebank::sine;

	use_scale = false;
	phasor.phase = 0;
	setScale(1.0f);
}

bool DelayLFO::isActive()
{
	return active;
}

void DelayLFO::setPhase(float phs)
{
	phasor.setPhase(phs);
}

void DelayLFO::updateFrequency()
{
	if(use_scale)
	{
		phasor.setFrequency(frequency/scale, one_over_sample_rate);
	} else {
		phasor.setFrequency(frequency, one_over_sample_rate);
	}
}


void DelayLFO::setUseScale(bool use)
{
	use_scale = use;
	
	// and update
	updateFrequency();
	
}

void DelayLFO::setScale(float s)
{
	scale = std::max(s, 0.01f);
	updateFrequency();
}

void DelayLFO::setFrequency(float freq, float one_over_sr)
{
	one_over_sample_rate = one_over_sr;
	frequency = freq;
	updateFrequency();
}

void DelayLFO::skipDelay()
{
	timeLeft = 0;
}

void DelayLFO::trigger()
{
	timeLeft = delay;
	phasor.phase = 0;
	currentVal = 0.0f;
	active = true;
}

void DelayLFO::increment(int samples)
{
	if(active)
	{
		if(timeLeft > samples) {
			timeLeft -= samples;
			return;
		} else if(timeLeft > 0)
		{
			samples -= timeLeft;
			timeLeft = 0;
		}

		phasor.increment(samples);
	}
}

void DelayLFO::pause()
{
	active = false;
}

void DelayLFO::resume()
{
	active = true;
}

float DelayLFO::lerp(float v1, float v2, float u)
{
	float one_m_u = 1.0f - u;
	return (v1 * one_m_u + v2 * u);

}

float DelayLFO::currentValue()
{
	// still counting down
	if(timeLeft) return 0.0f;

	// paused
	if(!active) return currentVal;

	// finished counting down
	// The LFO waveshapes are a little different
	unsigned int point1 = phasor.getSampleOffset();

	static const unsigned int frac_bits = (32 - LfoWavebank::table_bits);
	static const unsigned int frac_mask = (1 << frac_bits) - 1;

	unsigned int frac_part = point1 & frac_mask;
	
	// we want to convert from that to 0 to (2^(32-tb) -1)
	float fractional = float(frac_part) * (1.0f/(frac_mask + 1));
	
	// shift right enough to get only the right bits
	point1 >>= frac_bits;

	// get the second bits
	int point2 = (point1 + 1) & LfoWavebank::bit_mask;

	// get the two points to interpolate
	float val1 = pWaveform[point1];
	float val2 = pWaveform[point2];

	// lerp
	currentVal = lerp(val1, val2, fractional);
	return currentVal;

}
