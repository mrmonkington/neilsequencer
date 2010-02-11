#ifndef MUFFIN_VOICE_HPP
#define MUFFIN_VOICE_HPP

#include <vector>

#include "Adsr.hpp"
#include "Svf.hpp"
#include "Osc.hpp"
#include "Scale.hpp"
#include "Mul.hpp"
#include "Value.hpp"

class Voice {
private:
  lanternfish::Osc osc;
  lanternfish::Svf filter;
  lanternfish::Adsr env;
  lanternfish::Scale scale;
  lanternfish::Value freq;
  lanternfish::Value cutoff;
  lanternfish::Mul mul1;
  lanternfish::Mul mul2;
  int glide, filter_inertia;
  std::vector <float> table;
  float vol;
  float phi;
  float env_mod_min;
  int mode;
  float sample;
public:
  Voice();
  ~Voice();
  std::vector <float> *samples;
  void note_on(int note);
  void note_off();
  void set_sampling_rate(int sampling_rate);
  void set_attack(int attack);
  void set_decay(int decay);
  void set_sustain(float sustain);
  void set_release(int release);
  void set_cutoff(int cutoff);
  void set_resonance(float resonance);
  void set_env_amount(float env_amount);
  void set_tabsize(int tabsize);
  void set_filter_mode(int mode);
  void set_volume(float vol);
  void process(float *out_l, float *out_r, int n);
};

#endif // MUFFIN_VOICE_HPP
