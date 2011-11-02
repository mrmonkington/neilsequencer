#include <cstdio>
#include <cmath>
#include <cstdlib>

#include "JX10.hpp"

JX10::JX10() {
  global_values = &gval;
  track_values = &tval;
  attributes = 0;
  param[0] = 0.00f; // OSC Mix
  param[1] = 0.25f; // OSC Tune
  param[2] = 0.50f; // OSC Fine

  param[3] = 0.00f; // OSC Mode
  param[4] = 0.35f; // OSC Rate
  param[5] = 0.50f; // OSC Bend
  
  param[6] = 1.00f; // VCF Freq
  param[7] = 0.15f; // VCF Reso
  param[8] = 0.75f; // VCF <Env
  
  param[9] = 0.00f; // VCF <LFO
  param[10] = 0.50f; // VCF <Vel
  param[11] = 0.00f; // VCF Att

  param[12] = 0.30f; // VCF Dec
  param[13] = 0.00f; // VCF Sus
  param[14] = 0.25f; // VCF Rel

  param[15] = 0.00f; // ENV Att
  param[16] = 0.50f; // ENV Dec
  param[17] = 1.00f; // ENV Sus
	
  param[18] = 0.30f; // ENV Rel
  param[19] = 0.81f; // LFO Rate
  param[20] = 0.50f; // Vibrato
  
  param[21] = 0.00f; // Noise - not present in original patches
  param[22] = 0.50f; // Octave
  param[23] = 0.50f; // Tuning
}

void JX10::init(zzub::archive *pi) {
  Fs = _master_info->samples_per_second;
  for (int v = 0; v < NVOICES; v++)  {
    voice[v].dp = voice[v].dp2 = 1.0f;
    voice[v].saw = voice[v].p = voice[v].p2 = 0.0f;
    voice[v].env = voice[v].envd = voice[v].envl = 0.0f;
    voice[v].fenv = voice[v].fenvd = voice[v].fenvl = 0.0f;
    voice[v].f0  = voice[v].f1 = voice[v].f2 = 0.0f;
    voice[v].note = 0;
    last_note[v] = 0;
  }
  //notes[0] = EVENTS_DONE;
  lfo = modwhl = filtwhl = press = fzip = 0.0f; 
  rezwhl = pbend = ipbend = 1.0f;
  volume = 0.0005f;
  K = mode = lastnote = sustain = activevoices = 0;
  noise = 22222;

  update();
}

void JX10::update()  //parameter change
{
  double ifs = 1.0 / Fs;

  mode = (int)(7.9f * param[3]);
  noisemix = param[21] * param[21];
  voltrim = (3.2f - param[0] - 1.5f * noisemix) * (1.5f - 0.5f * param[7]);
  noisemix *= 0.06f;
  oscmix = param[0];

  semi = (float)floor(48.0f * param[1]) - 24.0f;
  cent = 15.876f * param[2] - 7.938f;
  cent = 0.1f * (float)floor(cent * cent * cent);
  detune = (float)pow(1.059463094359f, - semi - 0.01f * cent);
  tune = -23.376f - 2.0f * param[23] - 12.0f * (float)floor(param[22] * 4.9);
  tune = Fs * (float)pow(1.059463094359f, tune);

  vibrato = pwmdep = 0.2f * (param[20] - 0.5f) * (param[20] - 0.5f);
  if (param[20] < 0.5f) {
    vibrato = 0.0f;
  }

  lfoHz = (float)exp(7.0f * param[19] - 4.0f);
  dlfo = lfoHz * (float)(ifs * TWOPI * KMAX); 

  filtf = 8.0f * param[6] - 1.5f;
  filtq = (1.0f - param[7]) * (1.0f - param[7]);
  filtlfo = 2.5f * param[9] * param[9];
  filtenv = 12.0f * param[8] - 6.0f;
  filtvel = 0.1f * param[10] - 0.05f;
  if (param[10] < 0.05f) { 
    veloff = 1; 
    filtvel = 0; 
  } else {
    veloff = 0;
  }

  att = 1.0f - (float)exp(-ifs * exp(5.5 - 7.5 * param[15]));
  dec = 1.0f - (float)exp(-ifs * exp(5.5 - 7.5 * param[16]));
  sus = param[17];
  rel = 1.0f - (float)exp(-ifs * exp(5.5 - 7.5 * param[18]));
  if (param[18] < 0.01f) {
    rel = 0.1f; //extra fast release
  }

  ifs *= KMAX; //lower update rate...

  fatt = 1.0f - (float)exp(-ifs * exp(5.5 - 7.5 * param[11]));
  fdec = 1.0f - (float)exp(-ifs * exp(5.5 - 7.5 * param[12]));
  fsus = param[13] * param[13];
  frel = 1.0f - (float)exp(-ifs * exp(5.5 - 7.5 * param[14]));

  if (param[4] < 0.02f) {
    glide = 1.0f;
  } else {
    glide = 1.0f - (float)exp(-ifs * exp(6.0 - 7.0 * param[4]));
  }
  glidedisp = (6.604f * param[5] - 3.302f);
  glidedisp *= glidedisp * glidedisp;
}
	
