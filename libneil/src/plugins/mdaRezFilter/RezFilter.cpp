#include <cstdio>
#include <cmath>
#include <cstdlib>

#include "RezFilter.hpp"

RezFilter::RezFilter() {
  global_values = &gval;
  track_values = 0;
  attributes = 0;
}

void RezFilter::init(zzub::archive *pi) {
  fParam0 = 0.33f; // f
  fParam1 = 0.70f; // q
  fParam2 = 0.50f; // a
  fParam3 = 0.85f; // fenv
  fParam4 = 0.00f; // att
  fParam5 = 0.50f; // rel
  fParam6 = 0.70f; // lfo
  fParam7 = 0.40f; // rate
  fParam8 = 0.00f; // trigger
  fParam9 = 0.75f; // max freq
  buf0 = 0.f; 
  buf1 = 0.f; 
  buf2 = 0.f;
  process_events(); //go and set initial values!
}
	
void RezFilter::process_events() {
  if (gval.f != 65535) {
    fParam0 = gval.f * 0.001f;
  }
  if (gval.q != 65535) {
    fParam1 = gval.q * 0.001f;
  }
  if (gval.a != 65535) {
    fParam2 = gval.a * 0.001f;
  }
  if (gval.fenv != 65535) {
    fParam3 = gval.fenv * 0.001f;
  }
  if (gval.att != 65535) {
    fParam4 = gval.att * 0.001f;
  }
  if (gval.rel != 65535) {
    fParam5 = gval.rel * 0.001f;
  }
  if (gval.lfo != 65535) {
    fParam6 = gval.lfo * 0.001f;
  }
  if (gval.rate != 65535) {
    fParam7 = gval.rate * 0.001f;
  }
  if (gval.trigger != 65535) {
    fParam8 = gval.trigger * 0.001f;
  }
  if (gval.max_freq != 65535) {
    fParam9 = gval.max_freq * 0.001f;
  }
  //calcs here
  fff = 1.5f * fParam0 * fParam0 - 0.15f;
  fq = 0.99f * (float)pow(fParam1, 0.3f);
  fg = 0.5f * (float)pow(10.0f, 2.f * fParam2 - 1.f);
  
  fmax = 0.99f + 0.3f * fParam1;
  if (fmax > (1.3f * fParam9)) {
    fmax = 1.3f * fParam9; 
  }

  fenv = 2.f * (0.5f - fParam3) * (0.5f - fParam3); 
  fenv = (fParam3 > 0.5f) ? fenv : -fenv;
  att = (float)pow(10.0, -0.01 - 4.0 * fParam4);
  rel = 1.f - (float)pow(10.0, -2.00 - 4.0 * fParam5);

  lfomode = 0;
  flfo = 2.f * (fParam6 - 0.5f) * (fParam6 - 0.5f); 
  dphi = (float)(6.2832f * (float)pow(10.0f, 3.f * fParam7 - 1.5f) / _master_info->samples_per_second);
  if (fParam6 < 0.5) {
    lfomode = 1; 
    dphi *= 0.15915f; 
    flfo *= 0.001f; 
  } //S&H

  if (fParam8 < 0.1f) {
    tthr = 0.f;
  } else {
    tthr = 3.f * fParam8 * fParam8;
  }
}

