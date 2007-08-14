
#include "filter.h"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>
#include "svf.h"

class filter : public lunar::fx<filter> {
public:

	float l_ftype, l_freq, l_res;

	svf fk1;
	svf fk2;

	void update_filters() {
		fk1.setup(transport->samples_per_second, l_freq, l_res, 0);
		fk2.setup(transport->samples_per_second, l_freq, l_res, 0);
	}

	void init() {
	}

	void exit() {
	}

	void process_events() {
		int update = 0;
		
		if (globals->ftype) {
			l_ftype = *globals->ftype;
			update = 1;
		}
		if (globals->freq) {
			l_freq = *globals->freq;
			update = 1;
		}
		if (globals->res) {
			l_res = *globals->res;
			update = 1;
		}
		if (update)
			update_filters();
	}

	void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {
		dsp_copy(inL, outL, n);
		dsp_copy(inR, outR, n);
		fk1.process(outL, n, int(l_ftype));
		fk2.process(outR, n, int(l_ftype));
		dsp_clip(outL, n, 1); // signal may never exceed -1..1
		dsp_clip(outR, n, 1);
		
	}
};

lunar_fx *new_fx() { return new filter(); }
