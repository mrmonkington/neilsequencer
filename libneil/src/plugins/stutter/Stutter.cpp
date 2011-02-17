#include <cstdio>
#include <cmath>
#include <algorithm>

#include "Stutter.hpp"

#define MAX_SMOOTH 1000

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
  if (gval.smoothing != param_smoothing->value_none) {
    smoothing = gval.smoothing / float(param_smoothing->value_max);
  }
}

float Stutter::envelope(int cursor, int length) {
  if (cursor < (MAX_SMOOTH * smoothing)) {
    return cursor / (MAX_SMOOTH * smoothing);
  } else if (cursor > (length - MAX_SMOOTH * smoothing)) {
    return (length - cursor) / (MAX_SMOOTH * smoothing);
  } else {
    return 1.0;
  }
}

bool Stutter::process_stereo(float **pin, float **pout, int n, int mode) {
  for (int i = 0; i < n; i++) {
    if (counter == 0) {
      float env;
      if (cursor < (MAX_SMOOTH * smoothing)) {
	env = envelope(cursor, length);
	cursor += 1;
      } else {
	env = 1.0;
      }
      pout[0][i] = pin[0][i] * env;
      pout[1][i] = pin[1][i] * env;
    } else {
      if (record) {
	buffer[0][cursor] = pin[0][i];
	buffer[1][cursor] = pin[1][i];
	float env;
	if (cursor < MAX_SMOOTH * smoothing) {
	  env = 1.0;
	} else {
	  env = envelope(cursor, length);
	}
	pout[0][i] = pin[0][i] * env;
	pout[1][i] = pin[1][i] * env;
	cursor += 1;
	if (cursor >= length) {
	  cursor = 0;
	  record = false;
	}
      } else {
	float env = envelope(cursor, length);
	pout[0][i] = buffer[0][cursor] * env;
	pout[1][i] = buffer[1][cursor] * env;
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
  }
  else if (param == 2) {
    sprintf(str, "%d%%", int((value / 255.0) * 100));
    return str;
  }
  else {
    return 0;
  }
}



