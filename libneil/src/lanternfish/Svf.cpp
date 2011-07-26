#include <cmath>
#include <algorithm>

#include "Svf.hpp"
#include "Utils.hpp"

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
    q = sqrt(1.0 - atan(sqrt(reso * 100.0)) * 2.0 / M_PI) * 1.33 - 0.33;
  }

  inline void Svf::kill_denormal(float &val) {
    if (fabs(val) < 1e-15)
      val = 0.0;
  }
  
  void Svf::process(float *out, float *cutoff, float *input, int mode, int n) 
  {
    for (int i = 0; i < n; i++)
      out[i] = 0.0;
    if (!this->bypass) {
      for (int i = 0; i < n; i++) {
	float scale, f;
	scale = sqrt(q);
	for (int j = 0; j < 2; j++) {
	  f = cutoff[i] / sps * 2.0;
	  low = low + f * band;
	  high = scale * input[i] - low - q * band;
	  band = f * high + band;
	  notch = high + low;
	  switch(mode) {
	  case 0:
	    out[i] += 0.5 * low;
	    break;
	  case 1:
	    out[i] += 0.5 * high;
	    break;
	  case 2:
	    out[i] += 0.5 * band;
	    break;
	  case 3:
	    out[i] += 0.5 * notch;
	    break;
	  case 4:
	    out[i] += 0.5 * (low - high);
	    break;
	  }
	  kill_denormal(low);
	  kill_denormal(high);
	  kill_denormal(band);
	  kill_denormal(notch);
	}
      }
    } else {
      for (int i = 0; i < n; i++) {
	out[i] = input[i];
      }
    }
  }
}
