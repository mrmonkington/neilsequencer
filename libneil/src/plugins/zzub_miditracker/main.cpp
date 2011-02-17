#include <string>
#include <cassert>
#include <cmath>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <zzub/signature.h>
#include <zzub/plugin.h>
#include "miditracker.h"
#include "midicc.h"
#include "miditime.h"

using namespace std;

const char *zzub_get_signature() { return ZZUB_SIGNATURE; }

struct midipluginscollection : zzub::plugincollection {
	virtual void initialize(zzub::pluginfactory *factory) {
		factory->register_info(&miditracker::_miditracker_info);
		factory->register_info(&midicc::_midicc_info);
		factory->register_info(&miditime::_miditime_info);
	}
	
	virtual const zzub::info *get_info(const char *uri, zzub::archive *data) { return 0; }
	virtual void destroy() { delete this; }
	// Returns the uri of the collection to be identified,
	// return zero for no uri. Collections without uri can not be 
	// configured.
	virtual const char *get_uri() { return 0; }
	
	// Called by the host to set specific configuration options,
	// usually related to paths.
	virtual void configure(const char *key, const char *value) {}
};

zzub::plugincollection *zzub_get_plugincollection() {
	return new midipluginscollection();
}




