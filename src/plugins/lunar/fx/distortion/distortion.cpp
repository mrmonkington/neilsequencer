#include "distortion.h"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>

class Dist {
protected:
  float a, b, c;
public:
  void set_a(float a) {
    this->a = a;
  }

  float get_a() {
    return this->a;
  }

  void set_b(float b) {
    this->b = b;
  }

  float get_b() {
    return this->b;
  }

  void set_c(float c) {
    this->c = c;
  }

  float get_c() {
    return this->c;
  }

  virtual void process(float *in, float *out, int n) {
    for (int i = 0; i < n; i++) {
      out[i] = in[i];
    }
  }
};

class Harmonic : public Dist {
  void process(float *in, float *out, int n) {
    float a, b, c, x;
    a = this->get_a();
    b = this->get_b();
    c = this->get_c();
    for (int i = 0; i < n; i++) {
      x = in[i];
      out[i] = sin(x) + a * sin(2.0 * x) + b * sin(3.0 * x) + c * sin(3.0 * x);
      out[i] /= 1.0 + a + b + c;
    }
  }
};

class Shaper1 : public Dist {
  void process(float *in, float *out, int n) {
    float a, x, k;
    a = (this->get_a() - 0.5) * 1.99;
    k = 2.0 * a / (1.0 - a);
    for (int i = 0; i < n; i++) {
      x = in[i];
      out[i] = (1.0 + k) * x / (1.0 + k * abs(x));
    }
  }
};

class Shaper2 : public Dist {
  void process(float *in, float *out, int n) {
    float x;
    for (int i = 0; i < n; i++) {
      x = in[i];
      out[i] = 1.5 * x - 0.5 * x * x * x;
    }
  }
};

class Shaper3 : public Dist {
  void process(float *in, float *out, int n) {
    float x, a;
    for (int i = 0; i < n; i++) {
      x = in[i] * 0.686306;
      a = 1.0 + exp(sqrt(fabs(x)) * -0.75);
      out[i] = (exp(x) - exp(-x * a)) / (exp(x) + exp(-x));
    }
  }
};

class distortion : public lunar::fx<distortion> {
public:
  static const int n_algs = 4;
  int alg;
  float preamp, postamp;
  Dist *algs[n_algs];

  void init() {
    algs[0] = new Harmonic();
    algs[1] = new Shaper1();
    algs[2] = new Shaper2();
    algs[3] = new Shaper3();
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
    if (globals->a) {
      for (int i = 0; i < n_algs; i++) {
	algs[i]->set_a(*globals->a);
      }
    }
    if (globals->b) {
      for (int i = 0; i < n_algs; i++) {
	algs[i]->set_b(*globals->b);
      }
    }
    if (globals->c) {
      for (int i = 0; i < n_algs; i++) {
	algs[i]->set_c(*globals->c);
      }
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
