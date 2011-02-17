#pragma once

namespace modfile {
namespace it {

struct it_note {
	unsigned char note;
	unsigned char sample;
	unsigned char volume;
	unsigned char pan;
	unsigned char effect;
	unsigned char effect_value;
};

// user_flags for IT
#define IT_SAMPLE_COMPRESS	0x0001
#define IT_SAMPLE_DELTA		0x0002

struct _ITINSTRUMENT_NEW;

struct module_it : module {
	file::FileReader reader;
	std::string fileName;
	std::vector<std::vector<long> > sampleOffsets;
	std::vector<std::vector<int> > instrumentSamples;

	virtual bool open(std::string fileName);
	virtual void close();
	virtual std::string format() { return "it"; }
	std::string name();

	unsigned long sample_read(int instrument, int sample, void* buffer, unsigned long bytes);

	void getInstrumentSamples(_ITINSTRUMENT_NEW* instrument, std::vector<int>& sampleIndices);
};

}
}
