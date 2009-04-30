#include "distortion.h"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>

class distortion : public lunar::fx<distortion> {
public:
  float preamp, postamp;

  void init() {
    preamp = 1.0f;
    postamp = 1.0f;
  }

  void exit() {
    delete this;
  }

  void process_events() {
    if (globals->pregain) {
      preamp = pow(10.0, *globals->pregain / 10.0);
    }
    if (globals->postgain) {
      postamp = pow(10.0, *globals->postgain / 10.0);
    }
  }

  void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {
    dsp_copy(inL, outL, n);
    dsp_copy(inR, outR, n);
    dsp_amp(inL, n, preamp);
    dsp_amp(inR, n, preamp);
    float ls, rs;
    for (int i = 0; i < n; i++) {
      ls = inL[i];
      rs = inR[i];
      outL[i] = (exp(ls) - exp(-ls)) / (exp(ls) + exp(-ls));
      outR[i] = (exp(rs) - exp(-rs)) / (exp(rs) + exp(-rs));
    }
    dsp_amp(outL, n, postamp);
    dsp_amp(outR, n, postamp);
    dsp_clip(outL, n, 1); // signal may never exceed -1..1
    dsp_clip(outR, n, 1);
  }
};

lunar_fx *new_fx() { return new distortion(); }
