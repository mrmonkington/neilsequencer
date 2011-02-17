#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include "FileReader.h"
#include "module.h"
#include "it.h"
#include "module_it.h"

namespace modfile {
namespace it {

bool module_it::open(std::string fileName) {

	_ITHEADER header;

	std::vector<long> instrumentOffsets;
	std::vector<long> sampleHeaderOffsets;
	std::vector<long> patternOffsets;
	//std::vector<it_pattern> patterns;
	int numChannels;

	if (!reader.open(fileName.c_str())) return false;

	reader.readBytes(&header, sizeof(_ITHEADER));
	
	// skip orders
	char c;
	for (int i = 0; i<header.OrdNum; i++) {
		reader.read(&c);
	}

	unsigned int j;
	for (int i = 0; i<header.InsNum; i++) {
		reader.read(&j);
		instrumentOffsets.push_back(j);
	}

	for (int i = 0; i<header.SmpNum; i++) {
		reader.read(&j);
		sampleHeaderOffsets.push_back(j);
	}


	for (int i = 0; i<header.PatNum; i++) {
		reader.read(&j);
		patternOffsets.push_back(j);
	}

	_ITINSTRUMENT_NEW instrumentHeader;
	instrumentSamples.resize(instrumentOffsets.size());
	sampleOffsets.resize(instrumentOffsets.size());
	for (size_t i = 0; i<instrumentOffsets.size(); i++) {
		reader.seek(instrumentOffsets[i]);
		reader.readBytes(&instrumentHeader, sizeof(_ITINSTRUMENT_NEW));

		instrument_info instrument;
		instrument.name = instrumentHeader.instrumentName;
		
		std::vector<int> indices;
		getInstrumentSamples(&instrumentHeader, indices);

		for (size_t j = 0; j < indices.size(); j++) {
			reader.seek(sampleHeaderOffsets[indices[j]]);

			_ITSAMPLE sampleHeader;
			reader.readBytes(&sampleHeader, sizeof(_ITSAMPLE));

			sample_info sample;
			sample.amp = (float)sampleHeader.Vol / 255.0f;
			sample.bits_per_sample = (sampleHeader.Flags & sample_flag_16bit) != 0 ? 16 : 8;
			sample.is_bidir = (sampleHeader.Flags & sample_flag_bidir_loop) != 0;
			sample.is_float = false;
			sample.is_loop = sampleHeader.LoopEnd>0;
			sample.is_signed = (header.Cwtv >= 0x0202);
			sample.is_stereo = (sampleHeader.Flags & sample_flag_stereo) != 0;
			sample.loop_end = sampleHeader.LoopEnd;
			sample.loop_start = sampleHeader.LoopBegin;
			sample.name = sampleHeader.sampleName;
			sample.note_c4_rel = sampleHeader.C5Speed;
			sample.samples = sampleHeader.Length;
			sample.user_flags = (sampleHeader.Flags & sample_flag_compressed) != 0 ? IT_SAMPLE_COMPRESS : 0;
			sample.user_flags |= (sampleHeader.Cvt & sample_convert_delta) != 0 ? IT_SAMPLE_DELTA : 0;
			sample.samples_per_second = 22050;

			sampleOffsets[i].push_back(sampleHeader.SamplePointer);

			// sample.bytes ;	// TODO: calc this genericly
			instrument.samples.push_back(sample);
		}

		instruments.push_back(instrument);
	}

	numChannels = 64;

	int maxChannel = 0;
	for (size_t i = 0; i<patternOffsets.size(); i++) {
		reader.seek(patternOffsets[i]);
		unsigned short size, rows;
		reader.read(&size);
		reader.read(&rows);

		unsigned int wtf;
		reader.read(&wtf);

		pattern_type pattern;
		pattern.rows = rows;
		pattern.tracks = numChannels;
		pattern.pattern_matrix.resize(numChannels);

		for (int chn = 0; chn<numChannels; chn++) {
			pattern.pattern_matrix[chn].resize(rows);
		}
		std::vector<unsigned char> prevpack(numChannels);
		std::vector<unsigned char> prevnote(numChannels);
		std::vector<unsigned char> prevsample(numChannels);
		std::vector<unsigned char> prevvolume(numChannels);
		std::vector<unsigned char> preveffect(numChannels);
		std::vector<unsigned char> preveffect_value(numChannels);

		for (int row = 0; row<rows; row++) {
			for (int chn = 0; chn<numChannels; chn++) {
			
				unsigned char note = 0, sample = 0, volume = 255, effect = 0, effect_value = 0;
				unsigned char chnpack;
				reader.read(&chnpack);
				if (chnpack == 0) break;
				int track = (chnpack-1) & 63;
				if (track>maxChannel)
					maxChannel = track;
				unsigned char pack;
				if (chnpack & 128) 
					reader.read(&pack); else
					pack = prevpack[track];
				prevpack[track] = pack;

				if (pack & 1) {
					reader.read(&note);
					note++;
				}
				if (pack & 2) reader.read(&sample);
				if (pack & 4) reader.read(&volume);
				if (pack & 8) {
					reader.read(&effect);
					reader.read(&effect_value);
				}

				if (pack & 16) note = prevnote[track];
				if (pack & 32) sample = prevsample[track];
				if (pack & 64) volume = prevvolume[track];
				if (pack & 128) {
					effect = preveffect[track];
					effect_value = preveffect_value[track];
				}

				if (pack & 1) prevnote[track] = note;
				if (pack & 2) prevsample[track] = sample;
				if (pack & 4) prevvolume[track] = volume;
				if (pack & 8) {
					preveffect[track] = effect;
					preveffect_value[track] = effect_value;
				}

				// TODO: split volume byte into volume and panning

				pattern.note(track, row) = note;
				pattern.sample(track, row) = sample;
				pattern.volume(track, row) = volume;
				pattern.effect(track, row) = effect;
				pattern.effect_value(track, row) = effect_value;
			}
		}
		patterns.push_back(pattern);
	}

	return true;
}

void module_it::getInstrumentSamples(_ITINSTRUMENT_NEW* instrument, std::vector<int>& sampleIndices) {
	std::map<int, int> indices;
	for (int i = 0; i < 120; i++) {
		int sampleIndex = instrument->NodeSampleKeyboardTable[i * 2 + 1];
	    
		if (sampleIndex == 0) continue;
		sampleIndex--;
		if (std::find(sampleIndices.begin(), sampleIndices.end(), sampleIndex) == sampleIndices.end()) {
			sampleIndices.push_back(sampleIndex);
		}
	}
}

void module_it::close() {
	reader.close();
}

std::string module_it::name() {
	return "(unsupported)";
}


extern "C" {

int itsex_decompress8 (FILE *, void *, int, int);
int itsex_decompress16 (FILE *, void *, int, int);
	
};

unsigned long module_it::sample_read(int instrument, int sample, void* buffer, unsigned long bytes) {
	
	reader.seek(sampleOffsets[instrument][sample]);

	bool compressed = (instruments[instrument].samples[sample].user_flags & IT_SAMPLE_DELTA) != 0;
	int bits = instruments[instrument].samples[sample].bits_per_sample;

	bool delta = (instruments[instrument].samples[sample].user_flags & IT_SAMPLE_DELTA) != 0;
	if (compressed) {
		switch (bits) {
			case 8:
				return itsex_decompress8(reader.f, buffer, bytes, delta);
			case 16:
				return itsex_decompress16(reader.f, buffer, bytes / 2, delta);
			default:
				return 0;
		}
	} else
		return reader.readBytes(buffer, bytes);
}

}
}
