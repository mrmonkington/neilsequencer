#include <cstdio>
#include <cmath>

#include "Muffin.hpp"

Muffin::Muffin() {
  global_values = &gval;
  track_values = &tval;
  attributes = 0;
}

Muffin::~Muffin() {

}

void Muffin::init(zzub::archive* pi) {
  for (int i = 0; i < 16; i++) {
    voices[i].set_sampling_rate(_master_info->samples_per_second);
  }
  active_voices = 1;
}

void Muffin::destroy() {

}

void Muffin::set_track_count(int tracks) {
  active_voices = tracks;
}

void Muffin::process_events() {
  if (gval.wave != paramWave->value_none) {
    samples.clear();
    const zzub::wave_level *wave_level;
    wave_level = _host->get_wave_level(gval.wave, 0);
    if (wave_level) {
      int channels = _host->get_wave(gval.wave)->flags & 
	zzub::wave_flag_stereo ? 2 : 1;
      for (int i = 0; i < wave_level->sample_count; i++) {
	samples.push_back(wave_level->samples[i * channels] / 32768.0);
      }
      for (int i = 0; i < 16; i++) {
	voices[i].samples = &samples;
      }
    } else {
      for (int i = 0; i < 16; i++) {
	voices[i].samples = 0;
      }
    }
  }
  if (gval.attack != paramAttack->value_none) {
    float fvalue = 10.0 + (gval.attack / float(paramAttack->value_max)) * 990.0;
    int ivalue = _master_info->samples_per_second * (fvalue / 1000.0);
    for (int i = 0; i < 16; i++) {
      voices[i].set_attack(ivalue);
    }
  }
  if (gval.decay != paramDecay->value_none) {
    float fvalue = 10.0 + (gval.decay / float(paramDecay->value_max)) * 990.0;
    int ivalue = _master_info->samples_per_second * (fvalue / 1000.0);
    for (int i = 0; i < 16; i++) {
      voices[i].set_decay(ivalue);
    }
  }
  if (gval.sustain != paramSustain->value_none) {
    for (int i = 0; i < 16; i++) {
      voices[i].set_sustain(gval.sustain / float(paramSustain->value_max));
    }
  }
  if (gval.release != paramRelease->value_none) {
    float fvalue = 10.0 + (gval.release / float(paramRelease->value_max)) * 990.0;
    int ivalue = _master_info->samples_per_second * (fvalue / 1000.0);
    for (int i = 0; i < 16; i++) {
      voices[i].set_decay(ivalue);
    }
  }
  if (gval.mode != paramMode->value_none) {
    for (int i = 0; i < 16; i++) {
      voices[i].set_filter_mode(gval.mode);
    }
  }
  if (gval.cutoff != paramCutoff->value_none) {
    for (int i = 0; i < 16; i++) {
      voices[i].set_cutoff(gval.cutoff);
    }
  }
  if (gval.resonance != paramResonance->value_none) {
    for (int i = 0; i < 16; i++) {
      voices[i].set_resonance(gval.resonance / 
			      float(paramResonance->value_max));
    }
  }
  if (gval.env_amount != paramEnvAmount->value_none) {
    for (int i = 0; i < 16; i++) {
      voices[i].set_env_amount(gval.env_amount /
			       float(paramEnvAmount->value_max));
    }
  }
  if (gval.tabsize != paramTabsize->value_none) {
    for (int i = 0; i < 16; i++) {
      voices[i].set_tabsize(gval.tabsize);
    }
  }
  if (gval.volume != paramVolume->value_none) {
    for (int i = 0; i < 16; i++) {
      float db = 24.0 * (gval.volume / float(paramVolume->value_none)) - 18.0;
      float scale = pow(10.0, db / 10.0);
      voices[i].set_volume(scale);
    }
  }
  for (int i = 0; i < active_voices; i++) {
    if (tval[i].note != zzub::note_value_none) {
      if (tval[i].note != zzub::note_value_off) {
	int note = tval[i].note - 5;
	voices[i].note_on(note);
      } else {
	voices[i].note_off();
      }
    }
  }
}

bool Muffin::process_stereo(float **pin, float **pout, int n, int mode) {
  /*
  for (int i = 0; i < active_voices; i++) {
    voices[i].process(pout[0], pout[1], n);
  }
  */
  voices[0].process(pout[0], pout[1], n);
}

const char *Muffin::describe_value(int param, int value) {
  static const int LABEL_SIZE = 20;
  static char str[LABEL_SIZE];
  const char *wave_name = _host->get_wave_name(value);
  switch (param) {
    // Wave
  case 0:
    for (int i = 0; i < LABEL_SIZE; i++) {
      str[i] = wave_name[i];
    }
    break;
    // Attack
  case 1:
    sprintf(str, "%.2fms", 10.0 + 
	    (value / float(paramAttack->value_max)) * 990.0);
    break;
    // Decay
  case 2:
    sprintf(str, "%.2fms", 10.0 + 
	    (value / float(paramDecay->value_max)) * 990.0);
    break;
    // Sustain
  case 3:
    sprintf(str, "%.2f", value / float(paramSustain->value_max));
    break;
    // Release
  case 4:
    sprintf(str, "%.2fms", 10.0 + 
	    (value / float(paramRelease->value_max)) * 990.0);
    break;
    // Mode
  case 5:
    switch (value) {
    case 0:
      sprintf(str, "Lowpass");
      break;
    case 1:
      sprintf(str, "Highpass");
      break;
    case 2:
      sprintf(str, "Bandpass");
      break;
    case 3:
      sprintf(str, "Notch");
      break;
    }
    break;
    // Cutoff
  case 6:
    sprintf(str, "%dHz", value);
    break;
    // Resonance
  case 7:
    sprintf(str, "%.3f", value / float(paramResonance->value_max));
    break;
    // EnvAmount
  case 8:
    sprintf(str, "%.3f", value / float(paramEnvAmount->value_max));
    break;
    // Tabsize
  case 9:
    sprintf(str, "%d", value);
    break;
    // Volume
  case 10:
    sprintf(str, "%.2fdB", 24.0 * value / float(paramVolume->value_max) - 18.0);
    break;
  default:
    sprintf(str, "%d", value);
    break;
  }
  return str;
}



