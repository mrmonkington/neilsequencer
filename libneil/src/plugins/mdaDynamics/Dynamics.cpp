#include <cstdio>
#include <cmath>

#include "Dynamics.hpp"

Dynamics::Dynamics() {
  global_values = &gval;
  track_values = 0;
  attributes = 0;
}

void Dynamics::init(zzub::archive *pi) {
  fParam1 = (float)0.60; // thresh
  fParam2 = (float)0.40; // ratio
  fParam3 = (float)0.10; // level
  fParam4 = (float)0.18; // attack
  fParam5 = (float)0.55; // release
  fParam6 = (float)1.00; // limiter
  fParam7 = (float)0.00; // gate thresh
  fParam8 = (float)0.10; // gate attack
  fParam9 = (float)0.50; // gate decay
  fParam10 = (float)1.00; // fx mix
  process_events();
}
	
void Dynamics::process_events() {
  if (gval.thresh != 65535) {
    fParam1 = gval.thresh * 0.001f;
  }
  if (gval.ratio != 65535) {
    fParam2 = gval.ratio * 0.001f;
  }
  if (gval.level != 65535) {
    fParam3 = gval.level * 0.001f;
  }
  if (gval.attack != 65535) {
    fParam4 = gval.attack * 0.001f;
  }
  if (gval.release != 65535) {
    fParam5 = gval.release * 0.001f;
  }
  if (gval.limiter != 65535) {
    fParam6 = gval.limiter * 0.001f;
  }
  if (gval.gatethresh != 65535) {
    fParam7 = gval.gatethresh * 0.001f;
  }
  if (gval.gateattack != 65535) {
    fParam8 = gval.gateattack * 0.001f;
  }
  if (gval.gatedecay != 65535) {
    fParam9 = gval.gatedecay * 0.001f;
  }
  if (gval.fxmix != 65535) {
    fParam10 = gval.fxmix * 0.001f;
  }
  // calcs here
  mode = 0;
  thr = (float)pow(10.f, 2.f * fParam1 - 2.f);
  rat = 2.5f * fParam2 - 0.5f; 
  if (rat > 1.0) { 
    rat = 1.f + 16.f * (rat - 1.f) * (rat - 1.f); 
    mode = 1; 
  }
  if (rat < 0.0) { 
    rat = 0.6f * rat; 
    mode = 1; 
  }
  trim = (float)pow(10.f, 2.f * fParam3);
  att = (float)pow(10.f, -0.002f - 2.f * fParam4);
  rel = (float)pow(10.f, -2.f - 3.f * fParam5);

  if (fParam6 > 0.98) {
    lthr = 0.f; //limiter
  } else { 
    lthr = 0.99f * (float)pow(10.0f, int(30.0 * fParam6 - 20.0) / 20.f); 
    mode = 1; 
  } 
  
  if (fParam7 < 0.02) { 
    // expander
    xthr = 0.f; 
  } else { 
    xthr = (float)pow(10.f, 3.f * fParam7 - 3.f); 
    mode = 1; 
  } 
  xrat = 1.f - (float)pow(10.f, -2.f - 3.3f * fParam9);
  irel = (float)pow(10.0, -2.0 / _master_info->samples_per_second);
  gatt = (float)pow(10.f, -0.002f - 3.f * fParam8);

  if (rat < 0.0f && thr < 0.1f) {
    rat *= thr * 15.f;
  }

  dry = 1.0f - fParam10;  
  trim *= fParam10; // fx mix
}

