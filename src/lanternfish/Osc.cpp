#include "Osc.hpp"

namespace lanternfish {
  Osc::Osc() {
    phi = 0.0;
  }

  Osc::~Osc() {

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

  void process(int n) {
    float srate = float(this->sampling_rate);
    if (n != this->out.size()) {
      this->out.resize(n);
    }
    int tabsize = this->table->size();
    for (int i = 0; i < n; i++) {
      float f_index = this->phi * tabsize;
      float offset, intpart;
      offset = modf(f_index, &intpart);
      int i[4];
      i[1] = int(intpart);
      i[0] = i1 == 0 ? tabsize - 1 : i[1] - 1;
      i[2] = (i[1] + 1) % tabsize;
      i[3] = (i[2] + 1) % tabsize;
      this->out[i] = interpolate(this->table[i[0]],
				 this->table[i[1]],
				 this->table[i[2]],
				 this->table[i[3]],
				 offset);
    }
  }
}
