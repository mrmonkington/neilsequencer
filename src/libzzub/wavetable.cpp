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

#include <algorithm>
#include "common.h"
#include "tools.h"
#if defined(USE_RUBBERBAND)
#include "rubberband/RubberBandStretcher.h"
#endif

namespace zzub {

/*! \struct envelope_entry
	\brief Envelope properties and points
*/

/*! \struct wave_info_ex
	\brief Extended wave operations.
*/

/*! \struct wave_level
	\brief Wave data description.
*/

/*! \struct wave_table
	\brief Contains a list of waves.
*/

envelope_entry::envelope_entry() {
	attack=0;
	decay=0;
	sustain=0x4000;
	release=0x1000;
	subDivide=10;
	flags=21;
	disabled=true;

	points.push_back(envelope_point());
	points.back().x=0;
	points.back().y=65535;
	points.back().flags=0;

	points.push_back(envelope_point());
	points.back().x=65535;
	points.back().y=0;
	points.back().flags=0;
}


void envelope_entry::clear() {
	points.clear();
}

/***

	wave_info_ex

***/

wave_info_ex::wave_info_ex() {
	flags = 0;
	volume = 0;
	envelopes.push_back(envelope_entry());
}

wave_info_ex::wave_info_ex(const wave_info& w) {
	name = w.name;
	flags = w.flags;
	volume = w.volume;
	envelopes = w.envelopes;
	fileName = w.fileName;
	levels = w.levels;
}


wave_info_ex::~wave_info_ex() {
	clear();
}

void wave_info_ex::clear() {
	volume = 0;
	flags = 0;
	fileName = "";
	name = "";

/*	for (size_t i = 0; i < levels.size(); i++) {
		if (levels[i].samples != 0)
			delete[] levels[i].samples;
		levels[i].sample_count = 0;
	}
*/
	levels.clear();
	envelopes.clear();
	envelopes.push_back(envelope_entry());
}


// reallocate reallocates and truncates or inserts silence
bool wave_info_ex::reallocate_level(size_t level, size_t samples) {

	int samplesIn16Bit = 0;
	int waveChannels = get_stereo()?2:1;
	int oldSamples = get_sample_count(level);
	int oldSamplesIn16Bit = 0;

	if (get_extended()) {
		samplesIn16Bit = get_unextended_samples(level, samples);
		oldSamplesIn16Bit = get_unextended_samples(level, oldSamples);
	} else {
		samplesIn16Bit = samples;
		oldSamplesIn16Bit = oldSamples;
	}

	zzub::wave_level* l = get_level(level);
	if (!l) return false;

	short* pSamples = new short[samplesIn16Bit*waveChannels];
	memset(pSamples, 0, sizeof(short) * samplesIn16Bit * waveChannels);

	if (l->samples) {
		size_t copySize = std::min(oldSamplesIn16Bit, samplesIn16Bit)*waveChannels;
		memcpy(pSamples, l->samples, copySize*sizeof(short));
		delete[] l->samples;
	}

	l->sample_count = samplesIn16Bit;
	l->samples = pSamples;
	if (!get_looping()) {
		l->loop_start = 0;
		l->loop_end = samplesIn16Bit;
	}

	return true;
}


// bitsperSample = 16, 24 or 32 (currently only 16 supported )
// 8 bits are not supported internally and must be converted to 16 bit
bool wave_info_ex::allocate_level(size_t level, size_t numSamples, zzub::wave_buffer_type waveFormat, bool stereo) {
	if (level >= 0xc8) return false;

	if (level >= levels.size()) {
		if (!levels.size()) {
			// initialize wave_info when the first level is allocated
			set_stereo(stereo);
			volume = 1.0f;
		} else
		if (stereo != get_stereo())
			return false;

		levels.resize(level+1);
		for (size_t i = 0; i < levels.size(); ++i) {
			levels[i].wave = this; // make sure they all carry a pointer back
			levels[i].level = i; // and an index
		}
	} else
	if (levels.size() == 1 && stereo != get_stereo()) {
		// toggle stereo mode if there was only one level allocated and we're replacing it
		set_stereo(stereo);
	} else 
	if (stereo != get_stereo())
		return false;

	int waveChannels = stereo?2:1;
	size_t waveBufferSize = 0;
	size_t samplesIn16bit = 0;
	bool allocExtended = false;

	switch (waveFormat)  {
		case zzub::wave_buffer_type_si16:
			waveBufferSize = numSamples*waveChannels*2;
			samplesIn16bit = numSamples;
			break;
		case zzub::wave_buffer_type_si24:
			waveBufferSize = numSamples*waveChannels*3;
			samplesIn16bit = (numSamples*3) / 2;
			allocExtended = true;
			break;
		case zzub::wave_buffer_type_f32:
			waveBufferSize = numSamples*waveChannels*sizeof(float);
			samplesIn16bit = numSamples*2;
			allocExtended = true;
			break;
		case zzub::wave_buffer_type_si32:
			waveBufferSize = numSamples*waveChannels*sizeof(unsigned int);
			samplesIn16bit = numSamples*2;
			allocExtended = true;
			break;
		default:
			// unsupported number of bits
			return false;
	}
	if (allocExtended) {
		waveBufferSize += 8;	// 8 byte offset
		samplesIn16bit += 4/waveChannels; // and 8 bytes of sample data
	}

	zzub::wave_level* l = &levels[level];
	if (l->samples) delete[] l->samples;

	l->sample_count = samplesIn16bit;
	l->samples = (short*)new char[waveBufferSize];
	l->samples_per_second = 44100;
	l->loop_start = 0;
	l->loop_end = samplesIn16bit;
	l->root_note = note_value_c4;

	memset(l->samples, 0, waveBufferSize);

	if (allocExtended) {
		flags |= zzub::wave_flag_extended;	// ensure extended flag is set
		l->samples[0] = waveFormat;
	}

	return true;
}

void wave_info_ex::remove_level(size_t level) {
	wave_level* l = get_level(level);
	if (!l) return ;
	if (l->samples)
		delete[] l->samples;
	levels.erase(levels.begin() + level);

}

int wave_info_ex::get_root_note(size_t level) {
	zzub::wave_level* l = get_level(level);
	if (!l) return 0;
	return l->root_note;
}

size_t wave_info_ex::get_samples_per_sec(size_t level) {
	zzub::wave_level* l = get_level(level);
	if (!l) return 0;
	return l->samples_per_second;
}

void wave_info_ex::set_root_note(size_t level, size_t value) {
	zzub::wave_level* l = get_level(level);
	if (!l) return ;
	levels[level].root_note = value;
}

void wave_info_ex::set_samples_per_sec(size_t level, size_t value) {
	zzub::wave_level* l = get_level(level);
	if (!l) return ;
	levels[level].samples_per_second = value;
}

// these methods are for raw wave management, insertWaveAt promises conversion 
// according to its waveFormat parameter, which makes me want to consider having a target
// waveFormat parameter for createWaveRange as well

bool wave_info_ex::create_wave_range(size_t level, size_t fromSample, size_t numSamples, void** sampleData) {
	if (fromSample<0 || !numSamples) return false;
	zzub::wave_level* l=get_level(level);
	if (!l) return false;

	size_t bytesPerSample=get_bytes_per_sample(level);
	size_t channels=get_stereo()?2:1;
	size_t size=numSamples*bytesPerSample*channels;

	*sampleData=new char[size];

	// find offset in source buffer from where we start copying
	size_t ofs=fromSample*bytesPerSample*channels;
	if (get_extended())
		ofs+=8;
	char* cbuffer=(char*)l->samples;
	void* srcbuf=&cbuffer[ofs];
	memcpy(*sampleData, srcbuf, size);

	return true;
}

bool wave_info_ex::silence_wave_range(size_t level, size_t fromSample, size_t numSamples) {
	return true;
}

bool wave_info_ex::remove_wave_range(size_t level, size_t fromSample, size_t numSamples) {
	size_t size=this->get_sample_count(level);

	void* range2;
	size_t ch2=get_stereo()?2:1;
	int format2=get_wave_format(level);

	size_t range2Len=size - (fromSample+numSamples);
	create_wave_range(level, fromSample+numSamples, range2Len, &range2);

	size_t newSize=size-numSamples;
	reallocate_level(level, newSize);

	insert_wave_at(level, fromSample, range2, ch2, format2, range2Len);

	// adjust end looping point
	size_t loopEnd = get_loop_end(level);
	if (loopEnd > newSize)
		set_loop_end(level, newSize);

	return true;
}

bool wave_info_ex::stretch_wave_range(size_t level, size_t fromSample, size_t numSamples, size_t newSize) {
	size_t size = get_sample_count(level);
	size_t channels = get_stereo()?2:1;
	int format = get_wave_format(level);
	int samplerate = 44100; //get_level(level)->samples_per_second;
	
	
	int newsize = (int)size - (int)numSamples + (int)newSize;
	int lastrangesize = (int)size - (int)fromSample - (int)numSamples;
	
	assert(newsize > 0);
	assert(lastrangesize >= 0);
	assert(fromSample < size);
	assert((fromSample + numSamples) <= size);

	void* oldrange = 0; // part to sample
	void* lastrange = 0; // rest of sample
	
	create_wave_range(level, fromSample, numSamples, &oldrange);
	if (lastrangesize)
		create_wave_range(level, fromSample+numSamples, lastrangesize, &lastrange);

	reallocate_level(level, newsize);

#if defined(USE_RUBBERBAND)
	{
		using namespace RubberBand;
		double ratio = (double)newSize / (double)numSamples;
		RubberBandStretcher::Options options = RubberBandStretcher::OptionProcessOffline 
			| RubberBandStretcher::OptionTransientsCrisp 
			//~ | RubberBandStretcher::OptionTransientsSmooth 
			| RubberBandStretcher::OptionPhaseAdaptive
			| RubberBandStretcher::OptionThreadingAuto 
			| RubberBandStretcher::OptionWindowStandard;
		if ((fromSample == 0) && (numSamples == size)) {
			options |= RubberBandStretcher::OptionStretchElastic;
		} else {
			options |= RubberBandStretcher::OptionStretchPrecise;
		}
		RubberBand::RubberBandStretcher stretcher(samplerate, channels, options, ratio);
		stretcher.setExpectedInputDuration(numSamples);
		stretcher.setMaxProcessSize(1024);
		
		const int BLOCKSIZE = 1024;
		
		float audio[2][BLOCKSIZE];		
		float *buf[] = { &audio[0][0], &audio[1][0] };
		
		int toprocess = (int)numSamples;
		int processed = 0;
		
		// first, study
		while (toprocess > 0) {
			int blocksize = std::min(toprocess, BLOCKSIZE);
			for (int i = 0; i < channels; ++i) {
				CopySamples(oldrange, &audio[i][0], blocksize, format, zzub::wave_buffer_type_f32, channels, 1, processed*channels+i, 0);
			}
			std::cout << "studying " << blocksize << " blocks at @" << processed << " (" << channels << " channels, final = " << (toprocess == blocksize) << ")" << std::endl;
			stretcher.study(buf, blocksize, toprocess == blocksize);
			toprocess -= blocksize;
			processed += blocksize;
		}
		
		toprocess = (int)numSamples;
		processed = 0;
		int written = 0;
		int startpos = (int)fromSample;
		
		void *targetptr = get_sample_ptr(level);
		
		while (true)
		{
			if (stretcher.available() == -1) {
				std::cout << "stretch is done." << std::endl;
				break;
			}
			if (stretcher.getSamplesRequired())
			{
				int blocksize = std::min(toprocess, std::min((int)stretcher.getSamplesRequired(), BLOCKSIZE));
				for (int i = 0; i < channels; ++i) {
					CopySamples(oldrange, &audio[i][0], blocksize, format, zzub::wave_buffer_type_f32, channels, 1, processed*channels+i, 0);
				}
				std::cout << "processing " << blocksize << " blocks at @" << processed << " (" << channels << " channels, final = " << (toprocess == blocksize) << ")" << std::endl;
				stretcher.process(buf, blocksize, toprocess == blocksize);
				toprocess -= blocksize;
				processed += blocksize;				
			}
			while (stretcher.available() > 0)
			{
				int blocksize = std::min(stretcher.available(), BLOCKSIZE);
				std::cout << "retrieving " << blocksize << " blocks at @" << written << " (" << channels << " channels, final = " << (stretcher.available() == blocksize) << ")" << std::endl;
				stretcher.retrieve(buf, blocksize);
				for (int i = 0; i < channels; ++i) {
					CopySamples(&audio[i][0], targetptr, blocksize, zzub::wave_buffer_type_f32, format, 1, channels, 0, startpos*channels+i);
				}
				written += blocksize;
				startpos += blocksize;
			}
		}

		std::cout << "old selection size is " << numSamples << " samples, new size is " << newSize << " samples." << std::endl;
		std::cout << "processed " << processed << " samples, retrieved " << written << " samples." << std::endl;
		std::cout << "old wave size was " << size << ", new size is " << newsize << std::endl;
		
	}
#else
	{
		insert_wave_at(level, fromSample, oldrange, channels, format, newSize);
	}	
#endif

	//~ delete[] (char*)oldrange;
	if (lastrangesize)
	{
		insert_wave_at(level, fromSample + newSize, lastrange, channels, format, lastrangesize);
		//~ delete[] (char*)lastrange;
	}

	// adjust end looping point
	int loopEnd = (int)get_loop_end(level);
	if (loopEnd > newsize)
		set_loop_end(level, newsize);
	
	return true;
}

size_t waveFormatToBitSize(int waveFormat) {
	switch (waveFormat) {
		case zzub::wave_buffer_type_si16:
			return 16;
		case zzub::wave_buffer_type_si24:
			return 24;
		case zzub::wave_buffer_type_f32:
		case zzub::wave_buffer_type_si32:
			return 32;
		default:
			return 16;
	}
}

// TODO: make so we can paste in one channel!

bool wave_info_ex::insert_wave_at(size_t level, size_t atSample, void* sampleData, size_t channels, int fromFormat, size_t numSamples) {
	size_t waveChannels=get_stereo()?2:1;

	zzub::wave_level* l=&levels[level];

	void* tempbuffer=0;
	void* srcbuffer=0;

	size_t srcBytesPerSample=(waveFormatToBitSize(fromFormat)/8)*2;

	if (channels==1) {
		if (waveChannels==1) {
			// same number of channels
			srcbuffer=sampleData;
		} else
		if (waveChannels==2) {
			// need to convert from stereo to mono while pasting
			// allocate a temp stereo buffer, and convert the input wave to this
			srcbuffer=tempbuffer=new char[srcBytesPerSample*numSamples*2];
			CopyMonoToStereoEx(srcbuffer, sampleData, numSamples, fromFormat);
		}
	} else 
	if (channels==2) {

		if (waveChannels==1) {
			// need to convert from mono to stereo while pasting
			// allocate a temp mono buffer, and convert the input wave to this
			srcbuffer=tempbuffer=new char[srcBytesPerSample*numSamples];
			CopyStereoToMonoEx(srcbuffer, sampleData, numSamples, fromFormat);
		} else
		if (waveChannels==2) {
			srcbuffer=sampleData;
			// same number of channels
		}
	} else
		return false;

	char* cbuffer=(char*)l->samples;

	size_t bytesPerSample = get_bytes_per_sample(level) * waveChannels;

	size_t ofs=atSample*bytesPerSample;
	if (get_extended())
		ofs+=8;
	void* targetbuffer=&cbuffer[ofs];
	// we have a buffer which has the same number of channels as we need, now convert

	int waveFormat=get_wave_format(level);
	switch (fromFormat) {
		case zzub::wave_buffer_type_si16:
			switch (waveFormat) {
				case zzub::wave_buffer_type_si16:
					// 16 to 16
					memcpy(targetbuffer, srcbuffer, numSamples*waveChannels*sizeof(short));
					break;
				case zzub::wave_buffer_type_si24:
					// 16 to 24
					Copy16To24(targetbuffer, srcbuffer, numSamples*waveChannels);
					break;
				case zzub::wave_buffer_type_f32:
					// 16 to 32 bit floating point
					Copy16ToF32(targetbuffer, srcbuffer, numSamples*waveChannels);
					break;
				case zzub::wave_buffer_type_si32:
					// 16 to 32 bit integer
					Copy16ToS32(targetbuffer, srcbuffer, numSamples*waveChannels);
					break;
			}
			break;
		case zzub::wave_buffer_type_si24:
			switch (waveFormat) {
				case zzub::wave_buffer_type_si16:
					// 24 to 16
					Copy24To16(targetbuffer, srcbuffer, numSamples*waveChannels);
					break;
				case zzub::wave_buffer_type_si24:
					// 24 to 24
					memcpy(targetbuffer, srcbuffer, numSamples*waveChannels*3);
					break;
				case zzub::wave_buffer_type_f32:
					// 24 to 32 bit floating point
					Copy24ToF32(targetbuffer, srcbuffer, numSamples*waveChannels);
					break;
				case zzub::wave_buffer_type_si32:
					// 24 to 32 bit integer
					Copy24ToS32(targetbuffer, srcbuffer, numSamples*waveChannels);
					break;
			}
			break;
		case zzub::wave_buffer_type_si32:
			switch (waveFormat) {
				case zzub::wave_buffer_type_si16:
					// integer 32 to 16
					CopyS32To16(targetbuffer, srcbuffer, numSamples*waveChannels);
					break;
				case zzub::wave_buffer_type_si24:
					// integer 32 to 24
					CopyS32To24(targetbuffer, srcbuffer, numSamples*waveChannels);
					break;
				case zzub::wave_buffer_type_f32:
					// integer 32 to 32 bit floating point
					CopyS32ToF32(targetbuffer, srcbuffer, numSamples*waveChannels);
					break;
				case zzub::wave_buffer_type_si32:
					// integer 32 to 32 bit integer
					memcpy(targetbuffer, srcbuffer, numSamples*waveChannels*4);
					break;
			}
			break;
		case zzub::wave_buffer_type_f32:
			switch (waveFormat) {
				case zzub::wave_buffer_type_si16:
					// float 32 to 16
					CopyF32To16(targetbuffer, srcbuffer, numSamples*waveChannels);
					break;
				case zzub::wave_buffer_type_si24:
					// float 32 to 24
					CopyF32To24(targetbuffer, srcbuffer, numSamples*waveChannels);
					break;
				case zzub::wave_buffer_type_f32:
					// float 32 to 32 bit floating point
					memcpy(targetbuffer, srcbuffer, numSamples*waveChannels*4);
					break;
				case zzub::wave_buffer_type_si32:
					// float 32 to 32 bit integer
					CopyF32ToS32(targetbuffer, srcbuffer, numSamples*waveChannels);
					break;
			}
			break;
	}

	// if we allocated a temp buffer, release temporary memory:
	if (tempbuffer)
		delete[] (char*)tempbuffer;

	return true;
}

size_t wave_info_ex::get_level_index(zzub::wave_level* level) {
	for (int i = 0; i < get_levels(); i++) {
		if (&levels[i] == level) return i;
	}
	return -1;
}


void wave_info_ex::set_looping(bool state) {
	if (state) {
		flags |= zzub::wave_flag_loop;
	} else {
		flags ^= flags & zzub::wave_flag_loop;
	}
}

void wave_info_ex::set_bidir(bool state) {
	if (state) {
		flags |= zzub::wave_flag_pingpong;
	} else {
		flags ^= flags & zzub::wave_flag_pingpong;
	}
}


bool wave_info_ex::get_looping() {
	return flags&zzub::wave_flag_loop?true:false;
}

bool wave_info_ex::get_bidir() {
	return flags&zzub::wave_flag_pingpong?true:false;
}


void wave_info_ex::set_extended() {
	if (get_extended()) return ;	// TODO: cant revert

	// TODO: this only clears 8 first shorts, should reallocate sample with + 8 shorts
	flags |= wave_flag_extended;
	for (int i = 0; i < get_levels(); i++) {
		wave_level* l = get_level(i);
		if (!l) continue;
		if (l->sample_count < 8) continue;
		memset(l->samples, 0, 8);
	}
}



/***

	WaveTable

***/

wave_table::wave_table(void) {
//	waves.resize(0xc8);	// max 200 waves
}

wave_table::~wave_table(void) {
	clear();
	waves.clear();
}

void wave_table::clear() {
	for (size_t i = 0; i < waves.size(); i++) {
		waves[i]->clear();
	}
}


};
