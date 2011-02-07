#include <cmath>
#include <cstdio>
#include <algorithm>
#include "Grain.hpp"

Grain::Grain(unsigned int sampling_rate, zzub::host *host) {
  status = STATUS_FREE;
  this->sampling_rate = sampling_rate;
  this->host = host;
  env = new lanternfish::GrainEnv();
}

Grain::~Grain() {
  delete env;
}

inline float Grain::interpolate(float x1, float x2, float phi) {
  return x1 + (x2 - x1) * phi;
}

void Grain::set_wave(int wave) {

}

void Grain::trigger(float length, float offset, float amp, float attack,
		    float sustain, float release, float rate, float pan) {
  if (status == STATUS_FREE) {
    status = STATUS_PLAYING;
    counter = 0.0;
    sample_count = (length / 1000.0) * sampling_rate;
    this->offset = (unsigned int)(offset * samples->size());
    this->rate = rate < 0.0 ? 1.0 + rate / 4.0 : 1.0 + rate;
    this->amp = amp * 0.5;
    int attack_samples = sampling_rate * (attack * (length / 1000.0));
    int sustain_samples = sampling_rate * (sustain * (length / 1000.0));
    int release_samples = sampling_rate * (release * (length / 1000.0));
    pan = (pan + 1.0) * 0.5;
    amp_l = sqrt(1.0 - pan);
    amp_r = sqrt(pan);
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
    env->process(env_buffer, n);
    for (int i = 0; i < n; i++) {
      double counter_int;
      float counter_frac = modf(counter, &counter_int);
      float x1, x2;
      x1 = (*samples)[int(offset + counter_int) % samples->size()];
      x2 = (*samples)[int(offset + counter_int + 1) % samples->size()];
      float sample;
      sample = interpolate(x1, x2, counter_frac) * env_buffer[i] * amp;
      out_l[i] += (sample * amp_l);
      out_r[i] += (sample * amp_r);
      counter += rate;
    }
    if (env->is_off()) {
      counter = 0.0;
      status = STATUS_FREE;
    }
    delete[] env_buffer;
  }
}

bool Grain::is_free() {
  return status == STATUS_FREE;
}
