#pragma once

namespace modfile {
namespace mod {

struct module_mod : module {
	std::vector<long> sampleOffsets;
	file::FileReader reader;

	virtual bool open(std::string fileName);
	virtual void close();
	virtual std::string format() { return "mod"; }
	std::string name();
	virtual unsigned long sample_read(int instrument, int sample, void* buffer, unsigned long bytes);

	int period_to_note(int period);
};

}
}
