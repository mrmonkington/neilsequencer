#ifndef SOMONO_PLUGINS_CLOUD_ACLOUD_HPP
#define SOMONO_PLUGINS_CLOUD_ACLOUD_HPP

#include "Grain.hpp"
#include <zzub/plugin.h>

#define MAX_GRAINS 64
#define MAX_WORD 0xFFFE
#define MIN_LENGTH 10.0f
#define MAX_LENGTH 1000.0f
#define MAX_BYTE 0xFE
#define MIN_RATE -4.0f
#define MAX_RATE 4.0f
#define MIN_PAN -1.0f
#define MAX_PAN 1.0f
#define DIST_SIZE 1024

class ACloud {
private:
  // Private variables
  unsigned int sampling_rate;
  float offset_mean, offset_devi;
  float amp_mean, amp_devi;
  float length_mean, length_devi; // in miliseconds
  float sustain_mean, sustain_devi;
  float skew_mean, skew_devi;
  float rate_mean, rate_devi;
  float pan_mean, pan_devi;
  float density;
  int ngrains;
  zzub::host *host;
  bool random_event(float probability);
  float random_value(float min, float max);
  float calculate_offset();
  float calculate_amp();
  float calculate_length();
  float calculate_sustain();
  float calculate_skew();
  float calculate_rate();
  float calculate_pan();
  void calculate_envelope(float *a, float *s, float *r);
  Grain *grains[MAX_GRAINS];
  float dist[DIST_SIZE];
  std::vector <float> wave;
  int old_index;
public:
  ACloud(unsigned int sampling_rate, zzub::host *host);
  ~ACloud();
  void set_wave(int wave);
  void set_offset_mean(float offset_mean);
  void set_offset_devi(float offset_devi);
  void set_amp_mean(float amp_mean);
  void set_amp_devi(float amp_devi);
  void set_length_mean(float length_mean);
  void set_length_devi(float length_devi);
  void set_sustain_mean(float sustain_mean);
  void set_sustain_devi(float sustain_devi);
  void set_skew_mean(float skew_mean);
  void set_skew_devi(float skew_devi);
  void set_rate_mean(float rate_mean);
  void set_rate_devi(float rate_devi);
  void set_pan_mean(float pan_mean);
  void set_pan_devi(float pan_devi);
  void set_density(float density);
  void set_grains(int grains);
  void process(float *out_l, float *out_r, int n, int wave_index);
};

#endif // SOMONO_PLUGINS_CLOUD_ACLOUD_HPP
