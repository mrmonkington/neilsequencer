#include <cmath>

#include "Svf.hpp"

namespace lanternfish {
  Svf::Svf() {
    reset();
  }
  
  Svf::~Svf() {
    
  }
  
  void Svf::reset() {
    bypass = true;
    sps = 44100.0;
    low = high = band = notch = 0.0;
    q = 0.0;
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
  
  void Svf::process(int n) {
    if (this->out_low.size() != n) {
      this->out_low.resize(n);
      this->out_high.resize(n);
      this->out_band.resize(n);
      this->out_notch.resize(n);
    }
    if (!this->bypass) {
      float scale, f;
      scale = sqrt(q);
      for (int i = 0; i < n; i++) {
	f = (*this->cutoff)[i] / this->sps * 2.0;
	for (int j = 0; j < 2; j++) {
	  low = low + f * band;
	  high = scale * (*this->in)[i] - low - q * band;
	  band = f * high + band;
	  notch = high + low;
	}
	this->out_low[i] = low;
	this->out_high[i] = high;
	this->out_band[i] = band;
	this->out_notch[i] = notch;
      }
    } else {
      for (int i = 0; i < n; i++) {
	this->out_low[i] = (*this->in)[i];
	this->out_high[i] = (*this->in)[i];
	this->out_band[i] = (*this->in)[i];
	this->out_notch[i] = (*this->in)[i];
      }
    }
  }
}
