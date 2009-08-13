#include "filter.h"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>

class XYSvf {
private:
  float low, high, band, notch;
  float cutoff, x, y, dcutoff;
  float q;
  float sps;
  int inertia, counter;
public:
  XYSvf() {
    low = high = band = notch = 0.0;
    sps = 44100.0;
    cutoff = 1000.0;
  }

  ~XYSvf() {
    //~
  }

  void set_cutoff(float new_cutoff) {
    this->dcutoff = (new_cutoff - this->cutoff) / (float)inertia;
    this->counter = this->inertia;
  }

  void set_x(float x) {
    this->x = x;
  }

  void set_y(float y) {
    this->y = y;
  }

  void set_q(float reso) {
    this->q = sqrt(1.0 - atan(sqrt(reso * 100.0)) * 2.0 / M_PI) * 1.33 - 0.33;
  }

  void set_inertia(int inertia) {
    this->inertia = inertia;
  }

  void set_samples_per_second(float sps) {
    this->sps = sps;
  }

  void process(float *input, float *output, int n) {
    float scale, f, pair1, pair2;
    scale = sqrt(this->q);
    for (int i = 0; i < n; i++) {
      f = this->cutoff / this->sps * 2.0;
      for (int j = 0; j < 2; j++) {
	this->low = this->low + f * this->band;
	this->high = scale * input[i] - this->low - this->q * this->band;
	this->band = f * this->high + this->band;
	this->notch = this->high + this->low;
      }
      if (this->counter > 0) {
	this->cutoff += this->dcutoff;
	this->counter -= 1;
	if (this->cutoff > 20000.0)
	  this->cutoff = 20000.0;
	if (this->cutoff < 20.0)
	  this->cutoff = 20.0;
      }
      pair1 = this->low * this->y + this->high * (1.0 - this->y);
      pair2 = this->band * this->y + this->notch * (1.0 - this->y);
      output[i] = pair2 * this->x + pair1 * (1.0 - this->x);
    }
  }
};

class filter : public lunar::fx<filter> {
public:
  int inertia;
  XYSvf filter_l, filter_r;
    
  void init() {

  }
  
  void exit() {

  }

  void process_events() {
    this->filter_l.set_inertia(transport->samples_per_tick);
    this->filter_r.set_inertia(transport->samples_per_tick);
    if (globals->freq) {
      this->filter_l.set_cutoff(*globals->freq);
      this->filter_r.set_cutoff(*globals->freq);
    }
    if (globals->res) {
      this->filter_l.set_q(*globals->res);
      this->filter_r.set_q(*globals->res);
    }
    if (globals->x) {
      this->filter_l.set_x(*globals->x);
      this->filter_r.set_x(*globals->x);
    }
    if (globals->y) {
      this->filter_l.set_y(*globals->y);
      this->filter_r.set_y(*globals->y);
    }
  }
  
  void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {
    this->filter_l.set_samples_per_second(transport->samples_per_second);
    this->filter_r.set_samples_per_second(transport->samples_per_second);
    this->filter_l.process(inL, outL, n);
    this->filter_r.process(inR, outR, n);
    dsp_clip(outL, n, 1.0);
    dsp_clip(outR, n, 1.0);
  }
};

lunar_fx *new_fx() { return new filter(); }
