#include <cmath>

#include "Svf.hpp"

namespace lanternfish {
  Svf::Svf() {
    reset();
    this->out_low = new float[256];
    this->out_high = new float[256];
    this->out_band = new float[256];
    this->out_notch = new float[256];
    this->buff_size = 256;
  }
  
  Svf::~Svf() {
    delete[] this->out_low;
    delete[] this->out_high;
    delete[] this->out_band;
    delete[] this->out_notch;
  }
  
  void Svf::reset() {
    bypass = false;
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
    if (this->buff_size < n) {
      delete[] this->out_low;
      delete[] this->out_high;
      delete[] this->out_band;
      delete[] this->out_notch;
      this->out_low = new float[n];
      this->out_high = new float[n];
      this->out_band = new float[n];
      this->out_notch = new float[n];
      this->buff_size = n;
    }
    if (!this->bypass) {
      float scale, f;
      scale = sqrt(q);
      for (int i = 0; i < n; i++) {
	f = this->cutoff[i] / this->sps * 2.0;
	for (int j = 0; j < 2; j++) {
	  low = low + f * band;
	  high = scale * this->in[i] - low - q * band;
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
	this->out_low[i] = this->in[i];
	this->out_high[i] = this->in[i];
	this->out_band[i] = this->in[i];
	this->out_notch[i] = this->in[i];
      }
    }
  }
}
