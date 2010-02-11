#include <cstdio>
#include <cstdlib>
#include <cmath>

#include "Voice.hpp"

Voice::Voice() {
  osc.freq = freq.out;
  osc.table = &table;
  filter.in = osc.out;
  samples = 0;
  glide = 1;
  table.resize(64);
  phi = 0.0;
  env_mod_min = 1.0;
  mode = 0;
}

void Voice::note_on(int note) {
  if (samples && (samples->size() != 0)) {
    float f = 440.0 * pow(2, (note - 69) / 12.0);
    freq.set_value(f, this->glide);
    int offset = rand() % samples->size();
    for (int i = 0; i < table.size(); i++) {
      table[i] = (*samples)[(offset + i) % samples->size()];
    }
    env.note_on();
  }
}

void Voice::note_off() {
  env.note_off();
}

void Voice::set_sampling_rate(int sampling_rate) {
  osc.sampling_rate = sampling_rate;
  filter.set_sampling_rate(sampling_rate);
}

void Voice::set_attack(int attack) {
  env.set_attack_time(attack);
}

void Voice::set_decay(int decay) {
  env.set_decay_time(decay);
}

void Voice::set_sustain(float sustain) {
  env.set_sustain_level(sustain);
}

void Voice::set_release(int release) {
  env.set_release_time(release);
}

void Voice::set_cutoff(int cutoff) {
  this->cutoff.set_value(float(cutoff), filter_inertia);
}

void Voice::set_resonance(float resonance) {
  filter.set_resonance(resonance);
}

void Voice::set_env_amount(float env_amount) {
  this->env_mod_min = 1.0 - env_amount;
}

void Voice::set_tabsize(int tabsize) {
  table.clear();
  table.resize(tabsize);
}

void Voice::set_filter_mode(int mode) {
  this->mode = mode;
}

void Voice::set_volume(float vol) {
  this->vol = vol;
}

void Voice::process(float *out_l, float *out_r, int n) {
  freq.process(n);
  cutoff.process(n);
  osc.process(n);
  env.process(n);
  float *fcutoff = new float[n];
  filter.cutoff = fcutoff;
  for (int i = 0; i < n; i++) {
    fcutoff[i] = cutoff.out[i] * 
      (env_mod_min + (env.out[i] * (1.0 - env_mod_min)));
  }
  filter.process(n);
  float *filter_out;
  switch (mode) {
  case 0:
    filter_out = filter.out_low;
    break;
  case 1:
    filter_out = filter.out_high;
    break;
  case 2:
    filter_out = filter.out_band;
    break;
  case 3:
    filter_out = filter.out_notch;
    break;
  }
  for (int i = 0; i < n; i++) {
    sample = filter_out[i] * env.out[i];
    out_l[i] += sample * this->vol;
    out_r[i] += sample * this->vol;
  }
  delete[] fcutoff;
}
