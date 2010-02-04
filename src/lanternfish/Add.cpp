#include "Add.hpp"

namespace lanternfish {
  void Add::process(int n) {
    if (this->out.size() != n) {
      this->out.resize(n);
    }
    for (int i = 0; i < n; i++) {
      this->out[i] = (*this->in1)[i] + (*this->in2)[i];
    }
  }
}
