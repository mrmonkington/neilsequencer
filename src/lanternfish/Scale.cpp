#include "Scale.hpp"

namespace lanternfish {
  void Scale::set_min(float min) {
    this->min = min;
  }

  void Scale::set_max(float max) {
    this->max = max;
  }

  void Scale::process(int n) {
    if (this->out.size() != n) {
      this->out.resize(n);
    }
    for (int i = 0; i < n; i++) {
      this->out[i] = this->min + 
	(*this->in)[i] * (this->max - this->min);
    }
  }
}
