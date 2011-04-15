#include <cmath>
#include <cstdio>

#include "SubSynth.hpp"

SubSynth::SubSynth() {
  global_values = &gval;
  attributes = 0;
  track_values = 0;
  phi = env = filt1 = filt2 = filt3 = filt4 = filti = filto = 0.0f;
}

void SubSynth::init(zzub::archive *pi) {

}
	
void SubSynth::process_events() {
  if (gval.type != 0xff) {
    fParam1 = gval.type * 0.01f;
  }
  if (gval.level != 0xff) {
    fParam2 = gval.level * 0.01f;
  }
  if (gval.tune != 0xff) {
    fParam3 = gval.tune * 0.01f;
  }
  if (gval.dry_mix != 0xff) {
    fParam4 = gval.dry_mix * 0.01f;
  }
  if (gval.thresh != 0xff) {
    fParam5 = gval.thresh * 0.01f;
  }
  if (gval.release != 0xff) {
    fParam6 = gval.release * 0.01f;
  }
  dvd = 1.0;
  phs = 1.0;
  osc = 0.0; //oscillator phase
  typ = int(3.5 * fParam1);
  filti = (typ == 3)? 0.018f : (float)pow(10.0,-3.0 + (2.0 * fParam3));
  filto = 1.0f - filti;
  wet = fParam2;
  dry = fParam4;
  thr = (float)pow(10.0,-3.0 + (3.0 * fParam5));
  rls = (float)(1.0 - pow(10.0, -2.0 - (3.0 * fParam6)));
  dphi = (float)(0.456159 * pow(10.0,-2.5 + (1.5 * fParam3)));
}

bool SubSynth::process_stereo(float **pin, float **pout, 
			      int sampleFrames, int mode) {
  float *in1 = pin[0];
  float *in2 = pin[1];
  float *out1 = pout[0];
  float *out2 = pout[1];
  float a, b, c, d;	
  float we, dr, fi, fo, f1, f2, f3, f4, sub, rl, th, dv, ph, phii, dph, os, en;
  dph = dphi;
  rl = rls;
  phii = phi;
  en = env;
  os = osc;
  th = thr;
  dv = dvd;
  ph = phs;
  we = wet;
  dr = dry;
  f1 = filt1;
  f2 = filt2;
  f3 = filt3;
  f4 = filt4;
  fi = filti;
  fo = filto;
  --in1;	
  --in2;	
  --out1;
  --out2;
  while(--sampleFrames >= 0) {
    a = *++in1;		
    b = *++in2; //process from here...	
    f1 = (fo * f1) + (fi * (a + b));
    f2 = (fo * f2) + (fi * f1);
    sub = f2;
    if (sub > th) {
      sub = 1.0;        
    } else {
      if (sub < -th) {
        sub = -1.0;
      } else {
        sub = 0.0;
      }
    }
    if ((sub * dv) < 0) {
      dv = -dv; 
      if (dv < 0.0) {
	ph = -ph;
      }
    }
    if (typ == 1) {
      //divide
      sub = ph * sub;
    }
    if (typ == 2) {
      //invert
      sub = (float)(ph * f2 * 2.0);
    }
    if (typ == 3) {
      //osc
      if (f2 > th) {
	en = 1.0; 
      } else {
	en = en * rl;
      }
      sub = (float)(en * sin(phii));
      phii = (float)fmod( phii + dph, 6.283185f );
    }
    f3 = (fo * f3) + (fi * sub);
    f4 = (fo * f4) + (fi * f3);
    c = (a * dr) + (f4 * we); // output
    d = (b * dr) + (f4 * we);
    *++out1 = c;
    *++out2 = d;
  }
  if (fabs(f1) < 1.0e-10) {
    filt1 = 0.f;
  } else {
    filt1 = f1;
  }
  if (fabs(f2) < 1.0e-10) {
    filt2 = 0.f; 
  } else {
    filt2 = f2;
  }
  if (fabs(f3) < 1.0e-10) {
    filt3 = 0.f;
  } else {
    filt3 = f3;
  }
  if (fabs(f4) < 1.0e-10) { 
    filt4 = 0.f;
  } else {
    filt4 = f4;
  }
  dvd = dv;
  phs = ph;
  osc = os;
  phi = phii;
  env = en;
  return true;
}

const char *SubSynth::describe_value(int param, int value) {
  static char txt[20];
  switch (param) {
  case 0:
    switch (typ) {
    case 0:
      sprintf(txt, "Distort");
      break;
    case 1:
      sprintf(txt, "Divide");
      break;
    case 2:
      sprintf(txt, "Invert");
      break;
    case 3:
      sprintf(txt, "Key Osc.");
      break;
    }
    break;
  case 1:
    sprintf(txt, "%d %%", int(100.0f * wet));
    break;
  case 2:
    sprintf(txt, "%d Hz", int((0.0726 * _master_info->samples_per_second * 
			       pow(10.0,-2.5 + (1.5 * fParam3)))));
    break;
  case 3:
    sprintf(txt, "%d %%", int(100.0f * dry));
    break;
  case 4:
    sprintf(txt, "%.2f dB", 60.0f * fParam5 - 60.0f);
    break;
  case 5:
    sprintf(txt, "%d ms", int(-301.03 / 
			      (_master_info->samples_per_second * 
			       log10(rls))));
    break;
  default:
    return 0;
  }
  return txt;
}
