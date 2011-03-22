#include <algorithm>
#include <cstdio>
#include <cmath>

#include "Phaser.hpp"

LunarPhaser::LunarPhaser() {
  global_values = &gval;
  attributes = 0;
  track_values = 0;
}

void LunarPhaser::init(zzub::archive *pi) {
  // wavetable for lfo sine
  for (int i = 0; i < WAVESIZE; i++) {
    float t = (float) i / (float) WAVESIZE;
    wavetable[i] = (float)sin(2 * M_PI * t);
  }
}
	
void LunarPhaser::process_events() {
  phaser_left.swirl = 0;
  phaser_right.swirl = 20;
  phaser_left.feedback = phaser_left.init_feedback;
  phaser_right.feedback = phaser_right.init_feedback;
  if (gval.feedback != 0xff) {
    phaser_left.feedback = gval.feedback * 0.01f;
    phaser_right.feedback = gval.feedback * 0.01f;
    phaser_left.init_feedback = phaser_left.feedback;
    phaser_right.init_feedback = phaser_right.feedback;
  }
  if (gval.drywet != 0xff) {
    phaser_left.drywet = gval.drywet * 0.01f;
    phaser_left.n_drywet = 1.0 - phaser_left.drywet;
    phaser_right.drywet = gval.drywet * 0.01f;
    phaser_right.n_drywet = 1.0 - phaser_right.drywet;	
  }
  if (gval.lfo_min != 0xffff) {
    float fval = gval.lfo_min / float(0xfffe);
    fval = 20.0f + fval * fval * 19980.0f;
    lfo_min = fval / _master_info->samples_per_second;
  }
  if (gval.lfo_max != 0xffff) {
    float fval = gval.lfo_max / float(0xfffe);
    fval = 20.0f + fval * fval * 19980.0f;
    lfo_max =  fval / _master_info->samples_per_second;
  }
  if (gval.lfo_rate != 0xffff) {
    float fval = gval.lfo_rate / float(0xfffe);
    fval = 0.01f + fval * fval * fval * fval * 1999.99f;
    lfo_increment = WAVESIZE * (fval / _master_info->samples_per_second);
  }
  if (gval.stages != 0xff) {
    phaser_left.stages = gval.stages;
    phaser_right.stages = gval.stages;
    phaser_left.reset();
    phaser_right.reset();
  }
  if (gval.lfo_phase != 0xff) {
    phaser_left.lfo_phase = gval.lfo_phase * 0.01f * WAVESIZE;
    phaser_right.lfo_phase = gval.lfo_phase * 0.01f * WAVESIZE;
  }
}

void LunarPhaser::process(float *buffer, int size, phaser_state *phaser) {
  float in, out, delay;
  while (size--) {
    delay = lfo_min + (lfo_max - lfo_min) *  
      (1.0 + get_interpolated_sample(wavetable, phaser->lfo_phase, WAVEMASK)) / 2;
    phaser->set_delay(delay);
    phaser->lfo_phase = float_mask(phaser->lfo_phase + lfo_increment, WAVEMASK);
    in = *buffer;
    out = phaser->tick(in);
    *buffer++ = out;
  }
}

bool LunarPhaser::process_stereo(float **pin, float **pout, int n, int mode) {
  for (int i = 0; i < n; i++) {
    pout[0][i] = pin[0][i];
    pout[1][i] = pin[1][i];
  }
  process(pout[0], n, &phaser_left);
  process(pout[1], n, &phaser_right);
  return true;
}

const char *LunarPhaser::describe_value(int param, int value) {
  static char txt[20];
  float fval;
  switch (param) {
  case 0:
  case 1:
  case 5:
    sprintf(txt, "%.2f", value * 0.01f);
    break;
  case 2:
  case 3:
    fval = value / float(0xfffe);
    sprintf(txt, "%.2f Hz", 20.0f + fval * fval * 19980.0f);
    break;
  case 4:
    fval = value / float(0xfffe);
    sprintf(txt, "%.2f Hz", 0.01f + fval * fval * fval * fval * 1999.99f);
    break;
  default:
    return 0;
  }
  return txt;
}
