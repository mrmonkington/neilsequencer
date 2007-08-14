/*
kibibu Green Milk
templated phasor class

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

#include <stdlib.h>
#include "common.h"
#include "lfo_waveshapes.h"

template<int BIT_COUNT> class Phasor
{
public:
	unsigned int phase;	// phase inside the oversampled wavetable
	unsigned int inc;
	double frequency, fPhaseInc;

	int waveform;

	Phasor()
	{
		randomisePhase();
		waveform = 1;
	}

	void increment();
	void increment(int samples);

	int getSampleOffset();

	void setFrequency(double freq, double one_over_sample_rate);

	void setPhase(float phs);

	void randomisePhase();
};

template<int BIT_COUNT> class LFOPhasor : public Phasor<BIT_COUNT>
{
public:
	Phasor<LfoWavebank::table_bits> lfo;

	float * lfoWave;
	float LFO_depth;

	void increment();

	LFOPhasor();

private:
	int timeToLFO;

	int lfo_inc;

};


// LFO phaser

template<int BIT_COUNT> LFOPhasor<BIT_COUNT>::LFOPhasor() {
		Phasor<BIT_COUNT>::randomisePhase();
		Phasor<BIT_COUNT>::waveform = 0;
		lfoWave = LfoWavebank::getBank(0);
		timeToLFO = 0;
	}

template<int BIT_COUNT> void Phasor<BIT_COUNT>::increment()
{
	phase = (phase + inc);
}

template<int BIT_COUNT> void Phasor<BIT_COUNT>::increment(int samples)
{
	phase = (phase + (samples * inc));
}


template<int BIT_COUNT> int Phasor<BIT_COUNT>::getSampleOffset()
{
	// let it wrap to the full 32 bits
	return phase >> (32 - BIT_COUNT);
}

template<int BIT_COUNT> void Phasor<BIT_COUNT>::setPhase(float phs)
{
	phase = int(phs * (1 << (32 - BIT_COUNT)));
}

// expects SAMPLES/pMasterInfo->samplesPerSecond as second arg
template<int BIT_COUNT> void Phasor<BIT_COUNT>::setFrequency(double freq, double one_over_sample_rate)
{
	frequency = freq;
	fPhaseInc = (freq * one_over_sample_rate * 4294967296.0f);
	inc = (int)(fPhaseInc);
}

template<int BIT_COUNT> void Phasor<BIT_COUNT>::randomisePhase()
{
	// This is a bit shady - RAND_MAX is little (0x7fff)
	// so we want to shift left to get the full range
	phase = (rand() << (32 - 15));
}


template<int BIT_COUNT> void LFOPhasor<BIT_COUNT>::increment()
{
	if(!timeToLFO)
	{
		timeToLFO = UPDATE_FREQUENCY;
		unsigned int offset = lfo.getSampleOffset();
		lfo_inc = Phasor<BIT_COUNT>::inc + int((LFO_depth * float(Phasor<BIT_COUNT>::inc) * (0.5f - lfoWave[offset])));
		
		lfo.increment();
	}
	Phasor<BIT_COUNT>::phase += lfo_inc;
	timeToLFO--;
}
