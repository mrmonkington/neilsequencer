
#include "reverb.h"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>

namespace freeverb {
#include "revmodel.h"
#include "revmodel.cpp"
#include "comb.cpp"
#include "allpass.cpp"
	
}

class reverb : public lunar::fx<reverb>, public freeverb::revmodel {
public:
	void init() {
	}

	void exit() {
	}

	void process_events() {
		if (globals->roomsize)
			setroomsize(*globals->roomsize);
		if (globals->damp)
			setdamp(*globals->damp);
		if (globals->wet)
			setwet(dbtoamp(*globals->wet,-48.0f));
		if (globals->dry)
			setdry(dbtoamp(*globals->dry,-48.0f));
		if (globals->width)
			setwidth(*globals->width);
		if (globals->freeze)
			setmode(*globals->freeze);
	}

	void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {
		processreplace(inL, inR, outL, outR, n, 1);
		dsp_clip(outL, n, 1);
		dsp_clip(outR, n, 1); // signal may never exceed -1..1
	}
	
};

lunar_fx *new_fx() { return new reverb(); }
