#include <cstdio>
#include <cmath>
#include <algorithm>

#include "Stutter.hpp"

using namespace std;

Stutter::Stutter() {
  global_values = &gval;
  attributes = 0;
  track_values = 0;
  record = false;
  cursor = 0;
  counter = 0;
}

Stutter::~Stutter() {
  delete this;
}

void Stutter::init(zzub::archive* pi) {
  buffer[0] = new float[_master_info->samples_per_tick * 
			(param_length->value_max / 4)]; 
  buffer[1] = new float[_master_info->samples_per_tick *
			(param_length->value_max / 4)];
  length = _master_info->samples_per_tick;
  tick_length = 1.0;
}

void Stutter::destroy() {
  delete[] buffer[0];
  delete[] buffer[1];
}

void Stutter::process_events() {
  if (gval.length != param_length->value_none) {
    length = (gval.length / 4.0) * _master_info->samples_per_tick;
    tick_length = gval.length / 4.0;
    cursor = 0;
  }
  if (gval.time != param_length->value_none) {
    counter = gval.time / tick_length;
    cursor = 0;
    record = true;
  }
}

bool Stutter::process_stereo(float **pin, float **pout, int n, int mode) {
  for (int i = 0; i < n; i++) {
    if (counter == 0) {
      pout[0][i] = pin[0][i];
      pout[1][i] = pin[0][i];
    } else {
      if (record) {
	buffer[0][cursor] = pin[0][i];
	buffer[1][cursor] = pin[1][i];
	pout[0][i] = pin[0][i];
	pout[1][i] = pin[1][i];
	cursor += 1;
	if (cursor >= length) {
	  cursor = 0;
	  record = false;
	}
      } else {
	pout[0][i] = buffer[0][cursor];
	pout[1][i] = buffer[1][cursor];
	cursor += 1;
	if (cursor >= length) {
	  counter -= 1;
	  cursor = 0;
	}
      }
    }
  }
  return true;
}

const char *Stutter::describe_value(int param, int value) 
{
  static char str[20];
  if (param == 0) {
    sprintf(str, "%d sticks", value);
    return str;
  }
  else if (param == 1) {
    sprintf(str, "%d ticks", value);
    return str;
  } else {
    return 0;
  }
}



