
#include "filter.h"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>

class Lag {
private:
  float *output;
  float val, increment;
  int counter;

  inline float next() {
    if (counter > 0) {
      counter -= 1;
      return val += increment;
    } else {
      return val;
    }
  }
public:
  Lag(int n) {
    this->val = 0.0;
    counter = 0;
    output = new float[n];
  }

  ~Lag() {
    delete output;
  }

  void set_val(float target, int delay) {
    if (delay == 0) {
      val = target;
    }
    else {
      counter = delay;
      increment = (target - val) / (float)delay;
    }
  }

  inline float get_val() {
    return val;
  }

  float *get_output() {
    return output;
  }

  void process(int n, int fs) {
    for (int i = 0; i < n; i++) {
      output[i] = next();
    }
  }
};

class Svf {
private:
  float low, high, band, notch;
  float *input, *output, *cutoff, *x, *y;
  float q;
public:
  Svf(int n) {
    output = new float[n];
    low = high = band = 0.0;
  }

  ~Svf() {
    delete output;
  }

  void set_input(float *input) {
    this->input = input;
  }

  void set_cutoff(float *cutoff) {
    this->cutoff = cutoff;
  }

  void set_x(float *x) {
    this->x = x;
  }

  void set_y(float *y) {
    this->y = y;
  }

  void set_q(float reso) {
    this->q = sqrt(1.0 - atan(sqrt(reso * 100.0)) * 2.0 / M_PI) * 1.33 - 0.33;
  }

  float *get_output() {
    return output;
  }
  
  void process(int n, int fs) {
    float scale, f, pair1, pair2;
    scale = sqrt(q);
    for (int i = 0; i < n; i++) {
      f = cutoff[i] / (float)fs * 2.0;
      for (int j = 0; j < 2; j++) {
	low = low + f * band;
	high = scale * input[i] - low - q * band;
	band = f * high + band;
	notch = high + low;
      }
      pair1 = low * y[i] + high * (1.0 - y[i]);
      pair2 = band * y[i] + notch * (1.0 - y[i]);
      output[i] = pair2 * x[i] + pair1 * (1.0 - x[i]);
    }
  }
};

class filter : public lunar::fx<filter> {
public:

  float l_ftype, l_freq, l_res, l_x, l_y;
  int inertia;

  Lag *lag_cutoff, *lag_x, *lag_y;
  Svf *fleft, *fright;
    
  void init() {
    int n = transport->samples_per_tick;
    lag_cutoff = new Lag(n);
    lag_x = new Lag(n);
    lag_y = new Lag(n);
    fleft = new Svf(n);
    fright = new Svf(n);
  }
  
  void exit() {
    delete lag_cutoff;
    delete lag_x;
    delete lag_y;
    delete fleft;
    delete fright;
  }

  void process_events() {
    if (globals->freq) {
      l_freq = *globals->freq;
      lag_cutoff->set_val(l_freq, inertia * transport->samples_per_tick);
    }
    if (globals->res) {
      l_res = *globals->res;
      fleft->set_q(l_res);
      fright->set_q(l_res);
    }
    if (globals->x) {
      l_x = *globals->x;
      lag_x->set_val(l_x, inertia * transport->samples_per_tick);
    }
    if (globals->y) {
      l_y = *globals->y;
      lag_y->set_val(l_y, inertia * transport->samples_per_tick);
    }
    if (globals->inertia) {
      inertia = *globals->inertia;
    }
  }
  
  void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {
    int fs;
    fs = transport->samples_per_second;
    fleft->set_input(inL);
    fright->set_input(inR);
    fleft->set_cutoff(lag_cutoff->get_output());
    fright->set_cutoff(lag_cutoff->get_output());
    fleft->set_x(lag_x->get_output());
    fright->set_x(lag_x->get_output());
    fleft->set_y(lag_y->get_output());
    fright->set_y(lag_y->get_output());
    lag_cutoff->process(n, fs);
    lag_x->process(n, fs);
    lag_y->process(n, fs);
    fleft->process(n, fs);
    fright->process(n, fs);
    float *templ, *tempr;
    templ = fleft->get_output();
    tempr = fright->get_output();
    dsp_copy(templ, outL, n);
    dsp_copy(tempr, outR, n);
    dsp_clip(outL, n, 1);
    dsp_clip(outR, n, 1);
  }
};

lunar_fx *new_fx() { return new filter(); }
