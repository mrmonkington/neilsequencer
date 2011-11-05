#include <cstdio>
#include <cmath>

#include "DX10.hpp"

DX10::DX10() {
  global_values = &gval;
  track_values = &tval;
  attributes = 0;
}

void DX10::init(zzub::archive *pi) {
  param[0] = 0.000f;
  param[1] = 0.650f;
  param[2] = 0.441f;
  param[3] = 0.842f;
  param[4] = 0.329f;
  param[5] = 0.230f;
  param[6] = 0.800f;
  param[7] = 0.050f;
  param[8] = 0.800f;
  param[9] = 0.900f;
  param[10] = 0.000f;
  param[11] = 0.500f;
  param[12] = 0.500f;
  param[13] = 0.447f;
  param[14] = 0.000f;
  param[15] = 0.414f;
  Fs = _master_info->samples_per_second;
  //initialise...
  for (int i = 0; i < NVOICES; i++) {
    voice[i].env = 0.0f;
    voice[i].car = voice[i].dcar = 0.0f;
    voice[i].mod0 = voice[i].mod1 = voice[i].dmod = 0.0f;
    voice[i].cdec = 0.99f; //all notes off
    last_note[i] = 0;
  }
  lfo0 = dlfo = modwhl = 0.0f;
  lfo1 = pbend = 1.0f;
  volume = 0.0035f;
  sustain = activevoices = 0;
  K = 0;
  ntracks = 1;
  update();
}

void DX10::set_track_count(int track_count) 
{
  ntracks = track_count;
}

void DX10::update()
{
  float ifs = 1.0f / _master_info->samples_per_second;
  tune = (float)(8.175798915644 * ifs * pow(2.0, floor(param[11] * 6.9) - 2.0));

  rati = param[3];
  rati = (float)floor(40.1f * rati * rati);

  if (param[4] < 0.5f) {
    ratf = 0.2f * param[4] * param[4];
  } else {
    switch ((int)(8.9f * param[4])) {
      case 4: 
	ratf = 0.25f;       
	break;
      case 5: 
	ratf = 0.33333333f; 
	break;
      case 6: 
	ratf = 0.50f;       
	break;
      case 7: 
	ratf = 0.66666667f; 
	break;
      default: 
	ratf = 0.75f;
    }
  }
  ratio = 1.570796326795f * (rati + ratf);

  depth = 0.0002f * param[5] * param[5];
  dept2 = 0.0002f * param[7] * param[7];

  velsens = param[9];
  vibrato = 0.001f * param[10] * param[10];

  catt = 1.0f - (float)exp(-ifs * exp(8.0 - 8.0 * param[0]));
  if (param[1] > 0.98f) { 
    cdec = 1.0f; 
  } else {
    cdec = (float)exp(-ifs * exp(5.0 - 8.0 * param[1]));
  }
  crel = (float)exp(-ifs * exp(5.0 - 5.0 * param[2]));
  mdec = 1.0f - (float)exp(-ifs * exp(6.0 - 7.0 * param[6]));
  mrel = 1.0f - (float)exp(-ifs * exp(5.0 - 8.0 * param[8]));

  rich = 0.50f - 3.0f * param[13] * param[13];
  modmix = 0.25f * param[14] * param[14];
  dlfo = 628.3f * ifs * 25.0f * param[15] * param[15];
}
	
void DX10::process_events() {
  if (gval.attack != 65535) {
    param[0] = gval.attack * 0.001f;
  }
  if (gval.decay != 65535) {
    param[1] = gval.decay * 0.001f;
  }
  if (gval.release != 65535) {
    param[2] = gval.release * 0.001f;
  }
  if (gval.coarse != 65535) {
    param[3] = gval.coarse * 0.001f;
  }
  if (gval.fine != 65535) {
    param[4] = gval.fine * 0.001f;
  }
  if (gval.mod_init != 65535) {
    param[5] = gval.mod_init * 0.001f;
  }
  if (gval.mod_dec != 65535) {
    param[6] = gval.mod_dec * 0.001f;
  }
  if (gval.mod_sus != 65535) {
    param[7] = gval.mod_sus * 0.001f;
  }
  if (gval.mod_rel != 65535) {
    param[8] = gval.mod_rel * 0.001f;
  }
  if (gval.mod_vel != 65535) {
    param[9] = gval.mod_vel * 0.001f;
  }
  if (gval.vibrato != 65535) {
    param[10] = gval.vibrato * 0.001f;
  }
  if (gval.octave != 65535) {
    param[11] = gval.octave * 0.001f;
  }
  if (gval.fine_tune != 65535) {
    param[12] = gval.fine_tune * 0.001f;
  }
  if (gval.waveform != 65535) {
    param[13] = gval.waveform * 0.001f;
  }
  if (gval.mod_thru != 65535) {
    param[14] = gval.mod_thru * 0.001f;
  }
  if (gval.lfo_rate != 65535) {
    param[15] = gval.lfo_rate * 0.001f;
  }
  for (int track = 0; track < ntracks; track++) {
    if (tval[track].note != zzub::note_value_none) {
      if (tval[track].note == zzub::note_value_off) {
	noteOn(last_note[track], 0);
	last_note[track] = 0;
      } else {
	int oct = tval[track].note / 16 + 1;
	int note = tval[track].note % 16 - 1;
	int midi = oct * 12 + note;
	if (tval[track].velocity != 255) {
	  noteOn(last_note[track], 0);
	  noteOn(midi, tval[track].velocity);
	  last_note[track] = midi;
	} else {
	  noteOn(last_note[track], 0);
	  noteOn(midi, 80);
	  last_note[track] = midi;
	}
      }
    }
  }
  update();
}

