#include <cmath>
#include <cstdio>
#include <cassert>

#include "Lag.hpp"

namespace lanternfish {
  Lag::Lag() 
  {
    this->value = 1.0;
    this->delta = 0.0;
    this->counter = 0;
  }

  Lag::~Lag() 
  {

  }
  
  void Lag::set_value(float target, int lag)
  {
    assert(lag > 0);
    if (this->counter == 0) {
      this->delta = (target - this->value) / float(lag);
      this->counter = lag;
    }
  }

  void Lag::process(float *out, int n) 
  {
    for (int i = 0; i < n; i++) {
      if (counter > 0) {
	out[i] = this->value;
	this->value += this->delta;
	this->counter -= 1;
      } else {
	out[i] = this->value;
      }
    }
  }
}
