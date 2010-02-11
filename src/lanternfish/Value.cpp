#include "Value.hpp"

namespace lanternfish {
  Value::Value() {
    this->val = 0.0;
    this->dval = 0.0;
    this->counter = 0;
    this->out = new float[256];
    this->buff_size = 256;
  }

  Value::~Value() {

  }

  void Value::set_value(float val, int inertia) {
    this->counter = inertia;
    this->dval = (val - this->val) / float(inertia);
  }

  void Value::process(int n) {
    if (this->buff_size < n) {
      delete[] this->out;
      this->out = new float[n];
      this->buff_size = n;
    }
    for (int i = 0; i < n; i++) {
      if (this->counter == 0) {
	this->out[i] = this->val;
      } else {
	this->out[i] = this->val;
	this->val += this->dval;
	this->counter--;
      }
    }
  }
}
