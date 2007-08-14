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

/*! \struct pluginloader
    \brief Helper class for loading a specific plugin from a DLL.
*/

pluginlib::pluginlib(const std::string& fileName, zzub::player &p, zzub::plugincollection *_collection) : player(p) {
	refcount = 0;
	this->fileName=fileName;
	this->hMachine=0;
	if (_collection) { // internal
		this->initialized=true;
		collection = _collection;
		this->collection->initialize(this);
	} else { // external
		this->initialized=false;
		collection = 0;
		initDll();
	}	
}

pluginlib::~pluginlib()
{
	unload();
}

void pluginlib::unload() {
	printf("unloading plugin library\n");
	if (collection) {
		collection->destroy();
		collection = 0;
	}
	for (std::list<pluginloader*>::iterator i=loaders.begin(); i != loaders.end(); ++i)
	{
		delete *i;
	}
	loaders.clear();
	if (hMachine) {
		xp_dlclose(hMachine);
		hMachine=0;
	}
	initialized=false; 	
//	plugin_info=0;
}

void pluginlib::initDll() {
	// NOTE: buzz.exe does not leave loadlibrary and getprocaddress-traces while profiling with depends.exe
	
	if (initialized) return ;
//#if defined(_WIN32)
	//~ char currentdir[1024];
	//~ GetCurrentDirectory(1024, currentdir);

	printf("loading machine '%s'\n", fileName.c_str());

	bool is_so = false;
	bool is_win32 = false;
	
	int dpos=(int)fileName.find_last_of('.');
	string fileExtension = fileName.substr(dpos);
	is_so = (fileExtension == ".so");
	is_win32 = (fileExtension == ".dll");
	
	if (is_so || is_win32) {
		this->hMachine=xp_dlopen(fileName.c_str());
		if (this->hMachine==0) {
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
		} else {
			// is there an entry function?
			if (_func) {
				this->collection = _func(); // get our collection instance
				if (this->collection) {
					this->collection->initialize(this);
				} else {
					printf("%s: collection pointer is zero.\n", fileName.c_str());
				}
			} else {
				// let the user know
				printf("%s: entry function missing.\n", fileName.c_str());
			}
		}
	} else {
		// let the user know
		printf("%s: signature function missing.\n", fileName.c_str());
	}
	
	initialized=true;
	
}

void pluginlib::register_info(const zzub::info *_info) {
	// small sanity check (this is legacy)
	if (!_info->name || !_info->short_name) {
		// let the user know
		printf("%s: info name or short_name is empty.\n", fileName.c_str());
	} else {
		// add a pluginloader for this info struct
		pluginloader *loader = new pluginloader(this, _info);
		loaders.push_back(loader);
		player.registerMachineLoader(loader);
	}
}

int pluginlib::getPlugins()
{
	return loaders.size();
}

/// pluginloader ///

pluginloader::pluginloader(pluginlib *lib, const info *_info) {	
	this->lib = lib;
	this->plugin_info = _info;
	/*
	if (this->plugin_info->name) {
        // some machines have terminating whitespace (ex HD Combo), so we trim it here
		this->fullName= trim(this->plugin_info->name);
	}
	this->uri=trim(this->plugin_info->uri);
	transform(this->uri.begin(), this->uri.end(), this->uri.begin(), toLower);	
	*/
	// XXX: deduce presetfile from machinename in info struct, not dll name
	//~ int ld=fileName.find_last_of('.');
	//~ string presetFile=fileName.substr(0, ld)+".prs"; //getPlayer()->getMachinePath(machine->machineInfo.Type) + machine->fullName + ".prs";
	//~ presets.load(presetFile);
	
	// XXX: deduce helpfile from machinename in info struct, not dll name
	//~ helpFile=fileName.substr(0, ld)+".html";
	//~ FILE* f=fopen(helpFile.c_str(), "rb");
	//~ if (!f)
	//~ helpFile=""; else
	//~ fclose(f);

}

pluginloader::~pluginloader() {
}

const info* pluginloader::getInfo() {
	return plugin_info;
}

plugin* pluginloader::createMachine() {
	const info *i = getInfo();
	if (!i) return 0;
	return plugin_info->create_plugin();
}

int pluginloader::getType() {
	const info* i=getInfo();
	if (!i) return -1;
	return i->type;
}

const char* pluginloader::getName() {
	return plugin_info->name;
}

const char* pluginloader::getUri() {
	return plugin_info->uri;
}

static std::string empty_string;
std::string pluginloader::getFile() {
	if (lib)
		return lib->fileName;
	return empty_string;
}

std::string pluginloader::getHelpFile() {
	return helpFile;
}

};
