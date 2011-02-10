#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <cmath>
#include <stdint.h>
#include "ACloud.hpp"
#include "Utils.hpp"

bool ACloud::random_event(float probability) {
  /*
    Simulate a random event. Returns true if an event
    with the specified probability occured and false
    otherwise.
  */
  return ((float)rand() / (float)RAND_MAX) < probability;
}

float ACloud::random_value(float min, float max) {
  /*
    Look up a value in the distribution table and scale it
    so that it is between min and max.
  */
  return min + (double(rand()) / double(RAND_MAX)) * (max - min);
}

float ACloud::calculate_offset() {
  float devi = offset_devi / 2.0;
  float mini = std::max(offset_mean - devi, 0.0f);
  float maxi = std::min(offset_mean + devi, 1.0f);
  return random_value(mini, maxi);
}

float ACloud::calculate_amp() {
  float devi = amp_devi / 2.0;
  float mini = std::max(amp_mean - devi, 0.0f);
  float maxi = std::min(amp_mean + devi, 1.0f);
  return random_value(mini, maxi);
}

float ACloud::calculate_length() {
  float devi = (length_devi * MAX_LENGTH) / 2.0;
  float mini = std::max(length_mean - devi, MIN_LENGTH);
  float maxi = std::min(length_mean + devi, MAX_LENGTH);
  return random_value(mini, maxi);  
}

float ACloud::calculate_sustain() {
  float devi = sustain_devi / 2.0;
  float mini = std::max(sustain_mean - devi, 0.0f);
  float maxi = std::min(sustain_mean + devi, 1.0f);
  return random_value(mini, maxi);
}

float ACloud::calculate_skew() {
  float devi = skew_devi / 2.0;
  float mini = std::max(skew_mean - devi, 0.0f);
  float maxi = std::min(skew_mean + devi, 1.0f);
  return random_value(mini, maxi);
}

float ACloud::calculate_rate() {
  float devi = (rate_devi * (MAX_RATE - MIN_RATE)) / 2.0;
  float mini = std::max(rate_mean - devi, MIN_RATE);
  float maxi = std::min(rate_mean + devi, MAX_RATE);
  return random_value(mini, maxi);
}

float ACloud::calculate_pan() {
  float devi = (pan_devi * (MAX_PAN - MIN_PAN)) / 2.0;
  float mini = std::max(pan_mean - devi, MIN_PAN);
  float maxi = std::min(pan_mean + devi, MAX_PAN);
  return random_value(mini, maxi);
}

void ACloud::calculate_envelope(float *a, float *s, float *r) {
  float sustain = calculate_sustain();
  float skew = calculate_skew();
  float attack_release = 1.0 - sustain;
  float attack = attack_release * skew;
  float release = attack_release * (1.0 - skew);
  *a = attack;
  *s = sustain;
  *r = release;
}

ACloud::ACloud(unsigned int sampling_rate, zzub::host *host) {
  this->sampling_rate = sampling_rate;
  this->host = host;
  // Create grains
  for (int i = 0; i < MAX_GRAINS; i++) {
    grains[i] = new Grain(sampling_rate, host);
    grains[i]->samples = &wave;
  }
  old_index = 0;
}

ACloud::~ACloud() {
  for (int i = 0; i < MAX_GRAINS; i++) {
    delete grains[i];
  }
}

void ACloud::set_wave(int wave_index) {
  /*
    Loads a wave from the wavetable with a specified index.
    This is done by converting the wave in to a float buffer.
    Only the first channel is taken if the wave is in stereo.
  */
  if (wave_index != old_index) {
    wave.clear();
    const zzub::wave_level *wave_level;
    wave_level = host->get_wave_level(wave_index, 0);
    if (wave_level) {
      int channels = host->get_wave(wave_index)->flags & 
	zzub::wave_flag_stereo ? 2 : 1;
      int nsamples = std::min(wave_level->sample_count,
			      int(sampling_rate * 10.0));
      for (int i = 0; i < nsamples; i++) {
	float sample;
	char *samples = (char *)wave_level->samples;
	switch(wave_level->format) {
	case zzub::wave_buffer_type_si16:
	  sample = ((int16_t *)samples)[i * channels] / 32768.0;
	  break;
	case zzub::wave_buffer_type_si24:
	  sample = (((int32_t *)samples)[i * channels] & 0xffffff00) / 8388608.0;
	  break;
	case zzub::wave_buffer_type_si32:
	  sample = ((int32_t *)samples)[i * channels] / 2147483648.0;
	  break;
	case zzub::wave_buffer_type_f32:
	  sample = ((float *)samples)[i * channels];
	  break;
	}
	wave.push_back(sample);
      }
    }
    old_index = wave_index;
  }
}

void ACloud::set_offset_mean(float offset_mean) {
  this->offset_mean = offset_mean;
}

void ACloud::set_offset_devi(float offset_devi) {
  this->offset_devi = offset_devi;
}

void ACloud::set_amp_mean(float amp_mean) {
  this->amp_mean = amp_mean;
}

void ACloud::set_amp_devi(float amp_devi) {
  this->amp_devi = amp_devi;
}

void ACloud::set_length_mean(float length_mean) {
  this->length_mean = length_mean;
}

void ACloud::set_length_devi(float length_devi) {
  this->length_devi = length_devi;
}

void ACloud::set_sustain_mean(float sustain_mean) {
  this->sustain_mean = sustain_mean;
}

void ACloud::set_sustain_devi(float sustain_devi) {
  this->sustain_devi = sustain_devi;
}

void ACloud::set_skew_mean(float skew_mean) {
  this->skew_mean = skew_mean;
}

void ACloud::set_skew_devi(float skew_devi) {
  this->skew_devi = skew_devi;
}

void ACloud::set_rate_mean(float rate_mean) {
  this->rate_mean = rate_mean;
}

void ACloud::set_rate_devi(float rate_devi) {
  this->rate_devi = rate_devi;
}

void ACloud::set_pan_mean(float pan_mean) {
  this->pan_mean = pan_mean;
}

void ACloud::set_pan_devi(float pan_devi) {
  this->pan_devi = pan_devi;
}

void ACloud::set_density(float density) {
  this->density = density;
}

void ACloud::set_grains(int grains) {
  this->ngrains = grains;
}

void ACloud::process(float *out_l, float *out_r, int n, int wave_index) {
  set_wave(wave_index);
  /* 
     Clear the output buffers. This is needed
     since we will be mixing in new data instead
     of replacing it.
  */
  for (int i = 0; i < n; i++) {
    out_l[i] = out_r[i] = 0.0;
  }
  /* 
     Scan the grains and see if some are free for 
     triggering. If they are and a random event is
     satisfied then trigger that grain.
  */
  for (int i = 0; i < this->ngrains; i++) {
    if (this->grains[i]->is_free()) {
      if (random_event(this->density)) {
	float length = calculate_length();
	float offset = calculate_offset();
	float amp = calculate_amp();
	float attack, sustain, release;
	calculate_envelope(&attack, &sustain, &release);
	float rate = calculate_rate();
	float pan = calculate_pan();
	grains[i]->trigger(length, offset, amp, attack, 
			   sustain, release, rate, pan);
      }
    }
    /* 
       Each grain will mix in it's contribution to the
       final audio buffer.
    */
    grains[i]->process(out_l, out_r, n);
  }
}
