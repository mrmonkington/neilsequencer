#include <cstdio>
#include <cmath>

#include "Osc.hpp"
#include "Phasor.hpp"
#include "Lag.hpp"
#include "Adsr.hpp"
#include "Utils.hpp"

using namespace lanternfish;

#include "FM303.hpp"

FM303::FM303() 
{
  global_values = &gval;
  track_values = &tval;
  attributes = 0;
  sine = make_sine_table(1024);
  osc_c.set_table(this->sine, 1024);
  osc_m.set_table(this->sine, 1024);
}

FM303::~FM303() 
{
  delete[] this->sine;
}

void FM303::init(zzub::archive* pi) 
{
  phasor_m.set_sampling_rate(_master_info->samples_per_second);
  phasor_c.set_sampling_rate(_master_info->samples_per_second);
  aenv.set_attack_time(0.002 * _master_info->samples_per_second);
  aenv.set_decay_time(0.5 * _master_info->samples_per_second);
  aenv.set_sustain_level(0.4);
  aenv.set_release_time(0.01 * _master_info->samples_per_second);
  menv.set_attack_time(0.002 * _master_info->samples_per_second);
  menv.set_decay_time(0.3 * _master_info->samples_per_second);
  menv.set_sustain_level(0.0);
  menv.set_release_time(0.01 * _master_info->samples_per_second);
}

void FM303::destroy() 
{

}

void FM303::set_track_count(int tracks) 
{

}

void FM303::process_events() 
{
  if (gval.wave != paramWave->value_none) {
    wave = gval.wave;
  }
  if (gval.modulation != paramModulation->value_none) {
    mod.set_value((gval.modulation / float(paramModulation->value_max)),
		  _master_info->samples_per_tick);
  }
  if (gval.feedback != paramFeedback->value_none) {
    feedback = 0.33 * (gval.feedback / float(paramFeedback->value_max));
  }
  if (gval.decay != paramDecay->value_none) {
    decay_time = (0.03 + 0.47 * gval.decay / float(paramDecay->value_max)) *
      _master_info->samples_per_second;
    menv.set_decay_time(decay_time);
  }
  if (gval.env_mod != paramEnvMod->value_none) {
    env_mod = gval.env_mod / float(paramEnvMod->value_max);
    menv.set_peak_level(env_mod);
  }
  if (gval.acc_amount != paramAccAmount->value_none) {
    acc_amount = gval.acc_amount / float(paramAccAmount->value_max);
  }
  if (tval.note != paramNote->value_none) {
    if (tval.note != zzub::note_value_off) {
      if (tval.slide != paramSlide->value_none) {
	freq.set_value(note_to_freq(tval.note), 
		       0.8 * _master_info->samples_per_tick);
      } else {
	freq.set_value(note_to_freq(tval.note), 16);
	aenv.note_on();
	menv.note_on();
      }
      if (tval.accent != paramAccent->value_none) {
	aenv.set_peak_level(0.6 + 0.4 * acc_amount);
	menv.set_peak_level(env_mod + 0.6 * acc_amount);
	aenv.set_decay_time(decay_time * (1.0 - 0.5 * acc_amount));
	menv.set_decay_time(decay_time * (1.0 - 0.5 * acc_amount));
	aenv.set_attack_time((0.002 + 0.004 * acc_amount) * 
			     _master_info->samples_per_second);
	menv.set_attack_time((0.002 + 0.004 * acc_amount) *
			     _master_info->samples_per_second);
      } else {
	aenv.set_peak_level(0.6);
	menv.set_peak_level(env_mod);
	aenv.set_decay_time(decay_time);
	menv.set_decay_time(decay_time);
	aenv.set_attack_time(0.002 * _master_info->samples_per_second);
	menv.set_attack_time(0.002 * _master_info->samples_per_second);
      }
    } else {
      aenv.note_off();
      menv.note_off();
    }
  }
}

bool FM303::process_stereo(float **pin, float **pout, int n, int mode) 
{
  aenv.print_stats();
  menv.print_stats();
  aenv.process(s_aenv, n);
  menv.process(s_menv, n);
  mod.process(s_mod, n);
  freq.process(s_freq, n);
  phasor_c.process(s_freq, s_phasor_c, n);
  // If a square wave is chosen, modulator has to run at double frequency.
  if (wave == 1)
    mul_signals(2.0, s_freq, n);
  for (int i = 0; i < n; i++) {
    float phase = phasor_m.process(s_freq[i]);
    phase += feedback_v;
    s_osc_m[i] = osc_m.process(phase);
    feedback_v = feedback * s_osc_m[i];
  }
  add_signals(s_menv, s_mod, n);
  mul_signals(s_mod, s_osc_m, n);
  add_signals(s_osc_m, s_phasor_c, n);
  osc_c.process(s_phasor_c, s_osc_c, n);
  mul_signals(s_aenv, s_osc_c, n);
  signal_copy(s_osc_c, pout[0], n);
  signal_copy(s_osc_c, pout[1], n);
  return true;
}

const char *FM303::describe_value(int param, int value) 
{
  static char txt[16];
  // Wave parameter
  if (param == 0) {
    if (value == 0) {
      sprintf(txt, "Saw");
    } else {
      sprintf(txt, "Square");
    }
    return txt;
  } else if (param == 1 || param == 2 || param == 4 || param == 5) {
    sprintf(txt, "%.3f", value / float(0xfffe));
    return txt;
  } else if (param == 3) {
    sprintf(txt, "%.2f ms", 30.0 + value / float(0xfffe) * 470.0);
    return txt;
  }
  else {
    return 0;
  }
}



