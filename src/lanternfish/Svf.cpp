#include <cmath>

#include "Svf.hpp"

namespace lanternfish {
  Svf::Svf() 
  {
    reset();
  }
  
  Svf::~Svf() 
  {

  }
  
  void Svf::reset() 
  {
    bypass = false;
    sps = 44100.0;
    low = high = band = notch = 0.0;
    q = 0.0;
  }
  
  void Svf::set_bypass(bool on) 
  {
    this->bypass = on;
  }
  
  void Svf::set_sampling_rate(int rate) 
  {
    this->sps = (float)rate;
  }
  
  void Svf::set_resonance(float reso) 
  {
    this->q = sqrt(1.0 - atan(sqrt(reso * 100.0)) * 2.0 / M_PI) * 1.33 - 0.33;
  }
  
  void Svf::process(float *out_low, float *out_high, float *out_band,
	       float *out_notch, float *cutoff, float *in, int n) 
  {
    if (!this->bypass) {
      float scale, f;
      scale = sqrt(q);
      for (int i = 0; i < n; i++) {
	f = cutoff[i] / this->sps * 2.0;
	for (int j = 0; j < 2; j++) {
	  low = low + f * band;
	  high = scale * in[i] - low - q * band;
	  band = f * high + band;
	  notch = high + low;
	}
	out_low[i] = low;
	out_high[i] = high;
	out_band[i] = band;
	out_notch[i] = notch;
      }
    } else {
      for (int i = 0; i < n; i++) {
	out_low[i] = in[i];
	out_high[i] = in[i];
	out_band[i] = in[i];
	out_notch[i] = in[i];
      }
    }
  }
}
