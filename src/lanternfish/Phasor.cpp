#include <cmath>
#include <cstdio>

#include "Phasor.hpp"

namespace lanternfish {
  Phasor::Phasor() 
  {
    this->phi = 0.0;
  }

  Phasor::~Phasor() 
  {

  }
  
  void Phasor::set_sampling_rate(int sampling_rate)
  {
    this->sampling_rate = sampling_rate;
  }

  void Phasor::process(float *freq, float *out, int n) 
  {
    float srate = this->sampling_rate;
    for (int i = 0; i < n; i++) {
      out[i] = this->phi;
      float dphi = freq[i] / srate;
      this->phi += dphi;
      while (this->phi >= 1.0) {
	this->phi -= 1.0;
      }
    }
  }
  
  float Phasor::process(float freq) 
  {
    float srate = sampling_rate;
    float out = phi;
    float dphi = freq / srate;
    phi += dphi;
    while (phi >= 1.0)
      phi -= 1.0;
    return out;
  }
}
