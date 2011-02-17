#pragma once

namespace modfile {
namespace xm {

struct module_xm : module {
	file::FileReader reader;
	std::string fileName;
	char channelPan[32];
	std::vector<std::vector<long > > sampleOffsets;

	virtual bool open(std::string fileName);
	virtual void close();
	virtual std::string format() { return "xm"; }
	std::string name();
	virtual unsigned long sample_read(int instrument, int sample, void* buffer, unsigned long bytes);

};

}

}
