/*
Copyright (C) 2003-2007 Anders Ervik <calvin@countzero.no>
Copyright (C) 2006-2007 Leonard Ritter

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <cstdio>
#include <string>
#include <algorithm>
#include "common.h"
#include "tools.h"
#include "zzub/signature.h"
#include "player.h"

#ifdef POSIX
#include <dlfcn.h>
#endif

namespace zzub {

using namespace std;

/*! \struct pluginlib
    \brief Enumerates plugins from plugin DLLs.
*/

pluginlib::pluginlib(const std::string& _fileName, zzub::player &p, zzub::plugincollection *_collection) : player(p) {
	fileName = _fileName;
	hMachine = 0;
	if (_collection) { // internal
		collection = _collection;
		this->collection->initialize(this);
	} else { // external
		collection = 0;
		init_dll();
	}	
}

pluginlib::~pluginlib() {
	unload();
}

void pluginlib::unload() {
	printf("unloading plugin library\n");
	if (collection) {
		collection->destroy();
		collection = 0;
	}

	loaders.clear();

	if (hMachine) {
		xp_dlclose(hMachine);
		hMachine = 0;
	}
}

void pluginlib::init_dll() {
	printf("loading machine '%s'\n", fileName.c_str());

	bool is_so = false;
	bool is_win32 = false;
	
	int dpos = (int)fileName.find_last_of('.');
	string fileExtension = fileName.substr(dpos);
	is_so = (fileExtension == ".so");
	is_win32 = (fileExtension == ".dll");
	
	if (is_so || is_win32) {
		hMachine = xp_dlopen(fileName.c_str());
		if (hMachine == 0) {
			std::cerr << "error loading plugin library " << fileName << ": " << xp_dlerror() << std::endl;
			return ;
		}
	}
		
	zzub_get_signature_function _sig_func = (zzub_get_signature_function)xp_dlsym(hMachine, "zzub_get_signature");
	zzub_get_plugincollection_function _func=(zzub_get_plugincollection_function)xp_dlsym(hMachine, "zzub_get_plugincollection");
	// do we have a signature function?
	if (_sig_func) {
		// is it in synch with ours?
		const char *signature = _sig_func();
		if (!signature || strcmp(signature,ZZUB_SIGNATURE)) {
			// let the user know
			printf("%s: bad signature '%s' (expected '%s'), won't load.\n", fileName.c_str(), signature, ZZUB_SIGNATURE);
		} 
		//else {
			// is there an entry function?
			if (_func) {
				this->collection = _func(); // get our collection instance
				if (this->collection) {
					this->collection->initialize(this);
					return ;
				} else {
					printf("%s: collection pointer is zero.\n", fileName.c_str());
				}
			} else {
				// let the user know
				printf("%s: entry function missing.\n", fileName.c_str());
			}
		//}
	} else {
		// let the user know
		printf("%s: signature function missing.\n", fileName.c_str());
	}

	// there was an error, close the handle
	xp_dlclose(hMachine);
	hMachine = 0;
}

void pluginlib::register_info(const zzub::info *_info) {
	// add a pluginloader for this info struct
	loaders.push_back(_info);
	player.plugin_infos.push_back(_info);
}

};
