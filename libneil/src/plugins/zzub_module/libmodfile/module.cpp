#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include "FileReader.h"
#include "module.h"
#include "module_xm.h"
#include "module_s3m.h"
#include "module_mod.h"
#include "module_it.h"

using namespace std;

namespace modfile {

char transform_tolower(char c) {
	return tolower(c);
}

module* module::create(std::string fileName) {
	size_t ld = fileName.find_last_of('.');
	if (ld == -1) return 0;
	
	string ext = fileName.substr(ld + 1);
	transform(ext.begin(), ext.end(), ext.begin(), transform_tolower);
	
	module* archive = 0;
	if (ext == "xm") archive = new xm::module_xm(); else
	if (ext == "s3m") archive = new s3m::module_s3m(); else
	if (ext == "mod") archive = new mod::module_mod(); else
	if (ext == "it") archive = new it::module_it(); else
		return 0;

	if (!archive->open(fileName)) {
		delete archive;
		return 0;
	}

	return archive;
}

}
