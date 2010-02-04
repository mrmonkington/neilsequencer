#include <cstdlib>
#include <cmath>

#include "Voice.hpp"

Voice::Voice() {
  scale.set_min(0.0);
  scale.set_max(1.0);
  osc.freq = &freq.out;
  osc.table = &table;
  scale.in = &env.out;
  mul1.in1 = &cutoff.out;
  mul1.in2 = &scale.out;
  filter.in = &osc.out;
  filter.cutoff = &mul1.out;
  mul2.in1 = &filter.out_low;
  mul2.in2 = &env.out;
  samples = 0;
  glide = 0;
  table.resize(64);
  phi = 0.0;
}

void Voice::note_on(int note) {
  if (samples) {
    float f = 440.0 * pow(2, (note - 69) / 12.0);
    freq.set_value(f, this->glide);
    int offset = rand() % samples->size();
    for (int i = 0; i < table.size(); i++) {
      //table[i] = (*samples)[(offset + i) % samples->size()];
      table[i] = sin(2.0 * M_PI * (i / float(table.size())));
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
  scale.set_min(1.0 - env_amount);
}

void Voice::set_tabsize(int tabsize) {
  //table.clear();
  //table.resize(tabsize);
}

void Voice::set_filter_mode(int mode) {
  switch (mode) {
  case 0:
    mul2.in1 = &filter.out_low;
    break;
  case 1:
    mul2.in1 = &filter.out_high;
    break;
  case 2:
    mul2.in1 = &filter.out_band;
    break;
  case 3:
    mul2.in1 = &filter.out_notch;
    break;
  }
}

void Voice::set_volume(float vol) {
  this->vol = vol;
}

void Voice::process(float *out_l, float *out_r, int n) {
  freq.process(n);
  //cutoff.process(n);
  osc.process(n, out_l);
  //env.process(n);
  //scale.process(n);
  //mul1.process(n);
  //filter.process(n);
  //mul2.process(n);
  for (int i = 0; i < n; i++) {
    //out_l[i] = osc.out[i] * this->vol;
    //out_r[i] = osc.out[i] * this->vol;
    /*
    float dphi = 440.0 / 44100.0;
    float sample = sin(2.0 * M_PI * phi);
    out_l[i] = sample;
    out_r[i] = sample;
    phi += dphi;
    while (phi > 1.0)
      phi = phi - 1.0;
    */
  }
}