void JX10::process_events() {
  if (gval.osc_mix != 65535) {
    param[0] = gval.osc_mix * 0.001f;
  }
  if (gval.osc_tune != 65535) {
    param[1] = gval.osc_tune * 0.001f;
  }
  if (gval.osc_fine != 65535) {
    param[2] = gval.osc_fine * 0.001f;
  }
  if (gval.osc_mode != 65535) {
    param[3] = gval.osc_mode * 0.001f;
  }
  if (gval.osc_rate != 65535) {
    param[4] = gval.osc_rate * 0.001f;
  }
  if (gval.osc_bend != 65535) {
    param[5] = gval.osc_bend * 0.001f;
  }
  if (gval.vcf_freq != 65535) {
    param[6] = gval.vcf_freq * 0.001f;
  }
  if (gval.vcf_reso != 65535) {
    param[7] = gval.vcf_reso * 0.001f;
  }
  if (gval.vcf_env != 65535) {
    param[8] = gval.vcf_env * 0.001f;
  }
  if (gval.vcf_lfo != 65535) {
    param[9] = gval.vcf_lfo * 0.001f;
  }
  if (gval.vcf_vel != 65535) {
    param[10] = gval.vcf_vel * 0.001f;
  }
  if (gval.vcf_att != 65535) {
    param[11] = gval.vcf_att * 0.001f;
  }
  if (gval.vcf_dec != 65535) {
    param[12] = gval.vcf_dec * 0.001f;
  }
  if (gval.vcf_sus != 65535) {
    param[13] = gval.vcf_sus * 0.001f;
  }
  if (gval.vcf_rel != 65535) {
    param[14] = gval.vcf_rel * 0.001f;
  }
  if (gval.env_att != 65535) {
    param[15] = gval.env_att * 0.001f;
  }
  if (gval.env_dec != 65535) {
    param[16] = gval.env_dec * 0.001f;
  }
  if (gval.env_sus != 65535) {
    param[17] = gval.env_sus * 0.001f;
  }
  if (gval.env_rel != 65535) {
    param[18] = gval.env_rel * 0.001f;
  }
  if (gval.lfo_rate != 65535) {
    param[19] = gval.lfo_rate * 0.001f;
  }
  if (gval.vibrato != 65535) {
    param[20] = gval.vibrato * 0.001f;
  }
  if (gval.noise != 65535) {
    param[21] = gval.noise * 0.001f;
  }
  if (gval.octave != 65535) {
    param[22] = gval.octave * 0.001f;
  }
  if (gval.tuning != 65535) {
    param[23] = gval.tuning * 0.001f;
  }
  for (int track = 0; track < NVOICES; track++) {
    if (tval[track].note != zzub::note_value_none) {
      if (tval[track].note == zzub::note_value_off) {
	noteOn(last_note[track], 0);
	last_note[track] = 0;
      } else {
	int oct = tval[track].note / 16 + 1;
	int note = tval[track].note % 16 - 1;
	int midi = oct * 12 + note;
	bool slide = 
	  tval[track].slide != zzub::switch_value_none &&
	  tval[track].slide == zzub::switch_value_on;
	if (tval[track].velocity != 255) {
	  if (!slide) {
	    noteOn(last_note[track], 0);
	  }
	  noteOn(midi, tval[track].velocity);
	  if (slide) {
	    noteOn(last_note[track], 0);
	  }
	  last_note[track] = midi;
	} else {
	  if (!slide) {
	    noteOn(last_note[track], 0);
	  }
	  noteOn(midi, 80);
	  if (slide) {
	    noteOn(last_note[track], 0);
	  }
	  last_note[track] = midi;
	}
      }
    }
  }
  update();
}

