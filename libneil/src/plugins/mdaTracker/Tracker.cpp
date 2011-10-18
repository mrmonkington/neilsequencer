#include <cstdio>
#include <cmath>

#include "Tracker.hpp"

Tracker::Tracker() {
  global_values = &gval;
  track_values = 0;
  attributes = 0;
}

void Tracker::init(zzub::archive *pi) {
  dphi = 100.f / _master_info->samples_per_second; //initial pitch
  min = (int)(_master_info->samples_per_second / 30.0); //lower limit
  res1 = (float)cos(0.01); //p
  res2 = (float)sin(0.01); //q
}

float Tracker::filterFreq(float hz)
{
  float j, k, r = 0.999f;
  j = r * r - 1;
  k = (float)(2.f - 2.f * r * r * cos(0.647f * hz / _master_info->samples_per_second));
  return (float)((sqrt(k * k - 4.f * j * j) - k) / (2.f * j));
}
	
void Tracker::process_events() {
  if (gval.mode != 65535) {
    fParam1 = gval.mode * 0.001f;
  }
  if (gval.dynamics != 65535) {
    fParam2 = gval.dynamics * 0.001f;
  }
  if (gval.mix != 65535) {
    fParam3 = gval.mix * 0.001f;
  }
  if (gval.tracking != 65535) {
    fParam4 = gval.tracking * 0.001f;
  }
  if (gval.trnspose != 65535) {
    fParam5 = gval.trnspose * 0.001f;
  }
  if (gval.maximum_hz != 65535) {
    fParam6 = gval.maximum_hz * 0.001f;
  }
  if (gval.trigger_db != 65535) {
    fParam7 = gval.trigger_db * 0.001f;
  }
  if (gval.output != 65535) {
    fParam8 = gval.output * 0.001f;
  }
  mode = int(fParam1 * 4.9); 
  fo = filterFreq(50.f); 
  fi = (1.f - fo) * (1.f - fo);
  ddphi = fParam4 * fParam4;
  thr = (float)pow(10.0, 3.0 * fParam7 - 3.8);
  max = (int)(_master_info->samples_per_second / pow(10.0f, 1.6f + 2.2f * fParam6));
  trans = (float)pow(1.0594631, int(72.f * fParam5 - 36.f));
  wet = (float)pow(10.0, 2.0 * fParam8 - 1.0);
  if(mode < 4) {
    dyn = wet * 0.6f * fParam3 * fParam2;
    dry = wet * (float)sqrt(1.f - fParam3);
    wet = wet * 0.3f * fParam3 * (1.f - fParam2);
  } else {
    dry = wet * (1.f - fParam3);
    wet *= (0.02f * fParam3 - 0.004f);
    dyn = 0.f;
  }
  rel = (float)pow(10.0, -10.0 / _master_info->samples_per_second);
}

bool Tracker::process_stereo(float **pin, float **pout, int sampleFrames, int mode) {
  float *in1 = pin[0];
  float *in2 = pin[1];
  float *out1 = pout[0];
  float *out2 = pout[1];
  float a, b, c, d, x, t = thr, p = phi, dp = dphi, ddp = ddphi, tmp, tmp2;
  float o = fo, i = fi, b1 = buf1, b2 = buf2, twopi = 6.2831853f;
  float we = wet, dr = dry, bo = bold, r1 = res1, r2 = res2, b3 = buf3, b4 = buf4;
  float sw = saw, dsw = dsaw, dy = dyn, e = env, re = rel;
  int m = max, n = num, s = sig, mn = min, mo = mode;
  --in1;	
  --in2;	
  --out1;
  --out2;
  while (--sampleFrames >= 0) {
    a = *++in1;
    b = *++in2;
    c = out1[1];
    d = out2[1]; //process from here...
    x = a + b;
    tmp = (x > 0.f) ? x : -x; //dynamics envelope
    e = (tmp > e) ? 0.5f * (tmp + e) : e * re;

    b1 = o * b1 + i * x; 
    b2 = o * b2 + b1; //low-pass filter
    
    if (b2 > t) {  
      //if >thresh
      if (s < 1) {
	//and was <thresh
        if (n < mn) {
	  //not long ago
          tmp2 = b2 / (b2 - bo); //update period
          tmp = trans * twopi / (n + dn - tmp2); 
          dp = dp + ddp * (tmp - dp); 
          dn = tmp2;
          dsw = 0.3183098f * dp;
          if (mode == 4) {
            r1 = (float)cos(4.f * dp); //resonator
            r2 = (float)sin(4.f * dp);
          }  
        }
        n = 0; //restart period measurement
      }
      s = 1;
    } else {
      if (n > m) { 
	s = 0; //now <thresh 
      }
    }
    n++;
    bo = b2;
    p = (float)fmod(p + dp, twopi);
    switch(mo){ 
      //sine
      case 0: 
	x = (float)sin(p); 
	break; 
      //square
      case 1: 
	x = (sin(p) > 0.f) ? 0.5f : -0.5f; 
	break; 
      //saw
      case 2: 
	sw = (float)fmod(sw + dsw, 2.0f); 
	x = sw - 1.f; 
	break; 
      //ring
      case 3: 
	x *= (float)sin(p); 
	break; 
      //filt
      case 4: x += (b3 * r1) - (b4 * r2); 
	b4 = 0.996f * ((b3 * r2) + (b4 * r1)); 
	b3 = 0.996f * x; 
	break; 
    }    
    x *= (we + dy * e); 
    *++out1 = dr * a + x;
    *++out2 = dr * b + x;
  }
  if (fabs(b1) < 1.0e-10) {
    buf1 = 0.f; 
    buf2 = 0.f; 
    buf3 = 0.f; 
    buf4 = 0.f; 
  } else {
    buf1 = b1; 
    buf2 = b2;
    buf3 = b3; 
    buf4 = b4;
  }
  phi = p; dphi = dp; sig = s; bold = bo;
  num = (n > 100000) ? 100000 : n; 
  env = e; saw = sw; dsaw = dsw; res1 = r1; res2 = r2;
  return true;
}

const char *Tracker::describe_value(int param, int value) {
  static char txt[20];
  switch (param) {
  case 0:
    switch (mode) {
    case 0:
      sprintf(txt, "SINE");
      break;
    case 1:
      sprintf(txt, "SQUARE");
      break;
    case 2:
      sprintf(txt, "SAW");
      break;
    case 3:
      sprintf(txt, "RING");
      break;
    case 4:
      sprintf(txt, "EQ");
      break;
    default:
      sprintf(txt, "???");
      break;
    }
    break;
  case 1:
    sprintf(txt, "%d%%", int(100 * fParam2));
    break;
  case 2:
    sprintf(txt, "%d%%", int(100 * fParam3));
    break;
  case 3:
    sprintf(txt, "%d%%", int(100 * fParam4));
    break;
  case 4:
    sprintf(txt, "%d semi", int(72 * fParam5 - 36));
    break;
  case 5:
    sprintf(txt, "%d Hz", int(_master_info->samples_per_second / max));
    break;
  case 6:
    sprintf(txt, "%d dB", int(60 * fParam7 - 60));
    break;
  case 7:
    sprintf(txt, "%d dB", int(40 * fParam8 - 20));
    break;
  default:
    return 0;
  }
  return txt;
}
