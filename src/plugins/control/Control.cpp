#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <algorithm>

#include "Control.hpp"

Control::Control() {
  global_values = &gval;
  controller_values = &cval;
  attributes = NULL;
  track_values = NULL;
  for (int i = 0; i < 4 * 8; i += 4) {
    value[i / 4] = gparams[i]->value_default;
    power[i / 4] = gparams[i + 1]->value_default;
    min[i / 4] = gparams[i + 2]->value_default;
    max[i / 4] = gparams[i + 3]->value_default;
  }
}

Control::~Control() {

}

void Control::init(zzub::archive* pi) {

}

void Control::destroy() {

}

void Control::process_events() {
  for (int i = 0; i < 4 * 8; i += 4) {
    if (gval[i] != 0xFFFF) {
      value[i / 4] = gval[i];
    } else if (gval[i + 1] != 0xFFFF) {
      power[i / 4] = gval[i + 1];
    } else if (gval[i + 2] != 0xFFFF) {
      min[i / 4] = gval[i + 2];
    } else if (gval[i + 3] != 0xFFFF) {
      max[i / 4] = gval[i + 3];
    }
  }
}

void Control::process_controller_events() {
  for (int i = 0; i < 8; i++) {
    float fvalue = float(value[i]) / 0xFFFE;
    float fpower = float(power[i]) / 0xFFFE * 10.0 - 5.0;
    fpower = fpower > 0.0 ? (fpower + 1.0) : 1.0 / (-fpower + 1.0);
    float fmin = float(min[i]) / 0xFFFE;
    float fmax = float(max[i]) / 0xFFFE;
    float scale = fmin + fvalue * (fmax - fmin);
    cval[i] = int(cparams[i]->value_max * pow(scale, fpower));
  }
}

bool Control::process_stereo(float **pin, float **pout, int n, int mode) {
  return false;
}

const char *Control::describe_value(int param, int value) {
  static char txt[20];
  if (param % 4 == 0) {
    sprintf(txt, "%.3f", value / float(0xFFFE));
  } else if (param % 4 == 1) {
    sprintf(txt, "%.3f", value / float(0xFFFE) * 10.0 - 5.0);
  } else if (param % 4 == 2 || param % 4 == 3) {
    sprintf(txt, "%.3f", value / float(0xFFFE));
  } else {
    return 0;
  }
  return txt;
}



