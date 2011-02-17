
#include "philthy.h"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>
#include "filters.h"

class philthy : public lunar::fx<philthy> {
public:

	float l_filter_type, l_cutoff, l_resonance, l_thevfactor;

	C6thOrderFilter fk1;
	C6thOrderFilter fk2;

	void update_filters() {
		fk1.setup(transport->samples_per_second, l_filter_type, l_cutoff, l_resonance, l_thevfactor);
		fk2.setup(transport->samples_per_second, l_filter_type, l_cutoff, l_resonance, l_thevfactor);		
	}

	void init() {
        //~ fk1.CalcCoeffs(2, 100, 0, 25/50.0f);
        //~ fk2.CalcCoeffs(2, 100, 0, 25/50.0f);
        //~ fk1.ResetFilter();
        //~ fk2.ResetFilter();        
        update_filters();
	}

	void exit() {
	}

	void process_events() {
		int update = 0;
		
		if (globals->filter_type) {
			l_filter_type = *globals->filter_type;
			update = 1;
		}
		if (globals->cutoff) {
			l_cutoff = *globals->cutoff;
			update = 1;
		}
		if (globals->resonance) {
			l_resonance = *globals->resonance;
			update = 1;
		}
        if (globals->thevfactor) {
			l_thevfactor = *globals->thevfactor;
			update = 1;
		}
		if (update)
			update_filters();
	}

	void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {
		dsp_copy(inL, outL, n);
		dsp_copy(inR, outR, n);
		fk1.process(outL, n);
		fk2.process(outR, n);
		dsp_clip(outL, n, 1); // signal may never exceed -1..1
		dsp_clip(outR, n, 1);
	}
};

lunar_fx *new_fx() { return new philthy(); }
