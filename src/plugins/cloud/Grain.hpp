#ifndef SOMONO_PLUGINS_CLOUD_GRAIN_HPP
#define SOMONO_PLUGINS_CLOUD_GRAIN_HPP

#include <zzub/plugin.h>
#include <GrainEnv.hpp>

class Grain {
public:
  enum GrainStatus {
    STATUS_FREE,
    STATUS_PLAYING
  };  
private:
  int wave;
  zzub::host *host;
  GrainStatus status;
  float counter;
  unsigned int offset;
  unsigned int sampling_rate;
  unsigned int sample_count;
  bool is_stereo;
  float amp, amp_l, amp_r, rate;
  lanternfish::GrainEnv *env;
  float interpolate(float x1, float x2, float phi);
public:
  Grain(unsigned int sampling_rate, zzub::host *host);
  ~Grain();
  void set_wave(int wave);
  bool is_free();
  void trigger(float length, float offset, float amp, float attack, 
	       float sustain, float release, float rate, float pan);
  void process(float *out_l, float *out_r, int n);
};

#endif // SOMONO_PLUGINS_CLOUD_GRAIN_HPP
