#include "Add.hpp"

namespace lanternfish {
  Add::Add() {
    this->out = new float[256];
    this->buff_size = 256;
  }

  Add::~Add() {
    delete[] this->out;
  }

  void Add::process(int n) {
    if (this->buff_size < n) {
      delete[] this->out;
      this->out = new float[n];
      this->buff_size = n;
    }
    for (int i = 0; i < n; i++) {
      this->out[i] = this->in1[i] + this->in2[i];
    }
  }
}
