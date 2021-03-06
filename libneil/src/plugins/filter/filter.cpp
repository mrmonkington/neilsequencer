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
  for (int i = 0; i < RMS_WINDOW; i++) {
    rms_buffer[i] = 0.0;
  }
  squared_sum = 0.0;
  rms_cursor = 0;
  phase = 1.0;
}

Filter::~Filter() 
{

}

void Filter::init(zzub::archive* pi) 
{
  svf_l.set_sampling_rate(_master_info->samples_per_second);
  svf_r.set_sampling_rate(_master_info->samples_per_second);
  lag_cutoff.set_value(2000.0, 0);
  svf_l.set_resonance(0.0);
  svf_r.set_resonance(0.0);
}

void Filter::destroy() 
{

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
    lag_cutoff.set_value(cutoff, _master_info->samples_per_tick);
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
  if (mode == zzub::process_mode_write || mode == zzub::process_mode_no_io) {
    return false;
  }
  if (mode == zzub::process_mode_read) {
    return true;
  }
  lag_cutoff.process(svf_cutoff, n);
  for (int i = 0; i < n; i++) {
    rms_buffer[rms_cursor] = (pin[0][i] + pin[1][i]) * 0.5;
    float newest = rms_buffer[rms_cursor];
    float oldest = rms_buffer[(rms_cursor + 1) % RMS_WINDOW];
    squared_sum -= oldest * oldest;
    squared_sum += newest * newest;
    rms_out[i] = sqrt(squared_sum * RMS_WINDOW_INV);
    rms_out[i] = std::max(rms_out[i], 0.0f);
    rms_out[i] = std::min(rms_out[i], 1.0f);
    rms_cursor = (rms_cursor + 1) % RMS_WINDOW;
  }
  float lfo;
  for (int i = 0; i < n; i++) {
    rms_out[i] = (1.0 - rms_amp) + rms_out[i] * rms_amp;
    svf_cutoff[i] *= rms_out[i];
    lfo = (1.0 + sin(2.0 * M_PI * phase)) * 0.5;
    phase += lfo_speed / _master_info->samples_per_second;
    while (phase > 1.0) {
      phase -= 1.0;
    }
    svf_cutoff[i] *= (1.0 - lfo_amp) + lfo * rms_amp;
  }
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



