#include "distortion.h"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>

class Dist {
public:
  virtual void process(float *in, float *out, int n) {
    for (int i = 0; i < n; i++) {
      out[i] = in[i];
    }
  }
};

class Smooth : public Dist {
  void process(float *in, float *out, int n) {
    float x;
    for (int i = 0; i < n; i++) {
      x = in[i];
      if (x >= 1.0)
	out[i] = 1.0;
      else if (x <= -1.0)
	out[i] = -1.0;
      else
	out[i] = 1.5 * x - 0.5 * x * x * x;
    }
  }
};

class Hard : public Dist {
  void process(float *in, float *out, int n) {
    float x;
    for (int i = 0; i < n; i++) {
      x = in[i];
      if (x >= 1.0)
	out[i] = 1.0;
      else if (x <= -1.0)
	out[i] = -1.0;
      else
	out[i] = x;      
    }
  }
};

class Sine : public Dist {
  void process(float *in, float *out, int n) {
    float x;
    for (int i = 0; i < n; i++) {
      x = in[i];
      out[i] = sin(2.0 * M_PI * x);
    }
  }
};

class distortion : public lunar::fx<distortion> {
public:
  static const int n_algs = 3;
  int alg;
  float preamp, postamp;
  Dist *algs[n_algs];

  void init() {
    algs[0] = new Smooth();
    algs[1] = new Hard();
    algs[2] = new Sine();
    preamp = 1.0f;
    postamp = 1.0f;
  }

  void exit() {
    for (int i = 0; i < n_algs; i++) {
      delete algs[i];
    }
    delete this;
  }

  void process_events() {
    if (globals->alg) {
      this->alg = *globals->alg;
    }
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

    this->algs[alg]->process(inL, outL, n);
    this->algs[alg]->process(inR, outR, n);

    dsp_amp(outL, n, postamp);
    dsp_amp(outR, n, postamp);

    dsp_clip(outL, n, 1); // signal may never exceed -1..1
    dsp_clip(outR, n, 1);
  }
};

lunar_fx *new_fx() { return new distortion(); }
