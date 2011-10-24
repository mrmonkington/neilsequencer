#include <cstdio>
#include <cmath>
#include <algorithm>

#include "Stutter.hpp"

#define MAX_SMOOTH 100

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
			(param_length->value_max / 16)]; 
  buffer[1] = new float[_master_info->samples_per_tick *
			(param_length->value_max / 16)];
}

void Stutter::destroy() {
  delete[] buffer[0];
  delete[] buffer[1];
}

void Stutter::process_events() {
  if (gval.length != param_length->value_none) {
    length = (_master_info->samples_per_tick / 16.0) * gval.length;
    counter = 0;
  }
  if (gval.time != param_time->value_none) {
    counter = (gval.time * _master_info->samples_per_tick) / length;
    time = gval.time;
    cursor = 0;
    record = true;
  }
  if (gval.smoothing != param_smoothing->value_none) {
    smoothing = gval.smoothing / float(param_smoothing->value_max);
  }
}

float Stutter::envelope(int cursor, int length) {
  int smooth_samples = MAX_SMOOTH * smoothing;
  if (length < 2 * smooth_samples) {
    return 1.0;
  }
  if (cursor < smooth_samples) {
    return cursor / smooth_samples;
  } else if (cursor > length - smooth_samples) {
    return (length - cursor) / smooth_samples;
  } else {
    return 1.0;
  }
}

bool Stutter::process_stereo(float **pin, float **pout, int n, int mode) {
  for (int i = 0; i < n; i++) {
    if (record) {
      // Record mode
      // Memorize the data
      buffer[0][cursor] = pin[0][i];
      buffer[1][cursor] = pin[1][i];
      // Apply the envelope to output
      float env = envelope(cursor, length);
      pout[0][i] = pin[0][i] * env;
      pout[1][i] = pin[1][i] * env;
      if (cursor++ >= length) {
	cursor = 0;
	// Turn off recording
	record = false;
      }
    } else if (counter > 0) {
      // Playback mode
      float env = envelope(cursor, length);
      // Playback memorized output
      pout[0][i] = buffer[0][cursor] * env;
      pout[1][i] = buffer[1][cursor] * env;
      if (cursor++ >= length) {
	counter--;
	cursor = 0;
      }
    } else {
      // Bypass mode, if nothing needs to be done
      pout[0][i] = pin[0][i];
      pout[1][i] = pin[1][i];
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