bool RezFilter::process_stereo(float **pin, float **pout, int sampleFrames, int mode) {
  float *in1 = pin[0];
  float *in2 = pin[1];
  float *out1 = pout[0];
  float *out2 = pout[1];
  float a;	
  float f, i, ff = fff, fe = fenv, q = fq, g = fg, e = env, tmp;
  float b0 = buf0, b1 = buf1, b2 = buf2, at = att, re = rel, fm = fmax;
  float fl = flfo, dph = dphi, ph = phi, bl = bufl, th = tthr, e2 = env2;
  int lm = lfomode, ta = tatt, tt = ttrig;
  
  --in1;	
  --in2;	
  --out1;
  --out2;
  
  if (th == 0.f) {
    while (--sampleFrames >= 0) {
      a = *++in1 + *++in2;
    
      i = (a > 0) ? a : -a; //envelope
      e = (i > e) ? e + at * (i - e) : e * re; 
    
      if (lm == 0) {
	bl = fl * (float)sin(ph); //lfo
      } else if (ph > 1.f) {
	bl = fl * (rand() % 2000 - 1000); 
	ph=0.f; 
      }
      ph += dph;

      f = ff + fe * e + bl; //freq
      if (f < 0.f) {
	i = 0.f;
      } else {
	i = (f > fm) ? fm : f;
      }
      tmp = q + q * (1.0f + i * (1.0f + 1.1f * i));
      b0 += i * (g * a - b0 + tmp * (b0 - b1));
      b1 += i * (b0 - b1);

      *++out1 = b1;
      *++out2 = b1;
    }
  } else {
    while (--sampleFrames >= 0) {
      a = *++in1 + *++in2;

      i = (a > 0) ? a : -a; //envelope
      e = (i > e) ? i : e * re; 
      if (e > th) { 
	if (tt == 0) {
	  ta = 1; 
	  if (lm == 1) {
	    ph = 2.f;
	  } 
	} 
	tt = 1; 
      } else {
	tt = 0; 
      }
      if (ta == 1) { 
	e2 += at * (1.f - e2); 
	if (e2 > 0.999f) {
	  ta = 0; 
	}
      } else {
	e2 *= re;
      }

      if (lm == 0) {
	bl = fl * (float)sin(ph); //lfo
      } else if (ph > 1.f) { 
	bl = fl*(rand() % 2000 - 1000); 
	ph = 0.f; 
      }
      ph += dph;

      f = ff + fe * e + bl; //freq
      if (f < 0.f) {
	i = 0.f;
      } else {
	i = (f > fm)? fm : f;
      }
      tmp = q + q * (1.0f + i * (1.0f + 1.1f * i));
      b0 += i * (g * a - b0 + tmp * (b0 - b1));
      b1 += i * (b0 - b1);
      *++out1 = b1;
      *++out2 = b1;
    }
  }
  if (fabs(b0) < 1.0e-10) { 
    buf0 = 0.f; 
    buf1 = 0.f; 
    buf2 = 0.f; 
  } else { 
    buf0 = b0; 
    buf1 = b1; 
    buf2 = b2; 
  } 
  env = e; 
  env2 = e2;
  bufl = bl; 
  tatt = ta; 
  ttrig = tt;
  phi = (float)fmod(ph, 6.2831853f);
  return true;
}

const char *RezFilter::describe_value(int param, int value) {
  static char txt[20];
  switch (param) {
  case 0:
    sprintf(txt, "%d%%", int(100 * fParam0));
    break;
  case 1:
    sprintf(txt, "%d%%", int(100 * fParam1));
    break;
  case 2:
    sprintf(txt, "%d dB", int(40 * fParam2 - 20));
    break;
  case 3:
    sprintf(txt, "%d%%", int(200 * fParam3 - 100));
    break;
  case 4:
    sprintf(txt, "%.2f ms", -301.0301 / (_master_info->samples_per_second * log10(1.0 - att)));
    break;
  case 5:
    sprintf(txt, "%.2f ms", -301.0301 / (_master_info->samples_per_second * log10(rel)));
    break;
  case 6:
    sprintf(txt, "%d SH/Sin", int(200 * fParam6 - 100));
    break;
  case 7:
    sprintf(txt, "%.2f Hz", pow(10.0f, 4.f * fParam7 - 2.f));
    break;
  case 8:
    if (tthr == 0.f) {
      sprintf(txt, "FREE RUN");
    } else {
      sprintf(txt, "%d dB", int(20 * log10(0.5 * tthr)));
    }
    break;
  case 9:
    sprintf(txt, "%d%%", int(100 * fParam9));
    break;
  default:
    return 0;
  }
  return txt;
}
