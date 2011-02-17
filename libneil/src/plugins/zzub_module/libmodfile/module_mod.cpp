#include <string>
#include <vector>
#include <iostream>
#include "FileReader.h"
#include "module.h"
#include "mod.h"
#include "module_mod.h"

namespace modfile {
namespace mod {

typedef struct {
	const char* desc;
	int channels;
} modformat ;

modformat formats[] = {
	{ "TDZ1", 1 }, 
	{ "TDZ2", 2 }, 
	{ "TDZ3", 3 }, 
	{ "M.K.", 4 }, 
	{ "5CHN", 5 }, 
	{ "6CHN", 6 },	// there are some others for 6 channels
	{ "7CHN", 7 }, 
	{ "8CHN", 8 }, 	// there are some others for 8 channels
	{ "9CHN", 9 }, 
	{ "10CH", 10 }, 
	{ "11CH", 11 }, 
	{ "12CH", 12 }, 
	{ "13CH", 13 }, 
	{ "14CH", 14 }, 
	{ "15CH", 15 }, 
	{ "16CH", 16 }, 
	{ "17CH", 17 }, 
	{ "18CH", 18 }, 
	{ "19CH", 19 }, 
	{ "20CH", 20 }, 
	{ "21CH", 21 }, 
	{ "22CH", 22 }, 
	{ "23CH", 23 }, 
	{ "24CH", 24 }, 
	{ "25CH", 25 }, 
	{ "26CH", 26 }, 
	{ "27CH", 27 }, 
	{ "28CH", 28 }, 
	{ "29CH", 29 }, 
	{ "30CH", 30 }, 
	{ "31CH", 31 }, 
	{ "32CH", 32 }, 
};

const int formatsCount=sizeof(formats) / sizeof(modformat);

struct modfilenote {
	unsigned char a,b,c,d;
};

bool module_mod::open(std::string fileName) {
	if (!reader.open(fileName.c_str())) return false;

	_MODHEADER header;
	reader.readBytes(&header, sizeof(_MODHEADER));
	int numChannels;
	int numPatterns;

	numChannels = 0;
	for (int i = 0; i<formatsCount; i++) {
		if (strncmp(header.desc, formats[i].desc, 4)==0) {
			numChannels = formats[i].channels;
			break;
		}
	}
	if (!numChannels) {
		reader.close();
		return false;
	}

	numPatterns = 0;
	for (int i = 0; i<128; i++) {
		if (header.order[i] > numPatterns)
			numPatterns = header.order[i];
	}
	numPatterns++;

	int patternSize = 4*numChannels*64;	// 4 bytes pr column, always 64 rows

	int sampleStart = sizeof(_MODHEADER) + numPatterns*patternSize;
	int ofs = 0;
	for (int i = 0; i<31; i++) {
		instrument_info instrument;
		sample_info sample;

		sampleOffsets.push_back(sampleStart + ofs);

		char* plen = (char*)&header.sample[i].length;
		swab(plen, plen, 2);

		plen = (char*)&header.sample[i].loopend;
		swab(plen, plen, 2);

		plen = (char*)&header.sample[i].loopstart;
		swab(plen, plen, 2);

		ofs += header.sample[i].length*2;

		sample.amp = (float)header.sample[i].volume / 0x40;
		sample.samples = header.sample[i].length * 2;
		sample.bits_per_sample = 8;
		sample.is_bidir = false;
		sample.is_float = false;
		int loopstart = header.sample[i].loopstart * 2;
		int looplen = header.sample[i].loopend * 2;
		sample.is_loop = looplen>2;
		sample.is_signed = true;
		sample.is_stereo = false;
		sample.samples_per_second = 22050;
		sample.loop_start = loopstart;
		sample.loop_end = loopstart + looplen;
		sample.name = header.sample[i].samplename;
		sample.note_c4_rel = 0;
		sample.tuning = header.sample[i].finetune;
		
		instrument.name = sample.name;
		instrument.samples.push_back(sample);
		instruments.push_back(instrument);
	}

	int rowsize = 4*numChannels;
	char* pdata = new char[patternSize];
	for (int i = 0; i<numPatterns; i++) {
		pattern_type pattern;
		pattern.rows = 64;
		pattern.tracks = numChannels;

		pattern.pattern_matrix.resize(numChannels);

		reader.readBytes(pdata, patternSize);

		for (int chn = 0; chn < numChannels; chn++) {
			pattern.pattern_matrix[chn].resize(64);

			for (int row = 0; row < 64; row++) {
				modfilenote* v = (modfilenote*)(&pdata[chn*4 + row*rowsize]);

				pattern.note(chn, row) = period_to_note(((v->a & 0xF) << 8) | v->b) + 1;
				pattern.sample(chn, row) = (v->a & 0xF0) | (v->c >> 4);
				pattern.volume(chn, row) = 255;
				pattern.effect(chn, row) = v->c & 0xF;
				pattern.effect_value(chn, row) = v->d;
			}

		}
		patterns.push_back(pattern);
		
	}
	delete[] pdata;

	return true;
}

void module_mod::close() {
	reader.close();
}
std::string module_mod::name() {
	std::string songName;
	return songName;
}

unsigned long module_mod::sample_read(int instrument, int sample, void* buffer, unsigned long bytes) {
	reader.seek(sampleOffsets[instrument]);
	return reader.readBytes(buffer, bytes);
}

int module_mod::period_to_note(int period) {
	int numperiods = sizeof(_MODPERIODS) / sizeof(short);
	for (int i = 0; i<numperiods; i++) {
		if (_MODPERIODS[i] == period) return i;
	}
	return -1;
}

}
}
