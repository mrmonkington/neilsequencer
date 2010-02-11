#include <cmath>
#include <cstdio>

#include "Osc.hpp"

namespace lanternfish {
  Osc::Osc() {
    phi = 0.0;
    this->out = new float[256];
    this->buff_size = 256;
  }

  Osc::~Osc() {
    delete[] this->out;
  }

  float Osc::interpolate(float x0, float x1, float x2, float x3, float phi) {
    float a0, a1, a2, a3, phi2;
    phi2 = phi * phi;
    a0 = x3 - x2 - x0 + x1;
    a1 = x0 - x1 - a0;
    a2 = x2 - x0;
    a3 = x1;
    return (a0 * phi * phi2 + a1 * phi2 + a2 * phi + a3);
  }

  void Osc::process(int n) {
    float srate = float(this->sampling_rate);
    if (this->buff_size < n) {
      delete[] this->out;
      this->out = new float[n];
      this->buff_size = n;
    }
    int tabsize = this->table->size();
    for (int i = 0; i < n; i++) {
      float f_index = this->phi * tabsize;
      float offset, intpart;
      intpart = floor(f_index);
      offset = f_index - intpart;
      int i0, i1, i2, i3;
      i1 = int(intpart);
      i0 = i1 == 0 ? tabsize - 1 : i1 - 1;
      i2 = (i1 + 1) % tabsize;
      i3 = (i2 + 1) % tabsize;
      out[i] = interpolate((*table)[i0],
			   (*table)[i1],
			   (*table)[i2],
			   (*table)[i3],
			   offset);
      //printf("%.2f\n", this->freq[i]);
      float dphi = this->freq[i] / srate;
      //float dphi = 440.0 / srate;
      this->phi += dphi;
      while (this->phi > 1.0)
	this->phi = this->phi - 1.0;
    }
  }
}
