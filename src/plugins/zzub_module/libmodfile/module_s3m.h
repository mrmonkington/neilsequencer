#pragma once

namespace modfile {
namespace s3m {

struct s3m_note {
	unsigned char note;
	unsigned char sample;
	unsigned char volume;
	unsigned char effect;
	unsigned char effect_value;
};

typedef std::vector<s3m_note> s3m_track;
typedef std::vector<s3m_track> s3m_pattern;

struct module_s3m : module {
	file::FileReader reader;
	std::vector<long> sampleOffsets;

	virtual bool open(std::string fileName);
	virtual void close();
	virtual std::string format() { return "s3m"; }
	std::string name();
	virtual unsigned long sample_read(int instrument, int sample, void* buffer, unsigned long bytes);
};

}
}
