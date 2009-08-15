#include <cmath>

#include "Svf.hpp"

namespace lanternfish {
  Svf::Svf() {
    reset();
    modes[0] = &low;
    modes[1] = &high;
    modes[2] = &band;
    modes[3] = &notch;
  }
  
  Svf::~Svf() {
    
  }
  
  void Svf::reset() {
    bypass = true;
    inertia = 1;
    counter = 0;
    cutoff = 2000.0;
    sps = 44100.0;
    dcutoff = 0.0;
    low = high = band = notch = 0.0;
    mode = 0;
    q = 0.0;
  }
  
  void Svf::set_mode(FilterMode mode) {
    switch (mode) {
    case LOWPASS:
      this->mode = 0;
      break;
    case HIGHPASS:
      this->mode = 1;
      break;
    case BANDPASS:
      this->mode = 2;
      break;
    case NOTCH:
      this->mode = 3;
      break;
    }
  }
  
  void Svf::set_bypass(bool on) {
    this->bypass = on;
  }
  
  void Svf::set_sampling_rate(int rate) {
    this->sps = (float)rate;
  }
  
  void Svf::set_resonance(float reso) {
    this->q = sqrt(1.0 - atan(sqrt(reso * 100.0)) * 2.0 / M_PI) * 1.33 - 0.33;
  }
  
  void Svf::set_inertia(int inertia) {
    this->inertia = inertia;
  }
  
  void Svf::set_cutoff(float new_cutoff) {
    this->dcutoff = (new_cutoff - this->cutoff) / (float)inertia;
    this->counter = this->inertia;
  }
  
  void Svf::process(float *input, float *output, int n) {
    if (!this->bypass) {
      float scale, f;
      scale = sqrt(q);
      for (int i = 0; i < n; i++) {
	f = this->cutoff / this->sps * 2.0;
	for (int j = 0; j < 2; j++) {
	  low = low + f * band;
	  high = scale * input[i] - low - q * band;
	  band = f * high + band;
	  notch = high + low;
	}
	if (this->counter > 0) {
	  this->cutoff += this->dcutoff;
	  this->counter -= 1;
	  if (this->cutoff > 20000.0)
	    this->cutoff = 20000.0;
	  if (this->cutoff < 20.0)
	    this->cutoff = 20.0;
	}
	output[i] = *modes[mode];
      }
    } else {
      for (int i = 0; i < n; i++) {
	output[i] = input[i];
      }
    }
  }
  
  void Svf::process(float *input, float *cutoff, float *output, int n) {
    if (!this->bypass) {
      float scale, f;
      scale = sqrt(q);
      for (int i = 0; i < n; i++) {
	f = cutoff[i] / this->sps * 2.0;
	for (int j = 0; j < 2; j++) {
	  low = low + f * band;
	  high = scale * input[i] - low - q * band;
	  band = f * high + band;
	  notch = high + low;
	}
	output[i] = *modes[mode];
      }
    } else {
      for (int i = 0; i < n; i++) {
	output[i] = input[i];
      }
    }
  }
}
