#include <string>
#include <vector>
#include <iostream>
#include "FileReader.h"
#include "module.h"
#include "s3m.h"
#include "module_s3m.h"

namespace modfile {
namespace s3m {

int s3m_to_midi_note(int value) {
	return 12 * (value >> 4) + (value & 0xf) - 1;
}

bool module_s3m::open(std::string fileName) {
	_S3MHEADER header;
	_S3MSAMPLE* samples;
	std::vector<char> orders;
	std::vector<short> instrumentOffsets;
	std::vector<short> patternOffsets;
	unsigned char channelPan[32];

	int numChannels;

	if (!reader.open(fileName.c_str())) return false;

	reader.readBytes(&header, sizeof(_S3MHEADER));

	for (int i=0; i<header.ordNum; i++) {
		unsigned char ord;
		reader.read(&ord);
		orders.push_back(ord);
	}

	// TODO: read padding

	for (int i=0; i<header.insNum; i++) {
		unsigned short ins;
		reader.read(&ins);
		instrumentOffsets.push_back(ins);
	}

	for (int i=0; i<header.patNum; i++) {
		unsigned short pat;
		reader.read(&pat);
		patternOffsets.push_back(pat);
	}

	reader.readBytes(channelPan, sizeof(channelPan));
	numChannels = 0;
	for (int i= 0; i<32; i++) {
		if (header.channelSettings[i] < 128) numChannels = i+1;
	}

	samples=new _S3MSAMPLE[header.insNum];

	for (int i=0; i<header.insNum; i++) {
		int ofs=instrumentOffsets[i]*16;
		reader.seek(ofs);
		reader.readBytes(&samples[i], sizeof(_S3MSAMPLE));

		instrument_info instrument;
		instrument.name = samples[i].samplename;
		sample_info sample;
		sample.name = samples[i].samplename;
		sample.bits_per_sample = 8;
		sample.is_bidir = false;
		sample.is_float = false;
		sample.is_loop = false;
		sample.is_stereo = false;
		sample.is_signed = false;
		sample.samples = samples[i].len;
		sample.loop_start = samples[i].loopbeg;
		sample.loop_end = samples[i].loopend;
		sample.tuning = samples[i].c2spd;
		sample.note_c4_rel = 0;
		sample.samples_per_second = 22050;
		sample.amp = (float)samples[i].vol / 0x80;

		instrument.samples.push_back(sample);
		instruments.push_back(instrument);

		sampleOffsets.push_back(samples[i].seg * 16);
	}

/*
		So to unpack, first read one byte. If it's zero, this row is
		done (64 rows in entire pattern). If nonzero, the channel
		this entry belongs to is in BYTE AND 31. Then if bit 32
		is set, read NOTE and INSTRUMENT (2 bytes). Then if bit
		64 is set read VOLUME (1 byte). Then if bit 128 is set
		read COMMAND and INFO (2 bytes).

Note; hi=oct, lo=note, 255=empty note,
254=key off (used with adlib, with samples stops smp)

*/

	for (int i = 0; i<header.patNum; i++) {
		int ofs = patternOffsets[i]*16;
		reader.seek(ofs);
		unsigned short patternSize;
		reader.read(&patternSize);
		
		pattern_type pattern;
		pattern.rows = 64;
		pattern.tracks = numChannels;

		pattern.pattern_matrix.resize(numChannels);
		for (int chn = 0; chn < numChannels; chn++) {
			pattern.pattern_matrix[chn].resize(64);
		}
		for (int row = 0; row < 64; row++) {
			// chn is just a counter, we read the actual track from file
			for (int chn = 0; chn < numChannels; chn++) {
				unsigned char pack;
				reader.read(&pack);
				if (pack == 0) {
					//row = 64;
					break;//continue;
				}

				int track = pack&31;
				unsigned char note = 0, sample = 0, volume = 255, effect = 0, effect_value = 0;

				if (pack & 32) {
					reader.read(&note);
					reader.read(&sample);
					note = s3m_to_midi_note(note);
				}
				if (pack & 64) reader.read(&volume);
				if (pack & 128) {
					reader.read(&effect);
					reader.read(&effect_value);
				}
				if (track >= numChannels) continue;

				pattern.note(track, row) = note;
				pattern.sample(track, row) = sample;
				pattern.volume(track, row) = volume;
				pattern.effect(track, row) = effect;
				pattern.effect_value(track, row) = effect_value;
			}
		}
		patterns.push_back(pattern);

	}

	// here be sampledata

	return true;
}

void module_s3m::close() {
	reader.close();
}

std::string module_s3m::name() {
	return "(undefined)";
}


unsigned long module_s3m::sample_read(int instrument, int sample, void* buffer, unsigned long bytes) {
	reader.seek(sampleOffsets[instrument]);
	return reader.readBytes(buffer, bytes);
}

}
}
