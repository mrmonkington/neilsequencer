
#include "panning.h"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>

class panning : public lunar::fx<panning> {
public:
	float panl;
	float panr;

	void init() {
	}

	void process_events() {
		if (globals->pan) {
			float angle = (*globals->pan) * M_PI * 0.5;
			panl = cos(angle) * cos(angle);
			panr = sin(angle) * sin(angle);
		}
	}

	void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {
		float *l = outL;
		float *r = outR;
		int ns = n;
		while (ns--) {
			*l++ = *inL++ * panl;
			*r++ = *inR++ * panr;
		}
		dsp_clip(outL, n, 1); // signal may never exceed -1..1
		dsp_clip(outR, n, 1);
		
	}
};

lunar_fx *new_fx() { return new panning(); }
