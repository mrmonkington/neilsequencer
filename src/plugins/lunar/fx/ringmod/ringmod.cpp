
#include "ringmod.h"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>

class ringmod : public lunar::fx<ringmod> {
public:

	void init() {
	}

	void exit() {
		delete this;
	}

	void process_events() {
	}

	void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {
	  while (n--) {
	    *outL++ = *outR++ = *inL++ *= *inR++;
	  }
	}

};

lunar_fx *new_fx() { return new ringmod(); }
