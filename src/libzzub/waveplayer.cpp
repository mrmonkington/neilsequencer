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

#include "common.h"
#include "waveplayer.h"
#include "dummy.h"

namespace {

	
// assume we're working within 0x8000
int getSampleAt(void* buffer, int waveChannels, int waveFormat, float sample, int interleave) {
	if (!buffer)
		return 0;
	DWORD temp;
	float tempF;
	char* cbuffer;//=(char*)buffer;
	switch (waveFormat) {
		case zzub::wave_buffer_type_si16:
			return ((short*)buffer)[(size_t)sample*waveChannels + interleave];
		case zzub::wave_buffer_type_si24:
			cbuffer=(char*)buffer;	// aligned by 1 byte??
			temp=*(DWORD*)(&cbuffer[(size_t)sample*waveChannels*3 + interleave*3]);
			return (signed short)(temp >> 8) ;//cbuffer[sample*waveChannels*3];
		case zzub::wave_buffer_type_si32:
			cbuffer=(char*)buffer;
			temp=*(DWORD*)(&cbuffer[(size_t)sample*waveChannels*4 + interleave*4]);
			return (signed short)(temp >> 16) ;//cbuffer[sample*waveChannels*3];
		case zzub::wave_buffer_type_f32:
			cbuffer=(char*)buffer;
			tempF=*(float*)(&cbuffer[(size_t)sample*waveChannels*4 + interleave*4]);
			return (int)(tempF * (float)0x7fff) ;//cbuffer[sample*waveChannels*3];
	}
	return 0;
}

}
// TODO: support looping

namespace zzub {

/*! \struct wave_player
	\brief Helper for playing waves on the way table
*/

/***

	WavePlayer

***/

wave_player::wave_player() {
	this->player=0;
	info=0;
	level=-1;
	note=NOTE_C4;
	currentSample=0;
	amp = 1.0f;
}

wave_player::~wave_player() {
	stop();
}

void wave_player::initialize(zzub::player* poo) {
	this->player=poo;
}

void wave_player::play(wave_info_ex* info, size_t level, int note) {
	stop();

	critial.lock();
	this->info=info;
	this->level=level;
	this->currentSample=0.0f;
	this->note=note;
	critial.unlock();
}

void wave_player::stop() {
	critial.lock();

	this->info=0;
	this->level=-1;
	this->currentSample=0;
	critial.unlock();
}

void wave_player::work(float** samples, size_t numSamples, bool stereo) {
	critial.lock();

	if (level==-1 || !info) {
		critial.unlock();
		return;
	}
		

	size_t waveChannels=info->get_stereo()?2:1;
	size_t workChannels=stereo?2:1;
	float waveSamplesPerSec=static_cast<float>(info->get_samples_per_sec(level));
	float mixerSamplesPerSec=static_cast<float>(player->masterInfo.samples_per_second);
	size_t waveSamples=info->get_sample_count(level);
	int waveFormat=info->get_wave_format(level);

	void* waveBuffer=info->get_sample_ptr(level);

	// TODO: take note into account here...
	this->sampleDelta=waveSamplesPerSec/mixerSamplesPerSec;

	for (size_t i=0; i<numSamples; i++) {
		size_t playSample=(size_t)currentSample;
		if (playSample>=waveSamples) {
			level=0;
			info=0;
			currentSample=0;
			break;
		}

		float l,r;
		if (waveChannels==1) {
			r=l=static_cast<float>(getSampleAt(waveBuffer, waveChannels, waveFormat, playSample, 0));
		} else {
			l=static_cast<float>(getSampleAt(waveBuffer, waveChannels, waveFormat, playSample, 0));
			r=static_cast<float>(getSampleAt(waveBuffer, waveChannels, waveFormat, playSample, 1));
		}
        
        // simplest un-buzzify fix:
        r/=0x8000;
        l/=0x8000;
		
		// apply amp
		r *= amp;
		l *= amp;

		if (workChannels==1) {
			samples[0][i]+=(l+r)/2;
		} else {
			samples[0][i]+=l;
			samples[1][i]+=r;
		}

		currentSample+=sampleDelta;
	}

	critial.unlock();

}

};