bool JX10::process_stereo(float **pin, float **pout, int sampleFrames, int mode) {
  float* out1 = pout[0];
  float* out2 = pout[1];
  int v;
  float o, e, vib, pwm, pb = pbend, ipb = ipbend, gl = glide;
  float x, y, hpf = 0.997f, min = 1.0f, w = 0.0f, ww = noisemix;
  float ff, fe = filtenv, fq = filtq * rezwhl, fx = 1.97f - 0.85f * fq, fz = fzip;
  int k = K;
  unsigned int r;

  vib = (float)sin(lfo);
  ff = filtf + filtwhl + (filtlfo + press) * vib; //have to do again here as way that
  pwm = 1.0f + vib * (modwhl + pwmdep); //below triggers on k was too cheap!
  vib = 1.0f + vib * (modwhl + vibrato);

  while (--sampleFrames >= 0) {
    VOICE *V = voice;
    o = 0.0f;
      
    noise = (noise * 196314165) + 907633515;
    r = (noise & 0x7FFFFF) + 0x40000000; // generate noise + fast convert to float
    w = *(float *)&r;
    w = ww * (w - 3.0f);
      
    if (--k < 0) {
      lfo += dlfo;
      if (lfo > PI) { 
	lfo -= TWOPI;
      }
      vib = (float)sin(lfo);
      ff = filtf + filtwhl + (filtlfo + press) * vib;
      pwm = 1.0f + vib * (modwhl + pwmdep);
      vib = 1.0f + vib * (modwhl + vibrato);
      k = KMAX;
    }
      
    for (v = 0; v < NVOICES; v++) { 
      // for each voice
      e = V->env;
      if (e > SILENCE) { 
	// Sinc-Loop Oscillator
	x = V->p + V->dp;
	if (x > min) {
	  if (x > V->pmax) { 
	    x = V->pmax + V->pmax - x;  
	    V->dp = -V->dp; 
	  }
	  V->p = x;
	  x = V->sin0 * V->sinx - V->sin1; //sine osc
	  V->sin1 = V->sin0;
	  V->sin0 = x;
	  x = x / V->p;
	} else { 
	  V->p = x = - x;  
	  V->dp = V->period * vib * pb; //set period for next cycle
	  V->pmax = (float)floor(0.5f + V->dp) - 0.5f;
	  V->dc = -0.5f * V->lev / V->pmax;
	  V->pmax *= PI;
	  V->dp = V->pmax / V->dp;
	  V->sin0 = V->lev * (float)sin(x);
	  V->sin1 = V->lev * (float)sin(x - V->dp);
	  V->sinx = 2.0f * (float)cos(V->dp);
	  if (x * x > .1f) { 
	    x = V->sin0 / x;
	  } else {
	    x = V->lev; //was 0.01f;
	  }
	}
          
	y = V->p2 + V->dp2; //osc2
	if (y > min) { 
	  if (y > V->pmax2) { 
	    y = V->pmax2 + V->pmax2 - y;  
	    V->dp2 = -V->dp2; 
	  }
	  V->p2 = y;
	  y = V->sin02 * V->sinx2 - V->sin12;
	  V->sin12 = V->sin02;
	  V->sin02 = y;
	  y = y / V->p2;
	} else {
	  V->p2 = y = - y;  
	  V->dp2 = V->period * V->detune * pwm * pb;
	  V->pmax2 = (float)floor(0.5f + V->dp2) - 0.5f;
	  V->dc2 = -0.5f * V->lev2 / V->pmax2;
	  V->pmax2 *= PI;
	  V->dp2 = V->pmax2 / V->dp2;
	  V->sin02 = V->lev2 * (float)sin(y);
	  V->sin12 = V->lev2 * (float)sin(y - V->dp2);
	  V->sinx2 = 2.0f * (float)cos(V->dp2);
	  if (y * y > .1f) {
	    y = V->sin02 / y;
	  } else {
	    y = V->lev2;
	  }
	}
	V->saw = V->saw * hpf + V->dc + x - V->dc2 - y;  //integrated sinc = saw
	x = V->saw + w;
	V->env += V->envd * (V->envl - V->env);
	  
	if (k == KMAX) {
	  // filter freq update at LFO rate
	  if ((V->env + V->envl) > 3.0f) {
	    V->envd = dec; 
	    V->envl = sus; 
	  } //envelopes
	  V->fenv += V->fenvd * (V->fenvl - V->fenv);
	  if ((V->fenv + V->fenvl) > 3.0f) { 
	    V->fenvd = fdec; 
	    V->fenvl = fsus; 
	  }
	    
	  fz += 0.005f * (ff - fz); // filter zipper noise filter
	  y = V->fc * (float)exp(fz + fe * V->fenv) * ipb; // filter cutoff
	  if (y < 0.005f) {
	    y = 0.005f;
	  }
	  V->ff = y;
	    
	  V->period += gl * (V->target - V->period); // glide
	  if (V->target < V->period) {
	    V->period += gl * (V->target - V->period);
	  }
	}

	if (V->ff > fx) {
	  V->ff = fx; //stability limit
	}
          
	V->f0 += V->ff * V->f1; //state-variable filter
	V->f1 -= V->ff * (V->f0 + fq * V->f1 - x - V->f2);
	V->f1 -= 0.2f * V->f1 * V->f1 * V->f1; //soft limit
	  
	V->f2 = x;
          
	o += V->env * V->f0;
      }
      V++;
    }
      
    *out1++ = o;
    *out2++ = o;
  }
  for (v = 0; v < NVOICES; v++) {
    if (voice[v].env < SILENCE) {
      // choke voices
      voice[v].env = voice[v].envl = 0.0f;
      voice[v].f0 = voice[v].f1 = voice[v].f2 = 0.0f;
    }
  }
  fzip = fz;
  K = k;
  return true;
}

