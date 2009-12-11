#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <algorithm>

#include "LFNoise.hpp"

LFNoise::LFNoise() {
  global_values = &gval;
  controller_values = &cval;
  attributes = NULL;
  track_values = NULL;
  rate = 1;
  counter = 1;
  for (int i = 0; i < 4; i++) {
    buffer[i] = float(rand()) / RAND_MAX;
  }
}

LFNoise::~LFNoise() {

}

void LFNoise::init(zzub::archive* pi) {

}

void LFNoise::destroy() {

}

void LFNoise::process_events() {
  if (gval.rate != param_rate->value_none) {
    this->rate = gval.rate;
  }
  if (gval.interpolation != param_interpolation->value_none) {
    this->interpolation = gval.interpolation;
  }
  if (gval.min != param_min->value_none) {
    this->min = float(gval.min) / param_min->value_max;
  }
  if (gval.max != param_max->value_none) {
    this->max = float(gval.max) / param_max->value_max;
  }
}

float LFNoise::interpolate(float phi) {
  float out;
  switch(interpolation) {
  case 0:
    out = buffer[1];
  case 1:
    out = buffer[1] + (buffer[2] - buffer[1]) * phi;
  case 2:
    float a0, a1, a2, a3, phi2;
    phi2 = phi * phi;
    a0 = buffer[3] - buffer[2] - buffer[0] + buffer[1];
    a1 = buffer[0] - buffer[1] - a0;
    a2 = buffer[2] - buffer[0];
    a3 = buffer[1];
    out = a0 * phi * phi2 + a1 * phi2 + a2 * phi + a3;
  }
  out = std::min(out, 1.0f);
  out = std::max(out, 0.0f);
  return out;
}

void LFNoise::process_controller_events() {
  if (!this->counter) {
    float random_value = float(rand()) / RAND_MAX;
    float phi = float(this->rate - this->counter) / this->rate;
    this->cval.out = int(interpolate(phi) * param_out->value_max);
    random_value = this->min + random_value * (this->max - this->min);
    // Shift the random value buffer left
    for (int i = 0; i < 3; i++) {
      this->buffer[i] = this->buffer[i + 1];
    }
    // Set the right-most value to the newly generated random float
    this->buffer[3] = random_value;
    this->counter = this->rate;
  } else {
    float phi = float(this->rate - this->counter) / this->rate;
    this->cval.out = int(interpolate(phi) * param_out->value_max);
    this->counter--;
  }
}

bool LFNoise::process_stereo(float **pin, float **pout, int n, int mode) {
  
}

const char *LFNoise::describe_value(int param, int value) {
  static const int LABEL_SIZE = 20;
  static char str[LABEL_SIZE];
  switch (param) {
  case 0:
    if (value == 1) {
      sprintf(str, "1 row");
    } else {
      sprintf(str, "%d rows", value);
    }
    break;
  case 1:
    switch(interpolation) {
    case 0:
      sprintf(str, "None");
      break;
    case 1:
      sprintf(str, "Linear");
      break;
    case 2:
      sprintf(str, "Cubic");
      break;
    }
    break;
  case 2:
    sprintf(str, "%.4f", float(value) / param_min->value_max);
    break;
  case 3:
    sprintf(str, "%.4f", float(value) / param_max->value_max);
    break;
  }
  return str;
}



