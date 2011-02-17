#include <string>
#include <vector>
#include <iostream>
#include "FileReader.h"
#include "module.h"
#include "xm.h"
#include "module_xm.h"

namespace modfile {
namespace xm {

struct xmfilenote {
	unsigned char a,b,c,d,e;
};

bool module_xm::open(std::string fileName) {
	if (!reader.open(fileName.c_str())) return false;

	_XMHEADER header;
	reader.readBytes(&header, sizeof(_XMHEADER));

	for (int i = 0; i < header.numPats; i++) {

		_XMPATTERN patternHeader;
		reader.readBytes(&patternHeader, sizeof(_XMPATTERN));

		pattern_type pattern;
		pattern.rows = patternHeader.rows;
		pattern.tracks = header.numChans;

		pattern.pattern_matrix.resize(header.numChans);

		// read packed patterns

		for (int chn = 0; chn < header.numChans; chn++) {
			pattern.pattern_matrix[chn].resize(pattern.rows);
		}
		for (int row = 0; row < patternHeader.rows; row++) {
			for (int chn = 0; chn < header.numChans; chn++) {
				// blank patterns are only distinguished by their zero length
				if (patternHeader.patternSize == 0) continue;

				unsigned char note = 0, sample = 0, volume = 255, effect = 0, effect_value = 0;
				unsigned char pack;
				reader.read(&pack);
				if (pack & 0x80) {
					if (pack & 1) reader.read(&note);
					if (pack & 2) reader.read(&sample);
					if (pack & 4) reader.read(&volume);
					if (pack & 8) reader.read(&effect);
					if (pack & 16) reader.read(&effect_value);
				} else {
					note = pack;
					reader.read(&sample);
					reader.read(&volume);
					reader.read(&effect);
					reader.read(&effect_value);
				}
				if (note == 97) note = -1;
				pattern.note(chn, row) = note;
				pattern.sample(chn, row) = sample;
				pattern.volume(chn, row) = volume;
				pattern.effect(chn, row) = effect;
				pattern.effect_value(chn, row) = effect_value;
			}
		}
		patterns.push_back(pattern);
		//reader.seek(pattern.patternSize, SEEK_CUR);
	}

	sampleOffsets.resize(header.numInstr);

	for (int i = 0; i < header.numInstr; i++) {
		instrument_info instrument;
		int headerSize;
		_XMINSTRUMENT instrumentHeader;
		reader.read(&headerSize);

		if (0 == reader.readBytes(&instrumentHeader, sizeof(_XMINSTRUMENT))) break;	// eof = no more samples?
		instrumentHeader.name[21] = 0;
		instrument.name = instrumentHeader.name;

		_XMSAMPLE* samples = new _XMSAMPLE[instrumentHeader.numSamples];
		_XMSAMPLES samplesHeader;
		int samplesHeaderSize = 0;

		reader.read(&samplesHeaderSize);

		if (instrumentHeader.numSamples > 0) {
			reader.readBytes(&samplesHeader, sizeof(_XMSAMPLES));

			for (int j = 0; j < instrumentHeader.numSamples; j++) {
				reader.readBytes(&samples[j], sizeof(_XMSAMPLE));
				samples[j].samplename[21] = 0;

				int bytesPerSample = (samples[j].flags & sample_flag_16bit)?2:1;

				sample_info sample;
				sample.name = samples[j].samplename;
				sample.amp = (float)samples[j].vol / 255.0f;
				sample.bits_per_sample = (samples[j].flags & sample_flag_16bit) != 0?16:8;
				sample.samples = samples[j].len / bytesPerSample;
				sample.loop_start = samples[j].loopstart / bytesPerSample;
				sample.loop_end = (samples[j].loopstart + samples[j].looplen) / bytesPerSample;
				sample.is_bidir = (samples[j].flags & sample_flag_loop_type_mask) == sample_loop_bidir;
				sample.is_loop = (samples[j].flags & sample_flag_loop_type_mask ) != 0;
				sample.is_signed = true;
				sample.tuning = 0;
				sample.note_c4_rel = samples[j].relnote;
				sample.is_float = false;
				sample.is_stereo = false;
				sample.samples_per_second = 22050;
				instrument.samples.push_back(sample);

			}

			sampleOffsets[i].resize(instrumentHeader.numSamples);
			// here be sampledata
			for (int j = 0; j < instrumentHeader.numSamples; j++) {
				long pos = reader.position();
				sampleOffsets[i][j] = pos;
				long skip = samples[j].len;
				reader.seek(skip, SEEK_CUR);
			}
		}

		instruments.push_back(instrument);

		delete[] samples;
	}
	return true;
}

void module_xm::close() {
	reader.close();
}

std::string module_xm::name() {
	std::string songName = "(nothing yet)";
//    songName.assign(header.songName, 20);
	return songName;
}


unsigned long module_xm::sample_read(int instrument, int sample, void* buffer, unsigned long bytes) {
	int offset = sampleOffsets[instrument][sample];
	reader.seek(offset);
	int read = reader.readBytes(buffer, bytes);
	if (read != bytes) return read;

	int bits = instruments[instrument].samples[sample].bits_per_sample;
	int numSamples = instruments[instrument].samples[sample].samples;

	int olds = 0;

	// these are delta values, undelta 16 bit separately
	if (bits == 8) {
		char* sample = (char*)buffer;
		for (int i = 0; i < numSamples; i++) {
			int news = sample[i]+olds;
			sample[i] = news;
			olds = news;
		}
	} else
	if (bits==16) {
		short* sample = (short*)buffer;
		for (int i = 0; i < numSamples; i++) {
			int news = sample[i]+olds;
			sample[i] = news;
			olds = news;
		}
	}
	return read;
}


}
}
