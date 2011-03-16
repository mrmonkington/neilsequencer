#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <algorithm>

#include "Controller.hpp"

LunarController::LunarController() {
  global_values = 0;
  track_values = &tval;
  controller_values = &cval;
  attributes = 0;
}

void LunarController::init(zzub::archive* pi) {
  for (int i = 0; i < 8; i++) {
    value[i] = 0;
    power[i] = 500;
    min[i] = 0;
    max[i] = 1000;
  }
}

void LunarController::process_events() {
  for (int i = 0; i < 8; i++) {
    if (tval[i].value != para_value->value_none) {
      value[i] = tval[i].value;
    }
    if (tval[i].power != para_power->value_none) {
      power[i] = tval[i].power;
    }
    if (tval[i].minimum != para_minimum->value_none) {
      min[i] = tval[i].minimum;
    }
    if (tval[i].maximum != para_maximum->value_none) {
      max[i] = tval[i].maximum;
    }
  }
}

void LunarController::process_controller_events() {
  for (int i = 0; i < 8; i++) {
    float fvalue = value[i] * 0.001f;
    float fpower = power[i] * 0.01f - 5.0f;
    fpower = fpower > 0.0 ? (fpower + 1.0) : 1.0 / (-fpower + 1.0);
    fvalue = pow(fvalue, fpower);
    float fmin = min[i] * 0.001f;
    float fmax = max[i] * 0.001f;
    float scale = fmin + fvalue * (fmax - fmin);
    cval[i] = int(para_output[i]->value_min + 
		  (para_output[i]->value_max - para_output[i]->value_max) * 
		  scale);
    printf("%d\n", cval[i]);
  }
}

bool LunarController::process_stereo(float **pin, float **pout, int n, int mode) {
  return false;
}

const char *LunarController::describe_value(int param, int value) {
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