const char *JX10::describe_value(int index, int value) {
  static char txt[20];
  switch (index) {
  case 0:
    sprintf(txt, "%4.0f:%2.0f", 100.0 - 50.0f * param[index], 50.0f * param[index]);
    break;
  case 1:
    sprintf(txt, "%.0f semi", semi);
    break;
  case 2:
    sprintf(txt, "%.1f cent", cent);
    break;
  case 3:
    switch (mode) {
    case 0:
    case 1:
      sprintf(txt, "POLY");
      break;
    case 2:
      sprintf(txt, "P-LEGATO");
      break;
    case 3:
      sprintf(txt, "P-GLIDE");
      break;
    case 4:
    case 5:
      sprintf(txt, "MONO");
      break;
    case 6:
      sprintf(txt, "M-LEGATO");
      break;
    default:
      sprintf(txt, "M-GLIDE");
      break;
    }
    break;
  case 5:
    sprintf(txt, "%.2f semi", glidedisp);
    break;
  case 6:
    sprintf(txt, "%.1f%%", 100.0f * param[index]);
    break;
  case 8:
    sprintf(txt, "%.1f%%", 200.0f * param[index] - 100.0f);
    break;
  case 23:
    sprintf(txt, "%.1f cent", 200.0f * param[index] - 100.0f);
    break;
  case 10:
    if (param[index] < 0.05f) {
      sprintf(txt, "OFF");
    } else {
      sprintf(txt, "%.0f%%", 200.0f * param[index] - 100.0f);
    }
    break;
  case 19:
    sprintf(txt, "%.3f Hz", lfoHz);
    break;
  case 20:
    if (param[index] < 0.5f) {
      sprintf(txt, "PWM %3.0f%%", 100.0f - 200.0f * param[index]);
    } else {
      sprintf(txt, "%3.0f%%", 200.0f * param[index] - 100.0f);
    }
    break;
  case 22:
    sprintf(txt, "%d", (int)(param[index] * 4.9f) - 2);
    break;
  case 24:
  case 25:
  case 26:
    return 0;
    break;
  default:
    sprintf(txt, "%.0f%%", 100.0f * param[index]);
    break;
  }
  return txt;
}

