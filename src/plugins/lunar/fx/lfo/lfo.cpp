
#include "lfo.h"
#include <lunar/fx.hpp>

class lfo : public lunar::fx<lfo> {
public:
	float phase;
	float val;

	void init() {
		val = 0;
		phase = 0;
	}

	void exit() {
		delete this;
	}

	void process_events() {
		phase += 2*M_PI/transport->ticks_per_second;
		if (phase >= 2*M_PI)
			phase -= 2*M_PI;
		val = (1.0f+sin(phase))*0.5f;
		controllers->out1 = &val;
	}
	
	void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {
	}

};

lunar_fx *new_fx() { return new lfo(); }
