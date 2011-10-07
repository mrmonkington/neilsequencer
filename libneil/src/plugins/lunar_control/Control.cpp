#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <algorithm>

#include "Control.hpp"

LunarControl::LunarControl() {
  global_values = &gval;
  track_values = 0;
  controller_values = &cval;
  attributes = 0;
}

void LunarControl::init(zzub::archive* pi) {
  value = 0;
  power = 500;
  min = 0;
  max = 1000;
}

void LunarControl::process_events() {
  if (gval.value != para_value->value_none) {
    value = gval.value;
  }
  if (gval.power != para_power->value_none) {
    power = gval.power;
  }
  if (gval.minimum != para_minimum->value_none) {
    min = gval.minimum;
  }
  if (gval.maximum != para_maximum->value_none) {
    max = gval.maximum;
  }
}

void LunarControl::process_controller_events() {
  float fvalue = value * 0.001f;
  float fpower = power * 0.01f - 5.0f;
  fpower = fpower > 0.0 ? (fpower + 1.0) : 1.0 / (-fpower + 1.0);
  fvalue = pow(fvalue, fpower);
  float fmin = min * 0.001f;
  float fmax = max * 0.001f;
  float scale = fmin + fvalue * (fmax - fmin);
  cval = int(para_output->value_min + 
	     (para_output->value_max - para_output->value_min) * 
	     scale);
}

bool LunarControl::process_stereo(float **pin, float **pout, int n, int mode) {
  return false;
}

const char *LunarControl::describe_value(int param, int value) {
  static char txt[20];
  switch (param) {
  case 0:
  case 2:
  case 3:
    sprintf(txt, "%.2f", value * 0.001f);
    break;
  case 1:
    sprintf(txt, "%.2f", value * 0.01f - 5.0f);
    break;
  default:
    return 0;
  }
  return txt;
}
