#include <cstdio>
#include <cmath>

#include "Shepard.hpp"

Shepard::Shepard() {
  global_values = &gval;
  track_values = 0;
  attributes = 0;
}

void Shepard::init(zzub::archive *pi) {
  double x, a, twopi = 6.2831853;
  int i;
  
  //inits here!
  fParam0 = 0.20f; //mode
  fParam1 = 0.70f; //rate
  fParam2 = 0.50f; //level

  max = 512;
  buf1 = new float[max]; 
  buf2 = new float[max]; 

  for (max = 0; max < 511; max++) {
    pos = (float)(twopi * max / 511.f); //generate wavetables
    x = 0.0;
    a = 1.0;
    *(buf2 + max) = (float)sin(pos);
    for (i = 0; i < 8; i++) {
      x += a * sin(fmod((double)pos, twopi));
      a *= 0.5;
      pos *= 2.0;
    }
    *(buf1 + max) = (float)x;
  }
  *(buf1 + 511) = 0.f;
  *(buf2 + 511) = 0.f; //wrap end for interpolation
  pos = 0.f; 
  rate = 1.f; 

  process_events(); //go and set initial values!
}

void Shepard::destroy() {
  if (buf1) {
    delete[] buf1;
  }
  if (buf2) {
    delete[] buf2;
  }
}
	
void Shepard::process_events() {
  if (gval.mode != 65535) {
    fParam0 = gval.mode * 0.001f;
  }
  if (gval.rate != 65535) {
    fParam1 = gval.rate * 0.001f;
  }
  if (gval.level != 65535) {
    fParam2 = gval.level * 0.001f;
  }
  //calcs here
  mode = int(2.95f * fParam0);
  drate = 1.f + 10.f * (float)pow(fParam1 - 0.5, 3.0) / _master_info->samples_per_second;
  out = 0.4842f * (float)pow(10.0f, 2.f * fParam2 - 1.f);
}

bool Shepard::process_stereo(float **pin, float **pout, int sampleFrames, int mode_) {
  float *in1 = pin[0];
  float *in2 = pin[1];
  float *out1 = pout[0];
  float *out2 = pout[1];
  float a, b;
  float r = rate, dr = drate, o = out, p = pos, di;
  int x = max, m = mode, i1, i2;

  --in1;	
  --in2;	
  --out1;
  --out2;

  while (--sampleFrames >= 0) {
    a = *++in1 + *++in2;

    r *= dr; 
    if (r > 2.f) { 
      r *= 0.5f; 
      p *= 0.5f;
    } else if (r < 1.f) {
      r *= 2.f;
      p *= 2.f; 
      if (p > x) {
	p -= x;
      }
    }
    
    p += r; 
    if (p > x) {
      p -= x;
    }
    
    i1 = int(p); //interpolate position
    i2 = i1 + 1;
    di = (float)i2 - p;

    b = di  * (*(buf1 + i1) + (r - 2.f) * *(buf2 + i1));
    b += (1.f - di) * (*(buf1 + i2) + (r - 2.f) * *(buf2 + i2));
    b *= o / r;

    if (m > 0) {
      if (m == 2) {
	b += 0.5f * a;
      } else  {
	b *= a; //ring mod or add
      }
    }
    *++out1 = b;
    *++out2 = b;
  }
  pos = p; 
  rate = r;
  return true;
}

const char *Shepard::describe_value(int param, int value) {
  static char txt[20];
  switch (param) {
  case 0:
    switch (mode) {
    case 0:
      sprintf(txt, "TONES");
      break;
    case 1:
      sprintf(txt, "RING MOD");
      break;
    case 2:
      sprintf(txt, "TONES+IN");
      break;
    }
    break;
  case 1:
    sprintf(txt, "%d%%", int(200 * fParam1 - 100));
    break;
  case 2:
    sprintf(txt, "%d dB", int(40 * fParam2 - 20));
    break;
  default:
    return 0;
  }
  return txt;
}
