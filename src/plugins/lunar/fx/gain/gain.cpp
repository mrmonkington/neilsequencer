
#include "gain.h"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>

class gain : public lunar::fx<gain> {
public:
	float amp, mamp, famp, iamp, step;

	void init() {
		amp = 1.0;
		mamp = 1.0;
		famp = 0.0;
		iamp = 0.0;
		step = 1000.0f/(5.0f * transport->samples_per_second);
	}

	void exit() {
		delete this;
	}

	void process_events() {
		if (globals->gain) {
			amp = *globals->gain / 100.0f;
		}
		if (globals->mgain) {
			mamp = dbtoamp(*globals->mgain, -48.0f);
		}
		famp = amp * mamp;
		if (globals->inertia) {
			if (!*globals->inertia)
				step = 1.0f;
			else
				step = 1000.0f/(*globals->inertia * transport->samples_per_second);
		}
	}

	void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {
		float oldiamp;
		
		dsp_set(outL,n,famp);
		dsp_set(outR,n,famp);
		int sn = (int)min(abs((famp - iamp)/step),n);
		if (sn > 1) {
			oldiamp = iamp;
			if (famp > iamp) {
				iamp = dsp_slope(outL,sn,oldiamp,step);
				iamp = dsp_slope(outR,sn,oldiamp,step);
			} else {
				iamp = dsp_slope(outL,sn,oldiamp,-step);
				iamp = dsp_slope(outR,sn,oldiamp,-step);
			}
		}
		dsp_mul(inL, outL, n);
		dsp_mul(inR, outR, n);
		dsp_clip(outL, n, 1); // signal may never exceed -1..1
		dsp_clip(outR, n, 1);
	}

};

lunar_fx *new_fx() { return new gain(); }
