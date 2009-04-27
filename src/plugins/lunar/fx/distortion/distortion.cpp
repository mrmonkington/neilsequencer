
#include "distortion.h"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>

const float ATAN_FACTOR = 1.0f/1.5707963167948966f;
#define TABSIZE 2048


class distortion : public lunar::fx<distortion> {
public:
  float lx0, lx1, ly0, ly1;
  float rx0, rx1, ry0, ry1;
  float r;
  float asymmetry;
  float rect;

  float preamp, postamp;

  void init() {
    r = 1.0 - (190.0 / (float)transport->samples_per_second);
    asymmetry = 1.0;
    rect = 0.0;
    preamp = 1.0f;
    postamp = 1.0f;
    lx0 = lx1 = ly0 = ly1 = 0.0;
    rx0 = rx1 = ry0 = ry1 = 0.0;
  }

  void exit() {
    delete this;
  }

  void process_events() {
    if (globals->pregain) {
      preamp = dbtoamp(*globals->pregain, -48.0f);
    }
    if (globals->postgain) {
      postamp = dbtoamp(*globals->postgain, -48.0f);
    }
    if (globals->asymmetry) {
      asymmetry = *globals->asymmetry;
    }
    if (globals->rectification) {
      rect = *globals->rectification;
    }
  }

  void remove_dc(float *inl, float *inr, int n) {
    for (int i = 0; i < n; i++) {
      lx1 = lx0;
      rx1 = rx0;
      lx0 = inl[i];
      rx0 = inr[i];
      ly1 = ly0;
      ry1 = ry0;
      ly0 = lx0 - lx1 + r * ly1;
      ry0 = rx0 - rx1 + r * ry1;
      inl[i] = ly0;
      inr[i] = ry0;
    }
  }

  void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {
    dsp_copy(inL, outL, n);
    dsp_copy(inR, outR, n);
    dsp_amp(inL, n, preamp);
    dsp_amp(inR, n, preamp);
    float ls, rs;
    for (int i = 0; i < n; i++) {
      ls = inL[i] + rect;
      rs = inR[i] + rect;
      outL[i] = (exp(ls) - exp(-ls * asymmetry)) / (exp(ls) + exp(-ls)) - rect;
      outR[i] = (exp(rs) - exp(-rs * asymmetry)) / (exp(rs) + exp(-rs)) - rect;
    }
    remove_dc(outL, outR, n);
    dsp_amp(outL, n, postamp);
    dsp_amp(outR, n, postamp);
    dsp_clip(outL, n, 1); // signal may never exceed -1..1
    dsp_clip(outR, n, 1);
  }
};

lunar_fx *new_fx() { return new distortion(); }