void JX10::noteOn(int note, int velocity)
{
  float p, l = 100.0f; // louder than any envelope!
  int v = 0, tmp, held = 0;
  
  if (velocity > 0) {
    // note on
    if (veloff) {
      velocity = 80;
    }
    
    if (mode & 4) {
      // monophonic
      if (voice[0].note > 0) {
	// legato pitch change
        for (tmp = (NVOICES - 1); tmp > 0; tmp--) {
	  // queue any held notes
          voice[tmp].note = voice[tmp - 1].note;
        }
        p = tune * (float)exp(-0.05776226505 * ((double)note + ANALOG * (double)v));
        while (p < 3.0f || (p * detune) < 3.0f) { 
	  p += p;
	}
        voice[v].target = p;
        if ((mode & 2) == 0) {
	  voice[v].period = p;
	}
        voice[v].fc = (float)exp(filtvel * (float)(velocity - 64)) / p;
        voice[v].env += SILENCE + SILENCE; // was missed out below if returned?
        voice[v].note = note;
        return;
      }
    } else {
      // polyphonic 
      for (tmp = 0; tmp < NVOICES; tmp++) {
	// replace quietest voice not in attack
        if (voice[tmp].note > 0) {
	  held++;
	}
        if (voice[tmp].env < l && voice[tmp].envl < 2.0f) { 
	  l = voice[tmp].env;  
	  v = tmp; 
	}
      }
    }  
    p = tune * (float)exp(-0.05776226505 * ((double)note + ANALOG * (double)v));
    while (p < 3.0f || (p * detune) < 3.0f) {
      p += p;
    }
    voice[v].target = p;
    voice[v].detune = detune;
  
    tmp = 0;
    if (mode & 2) {
      if ((mode & 1) || held) {
	tmp = note - lastnote; //glide
      }
    }
    voice[v].period = p * (float)pow(1.059463094359, (double)tmp - glidedisp);
    if (voice[v].period < 3.0f) {
      voice[v].period = 3.0f; // limit min period
    }

    voice[v].note = lastnote = note;

    voice[v].fc = (float)exp(filtvel * (float)(velocity - 64)) / p; // filter tracking

    voice[v].lev = voltrim * volume * (0.004f * (float)((velocity + 64) * (velocity + 64)) - 8.0f);
    voice[v].lev2 = voice[v].lev * oscmix;

    if (param[20] < 0.5f) {
      // force 180 deg phase difference for PWM
      if (voice[v].dp > 0.0f) {
        p = voice[v].pmax + voice[v].pmax - voice[v].p;
        voice[v].dp2 = -voice[v].dp;
      } else {
        p = voice[v].p;
        voice[v].dp2 = voice[v].dp;
      }
      voice[v].p2 = voice[v].pmax2 = p + PI * voice[v].period;

      voice[v].dc2 = 0.0f;
      voice[v].sin02 = voice[v].sin12 = voice[v].sinx2 = 0.0f;
    }

    if(mode & 4) {
      // monophonic retriggering
      voice[v].env += SILENCE + SILENCE;
    } else {
      //if(programs[curProgram].param[15] < 0.28f) 
      //{
      //  voice[v].f0 = voice[v].f1 = voice[v].f2 = 0.0f; //reset filter
      //  voice[v].env = SILENCE + SILENCE;
      //  voice[v].fenv = 0.0f;
      //}
      //else 
      voice[v].env += SILENCE + SILENCE; // anti-glitching trick
    }
    voice[v].envl = 2.0f;
    voice[v].envd = att;
    voice[v].fenvl = 2.0f;
    voice[v].fenvd = fatt;
  } else {
    // note off
    if ((mode & 4) && (voice[0].note == note)) {
      // monophonic (and current note)
      for (v = (NVOICES - 1); v > 0; v--) {
        if (voice[v].note > 0) {
	  held = v; //any other notes queued?
	}
      }
      if (held > 0) {
        voice[v].note = voice[held].note;
        voice[held].note = 0;
        
        p = tune * (float)exp(-0.05776226505 * ((double)voice[v].note + ANALOG * (double)v));
        while (p < 3.0f || (p * detune) < 3.0f) {
	  p += p;
	}
        voice[v].target = p;
        if ((mode & 2) == 0) {
	  voice[v].period = p;
	}
        voice[v].fc = 1.0f / p;
      } else {
        voice[v].envl = 0.0f;
        voice[v].envd = rel;
        voice[v].fenvl = 0.0f;
        voice[v].fenvd = frel;
        voice[v].note = 0;
      }
    } else {
      // polyphonic
      for (v = 0; v < NVOICES; v++) {
	if (voice[v].note == note) {
	  //any voices playing that note?
	  if (sustain == 0) {
	    voice[v].envl = 0.0f;
	    voice[v].envd = rel;
	    voice[v].fenvl = 0.0f;
	    voice[v].fenvd = frel;
	    voice[v].note  = 0;
	  } else {
	    voice[v].note = SUSTAIN;
	  }
	}
      }
    }
  }
}