bool Dynamics::process_stereo(float **pin, float **pout, int sampleFrames, int pmode) {
  if (pmode == zzub::process_mode_write || pmode == zzub::process_mode_no_io) {
    return false;
  }
  if (pmode == zzub::process_mode_read) {
    return true;
  }
  float *in1 = pin[0];
  float *in2 = pin[1];
  float *out1 = pout[0];
  float *out2 = pout[1];
  float a, b, i, j, g, e = env, e2 = env2, ra = rat, re = (1.f - rel), at = att, ga = gatt;
  float tr = trim, th = thr, lth = lthr, xth = xthr, ge = genv, y = dry;  
  --in1;	
  --in2;	
  --out1;
  --out2;

  if (mode) {
    // comp/gate/lim
    if (lth == 0.f) {
      lth = 1000.f;
    }
    while (--sampleFrames >= 0) {
      a = *++in1;
      b = *++in2;
      
      i = (a < 0.f) ? -a : a;
      j = (b < 0.f) ? -b : b;
      i = (j > i) ? j : i;
        
      e = (i > e) ? e + at * (i - e) : e * re;
      e2 = (i > e) ? i : e2 * re; // ir;
      
      g = (e > th) ? tr / (1.f + ra * ((e / th) - 1.f)) : tr;

      if (g < 0.f) {
	g = 0.f; 
      }
      if (g * e2 > lth) {
	g = lth / e2; // limit 
      }

      ge = (e > xth) ? ge + ga - ga * ge : ge * xrat; // gate

      *++out1 = a * (g * ge + y);	
      *++out2 = b * (g * ge + y);
    }
  } else {
    // compressor only
    while(--sampleFrames >= 0) {
      a = *++in1;
      b = *++in2;
      
      i = (a < 0.f) ? -a : a;
      j = (b < 0.f) ? -b : b;
      i = (j > i) ? j : i; // get peak level
        
      e = (i > e) ? e + at * (i - e) : e * re; // envelope
      g = (e > th) ? tr / (1.f + ra * ((e / th) - 1.f)) : tr; // gain

      *++out1 = a * (g + y); // vca
      *++out2 = b * (g + y);	
    }
  }
  if(e < 1.0e-10) {
    env = 0.f;
  } else {
    env = e;
  }
  if (e2 < 1.0e-10) {
    env2 = 0.f;
  } else {
    env2 = e2;
  }
  if (ge < 1.0e-10) {
    genv = 0.f;
  } else {
    genv = ge;
  }
  return true;
}

const char *Dynamics::describe_value(int index, int value) {
  static char txt[20];
  switch (index) {
  case 0:
    sprintf(txt, "%d dB", int(40.0 * fParam1 - 40.0));
    break;
  case 1:
    if (fParam2 > 0.58) {
      if (fParam2 < 0.62) {
	sprintf(txt, "Limit");
      } else {
	sprintf(txt, "%.2f:1", -rat);
      }
    } else {
      if (fParam2 < 0.2) {
	sprintf(txt, "%.2f:1", 0.5f + 2.5f * fParam2);
      } else {
	sprintf(txt, "%.2f:1", 1.0f / (1.0f - rat));
      }
    }
    break;
  case 2:
    sprintf(txt, "%d dB", int(40.0 * fParam3));
    break;
  case 3:
    sprintf(txt, "%d ms", int(-301030.1 / (_master_info->samples_per_second * log10(1.0 - att))));
    break;
  case 4:
    sprintf(txt, "%d ms", int(-301.0301 / (_master_info->samples_per_second * log10(1.0 - rel))));
    break;
  case 5:
    if (lthr == 0.0f) {
      sprintf(txt, "OFF");
    } else {
      sprintf(txt, "%d dB", int(30.0 * fParam6 - 20.0));
    }
    break;
  case 6:
    if (xthr == 0.0f) {
      sprintf(txt, "OFF");
    } else {
      sprintf(txt, "%d dB", int(60.0 * fParam7 - 60.0));
    }
    break;
  case 7:
    sprintf(txt, "%d ms", int(-301030.1 / (_master_info->samples_per_second * log10(1.0 - gatt))));
    break;
  case 8:
    sprintf(txt, "%d ms", int(-1806.0 / (_master_info->samples_per_second * log10(xrat))));
    break;
  case 9:
    sprintf(txt, "%d%%", int(100.0 * fParam10));
    break;
  default:
    return 0;
  }
  return txt;
}