bool DX10::process_stereo(float **pin, float **pout, int sampleFrames, int mode) {
  float* out1 = pout[0];
  float* out2 = pout[1];
  int event = 0, frame = 0, frames, v;
  float o, x, e, mw = MW, w = rich, m = modmix;
  int k = K;
  
  while (--sampleFrames >= 0) {
    VOICE *V = voice;
    o = 0.0f;

    if (--k < 0) {
      lfo0 += dlfo * lfo1; //sine LFO
      lfo1 -= dlfo * lfo0;
      mw = lfo1 * (modwhl + vibrato);
      k = 100;
    }

    for (v = 0; v < NVOICES; v++) {
      // for each voice
      e = V->env;
      if (e > SILENCE) {
	// **** this is the synth ****
	V->env = e * V->cdec; // decay & release
	V->cenv += V->catt * (e - V->cenv); // attack

	x = V->dmod * V->mod0 - V->mod1; // could add more modulator blocks like
	V->mod1 = V->mod0; // this for a wider range of FM sounds
	V->mod0 = x;    
	V->menv += V->mdec * (V->mlev - V->menv);

	x = V->car + V->dcar + x * V->menv + mw; // carrier phase
	while (x > 1.0f) {
	  x -= 2.0f; // wrap phase
	}
	while(x < -1.0f) {
	  x += 2.0f;
	}
	V->car = x;
	o += V->cenv * (m * V->mod1 + (x + x * x * x * (w * x * x - 1.0f - w))); 
      }
      V++;
    }
    *out1++ = o;
    *out2++ = o;
  }
  for (v = 0; v < NVOICES; v++) {
    if (voice[v].env < SILENCE) {
      // choke voices that have finished
      voice[v].env = voice[v].cenv = 0.0f;
    }
    if (voice[v].menv < SILENCE) {
      voice[v].menv = voice[v].mlev = 0.0f;
    }
  }

  K = k; 
  MW = mw; //remember these so vibrato speed not buffer size dependant!
  return true;
}

const char *DX10::describe_value(int index, int value) {
  static char txt[20];
  switch (index) {
  case 3:
    sprintf(txt, "%.0f ratio", rati);
    break;
  case 4:
    sprintf(txt, "%.3f ratio", ratf);
    break;
  case 11:
    sprintf(txt, "%d", int(param[index] * 6.9f) - 3);
    break;
  case 12:
    sprintf(txt, "%.0f cents", 200.0f * param[index] - 100.0f);
    break;
  case 15:
    sprintf(txt, "%.2f Hz", 25.0f * param[index] * param[index]);
    break;
  case 16:
    return 0;
  case 17:
    return 0;
  default:
    sprintf(txt, "%.0f%%", 100.0f * param[index]);
    break;
  }
  return txt;
}

void DX10::noteOn(int note, int velocity)
{
  float l = 1.0f;
  int  v, vl = 0;
  
  if (velocity > 0) {
    for (v = 0; v < NVOICES; v++) {
      // find quietest voice
      if (voice[v].env < l) {
	l = voice[v].env;  
	vl = v; 
      }
    }

    l = (float)exp(0.05776226505f * ((float)note + param[12] + param[12] - 1.0f));
    voice[vl].note = note; //fine tuning
    voice[vl].car  = 0.0f;
    voice[vl].dcar = tune * pbend * l; //pitch bend not updated during note as a bit tricky...

    if (l > 50.0f) {
      l = 50.0f; //key tracking
    }
    l *= (64.0f + velsens * (velocity - 64)); //vel sens
    voice[vl].menv = depth * l;
    voice[vl].mlev = dept2 * l;
    voice[vl].mdec = mdec;

    voice[vl].dmod = ratio * voice[vl].dcar; //sine oscillator
    voice[vl].mod0 = 0.0f;
    voice[vl].mod1 = (float)sin(voice[vl].dmod); 
    voice[vl].dmod = 2.0f * (float)cos(voice[vl].dmod);
    //scale volume with richness
    voice[vl].env  = (1.5f - param[13]) * volume * (velocity + 10);
    voice[vl].catt = catt;
    voice[vl].cenv = 0.0f;
    voice[vl].cdec = cdec;
  } else {
    // note off
    for (v = 0; v < NVOICES; v++) {
      if (voice[v].note == note) {
	// any voices playing that note?
	if (sustain == 0) {
	  voice[v].cdec = crel; //release phase
	  voice[v].env  = voice[v].cenv;
	  voice[v].catt = 1.0f;
	  voice[v].mlev = 0.0f;
	  voice[v].mdec = mrel;
	} else {
	  voice[v].note = SUSTAIN;
	}
      }
    }
  }
}
