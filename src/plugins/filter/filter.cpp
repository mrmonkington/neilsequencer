#include <cstdio>
#include <cmath>

#include "Osc.hpp"
#include "Phasor.hpp"
#include "Lag.hpp"
#include "Svf.hpp"
#include "Utils.hpp"

using namespace lanternfish;

#include "filter.hpp"

Filter::Filter() 
{
  global_values = &gval;
  track_values = 0;
  attributes = 0;
  for (int i = 0; i < 16; i++) {
    rms_buffer[i] = 0.0;
  }
  rms_cursor = 0;
}

Filter::~Filter() 
{

}

void Filter::init(zzub::archive* pi) 
{
  sine_table = make_sine_table(256);
  svf_l.set_sampling_rate(_master_info->samples_per_second);
  svf_r.set_sampling_rate(_master_info->samples_per_second);
  phase.set_sampling_rate(_master_info->samples_per_second);
  lfo.set_table(sine_table, 256);
}

void Filter::destroy() 
{
  delete[] sine_table;
}

void Filter::stop() {

}

void Filter::set_track_count(int tracks) 
{

}

void Filter::process_events() 
{
  if (gval.type != paramType->value_none) {
    type = gval.type;
  }
  if (gval.cutoff != paramCutoff->value_none) {
    cutoff = 10.0 + pow(gval.cutoff / float(paramCutoff->value_max), 4.0) * 
      19990.0;
  }
  if (gval.resonance != paramResonance->value_none) {
    svf_l.set_resonance(gval.resonance / float(paramResonance->value_max));
    svf_r.set_resonance(gval.resonance / float(paramResonance->value_max));
  }
  if (gval.lfo_speed != paramLfoSpeed->value_none) {
    lfo_speed = 0.01 + 
      pow(gval.lfo_speed / float(paramLfoSpeed->value_max), 8.0) * 5000.0;
  }
  if (gval.lfo_amp != paramLfoAmp->value_none) {
    lfo_amp = (gval.lfo_amp / float(paramLfoAmp->value_max)) * 0.99;
  }
  if (gval.env_mod != paramEnvMod->value_none) {
    rms_amp = (gval.env_mod / float(paramEnvMod->value_max)) * 0.99;
  }
}

bool Filter::process_stereo(float **pin, float **pout, int n, int mode) 
{
  const_signal(phaser_freq, lfo_speed, n);
  phase.process(phaser_freq, phaser, n);
  lfo.process(phaser, lfo_out, n);
  for (int i = 0; i < n; i++) {
    rms_out[i] = rms(rms_buffer, 16);
    rms_buffer[rms_cursor] = (pin[0][i] + pin[1][i]) * 0.5;
    rms_cursor = (rms_cursor + 1) % 16;
  }
  const_signal(svf_cutoff, cutoff, n);
  add_signals(1.0, lfo_out, n);
  mul_signals(0.5, lfo_out, n);
  scale_signal(lfo_out, 1.0 - lfo_amp, 1.0, n);
  mul_signals(lfo_out, svf_cutoff, n);
  scale_signal(rms_out, 1.0 - rms_amp, 1.0, n);
  mul_signals(rms_out, svf_cutoff, n);
  svf_l.process(pout[0], svf_cutoff, pin[0], type, n);
  svf_r.process(pout[1], svf_cutoff, pin[1], type, n);
  return true;
}

const char *Filter::describe_value(int param, int value) 
{
  static char txt[16];
  if (param == 0) {
    switch (value) {
    case 0:
      sprintf(txt, "Lowpass");
      break;
    case 1:
      sprintf(txt, "Highpass");
      break;
    case 2:
      sprintf(txt, "Bandpass");
    }
  } else if (param == 1) {
    sprintf(txt, "%.2f Hz", 
	    10.0 + pow(value / float(paramCutoff->value_max), 4.0) * 19990.0);
  } else if (param == 2) {
    sprintf(txt, "%.2f", value / float(paramResonance->value_max));
  } else if (param == 3) {
    sprintf(txt, "%.2f Hz", 
	    pow(value / float(paramLfoSpeed->value_max), 8.0) * 5000.0);
  } else if (param == 4) {
    sprintf(txt, "%.2f", value / float(paramLfoAmp->value_max));
  } else if (param == 5) {
    sprintf(txt, "%.2f", value / float(paramEnvMod->value_max));
  } else {
    return 0;
  }
  return txt;
}



