
#include "crossfade.h"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>

class crossfade : public lunar::fx<crossfade> {
public:
  float out1, out2;
	void init() {
	}

	void exit() {
		delete this;
	}

	void process_events() {
		if (globals->crossfade) {
		  out1 = *globals->crossfade;
		  out2 = 1.0f - out1;
		}

	}

	void process_controller_events() {
	  controllers->out1 = &out1;
	  controllers->out2 = &out2;
	}

	void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {
	}

};

lunar_fx *new_fx() { return new crossfade(); }
