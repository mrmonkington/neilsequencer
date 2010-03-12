#include <cmath>
#include <cstdio>
#include <algorithm>
#include "Grain.hpp"

Grain::Grain(unsigned int sampling_rate, zzub::host *host) {
  this->status = STATUS_FREE;
  this->sampling_rate = sampling_rate;
  this->host = host;
  this->env = new lanternfish::GrainEnv();
}

Grain::~Grain() {
  delete this->env;
  printf("grain done.\n");
}

inline float Grain::interpolate(float x1, float x2, float phi) {
  return x1 + (x2 - x1) * phi;
}

void Grain::set_wave(int wave) {

}

void Grain::trigger(float length, float offset, float amp, float attack,
		    float sustain, float release, float rate, float pan) {
  if (this->status == STATUS_FREE) {
    this->status = STATUS_PLAYING;
    this->counter = 0.0;
    this->sample_count = (length / 1000.0) * this->sampling_rate;
    this->offset = (unsigned int)(offset * this->samples->size());
    this->rate = rate < 0.0 ? 1.0 + rate / 4.0 : 1.0 + rate;
    this->amp = amp * 0.5;
    int attack_samples = this->sampling_rate * (attack * (length / 1000.0));
    int sustain_samples = this->sampling_rate * (sustain * (length / 1000.0));
    int release_samples = this->sampling_rate * (release * (length / 1000.0));
    pan = (pan + 1.0) * 0.5;
    this->amp_l = sqrt(pan);
    this->amp_r = sqrt(1.0 - pan);
    env->set_attack_length(attack_samples);
    env->set_sustain_length(sustain_samples);
    env->set_release_length(release_samples);
    env->trigger();
  }
}

void Grain::process(float *out_l, float *out_r, int n) {
  // If the grain is not triggered then chicken out.
  if (is_free())
    return;
  // First lets check wether the wave file is loaded.
  if (!this->samples->empty()) {
    float *env_buffer = new float[n];
    this->env->process(env_buffer, n);
    for (int i = 0; i < n; i++) {
      int counter_floor = floor(this->counter);
      int counter_ceil = ceil(this->counter);
      float phi;
      phi = this->counter - float(counter_floor);
      float x1, x2;
      x1 = (*samples)[(offset + counter_floor) % this->samples->size()];
      x2 = (*samples)[(offset + counter_ceil) % this->samples->size()];
      float sample;
      sample = interpolate(x1, x2, phi) * env_buffer[i] * this->amp;
      out_l[i] += (sample * this->amp_l);
      out_r[i] += (sample * this->amp_r);
      this->counter += this->rate;
    }
    if (this->env->is_off()) {
      this->counter = 0.0;
      this->status = STATUS_FREE;
    }
    delete[] env_buffer;
  }
}

bool Grain::is_free() {
  return this->status == STATUS_FREE;
}
