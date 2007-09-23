
#include "transpose.h"
#include <lunar/fx.hpp>

class transpose : public lunar::fx<transpose> {
public:
	float otrans;
	float ntrans;
	
	void init() {
		otrans = 0.0f;
		ntrans = 0.0f;
	}

	void exit() {
		delete this;
	}

	void process_events() {
		if (globals->otrans)
			otrans = *globals->otrans - 5.0f;
		if (globals->ntrans)
			ntrans = *globals->ntrans - 6.0f;
	}
	
	void process_controller_events() {
		if (controllers->note && (*controllers->note != 0)) {
			*controllers->note = *controllers->note + otrans*12.0f + ntrans;
		}
	}
	
	void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {
	}

};

lunar_fx *new_fx() { return new transpose(); }
