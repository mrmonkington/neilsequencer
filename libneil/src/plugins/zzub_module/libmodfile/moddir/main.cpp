#include <string>
#include <vector>
#include <iostream>
#include "../module.h"

#if defined(_DEBUG)
#pragma comment (lib, "../Debug/libmodfile.lib")
#else
#pragma comment (lib, "../Release/libmodfile.lib")
#endif

using namespace std;
using namespace modfile;

std::string fillString(char c, int l) {
	std::string r="";
	for (int i=0; i<l; i++) {
		r+=c;
	}
	return r;
}

void dump_module_dir(const char* path, int indent = 0) {
	modfile::module* mod = module::create(path);
	if (!mod) return ;

	for (int i = 0; i<mod->instruments.size(); i++) {
		cout << mod->instruments[i].name << " (" << mod->instruments[i].samples.size() << " sample streams):" << endl;
		for (int j = 0; j<mod->instruments[i].samples.size(); j++) {
			sample_info& info = mod->instruments[i].samples[j];
			cout << "    " << info.name << endl;
			cout << "    " << info.bits_per_sample << " bits, " << info.samples << (info.is_stereo?" stereo ":" mono ") << "samples." << endl;
			cout << endl;
		}
	}

}

int main(int argc, char** argv) {

//	dump_module_dir("../modextract/test.xm");
//	dump_module_dir("../modextract/test.s3m");
//	dump_module_dir("../modextract/test.mod");
//	dump_module_dir("../modextract/test.it");
	dump_module_dir("../modextract/The Trackering IV.xm");

	return 0;
}
