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
    this->q = reso;
  }
  
  void Svf::process(float *out, float *cutoff, float *input, int mode, int n) 
  {
    for (int i = 0; i < n; i++)
      out[i] = 0.0;
    if (!this->bypass) {
      for (int i = 0; i < n; i++) {
	float freq = 2.0f * sin(M_PI * 
				std::min(0.25f, cutoff[i] / (sps * 2.0f)));
	float damp = std::min(2.0f * (1.0f - (float)sqrt(sqrt(q))), 
			      std::min(2.0f, 2.0f / freq  - freq * 0.5f));
	float in = input[i];
	for (int j = 0; j < 2; j++) {
	  notch = in - damp * band;
	  low = low + freq * band;
	  high = notch - low;
	  band = freq * high + band;
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
	}
      }
    } else {
      for (int i = 0; i < n; i++) {
	out[i] = input[i];
      }
    }
  }
}
