#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <algorithm>
#include <stdint.h>

#include "Noise.hpp"

float Noise::gauss(float mean, float var) {
  float u1, u2, v1, v2, s, x, y;
  do {
    u1 = rand() / float(RAND_MAX);
    u2 = rand() / float(RAND_MAX);
    v1 = 2.0 * u1 - 1.0;
    v2 = 2.0 * u2 - 1.0;
    s = v1 * v1 + v2 * v2;
  } while (s >= 1.0);
  x = sqrt(-2.0 * log(s) / s) * v1;
  y = sqrt(-2.0 * log(s) / s) * v2;
  float xp = mean + sqrt(var) * x;
  float yp = mean + sqrt(var) * y;
  return xp;
}

Noise::Noise() {
  global_values = &gval;
  controller_values = &cval;
  attributes = 0;
  track_values = 0;
}

Noise::~Noise() {

}

void Noise::init(zzub::archive* pi) {

}

void Noise::destroy() {

}

void Noise::process_events() {
  if (gval.mean != param_mean->value_none) {
    this->mean = gval.mean / float(0xFFFE);
  }
  if (gval.variance != param_variance->value_none) {
    this->variance = pow(gval.variance / float(0xFFFE), 2.0) * 0.05;
  }
  if (gval.note != zzub::note_value_none) {
    if (gval.note == zzub::note_value_off) {
      this->note = 0;
    } else {
      this->note = 1;
    }
  } else {
    this->note = -1;
  }
}

void Noise::process_controller_events() {
  if (this->note == 0) {
    cval.out = zzub::note_value_off;
  } else if (this->note == 1) {
    float grand = gauss(this->mean, this->variance);
    if (grand > 1.0)
      grand = 1.0;
    if (grand < 0.0)
      grand = 0.0;
    uint8_t new_note = zzub::note_value_min + grand * zzub::note_value_max;
    cval.out = new_note;
  } else if (this->note == -1) {
    // pass
  }
}

bool Noise::process_stereo(float **pin, float **pout, int n, int mode) {
  return false;
}

const char *Noise::describe_value(int param, int value) {
  static char txt[20];
  if (param == 0) {
    sprintf(txt, "%.3f", value / float(0xFFFE));
    return txt;
  } if (param == 1) {
    sprintf(txt, "%.3f", pow(value / float(0xFFFE), 2.0) * 0.05);
    return txt;
  } else {
    return 0;
  }
}



