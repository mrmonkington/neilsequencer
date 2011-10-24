#include <cstdio>
#include <cmath>

#include "Envelope.hpp"

Envelope::Envelope() {
  global_values = &gval;
  track_values = 0;
  attributes = 0;
}

void Envelope::init(zzub::archive *pi) {
  fParam1 = 0.00f; //mode
  fParam2 = 0.25f; //attack
  fParam3 = 0.40f; //release
  fParam4 = 0.50f; //output
  env = 0.f; 
  drel = 0.f;
  process_events();
}
	
void Envelope::process_events() {
  if (gval.mode != 65535) {
    fParam1 = gval.mode * 0.001f;
  }
  if (gval.attack != 65535) {
    fParam2 = gval.attack * 0.001f;
  }
  if (gval.release != 65535) {
    fParam3 = gval.release * 0.001f;
  }
  if (gval.output != 65535) {
    fParam4 = gval.output * 0.001f;
  }
  mode = int(fParam1 * 2.9);
  att = (float)pow(10.f, -0.002f - 4.f * fParam2);       
  rel = 1.f - (float)pow(10.f, -2.f - 3.f * fParam3);
  out = (float)pow(10.f, 2.f * fParam4-1.f);
  if (mode) {
    out *= 0.5f;
  } else {
    out *= 2.0f;
  }
}

bool Envelope::process_stereo(float **pin, float **pout, int sampleFrames, int mode) {
  float *in1 = pin[0];
  float *in2 = pin[1];
  float *out1 = pout[0];
  float *out2 = pout[1];
  float a, b, x;
  float e = env, at = att, re = rel, dre = drel, db = out;	
  long flat;

  if (dre > (1.0f - re)) {
    dre = (1.0f - re); //can be unstable if re edited
  }
  flat = (mode == 2) ?  1 : 0; //flatten output audio?

  --in1;	
  --in2;	
  --out1;
  --out2;

  if (mode) {
    //Envelope follower mode
    while (--sampleFrames >= 0) {
      b = *++in1 + *++in2;
      x = (b < 0.f) ? -b : b; //rectify

      if (x > e) {
        e += at * (x - e); //attack + reset rate
        dre = 1.0f - re;
      } else {
        e *= (re + dre); //release
        dre *= 0.9999f; //increase release rate
      }

      x = db * b; //VCA
      if (flat) {
        if (e < 0.01f) {
	  x *= 100;
	} else {
	  x /= e; //flatten audio signal
	}
      }
       
      *++out2 = 0.5f * e; //envelope out
      *++out1 = x; //audio out
    }
  } else {
    // VCA mode
    while (--sampleFrames >= 0) {
      a = *++in1; 
      b = *++in2;

      x = (b < 0.f)? -b : b; //rectify

      if (x > e){
        e += at * (x - e); //attack + reset rate
        dre = 1.0f - re;
      } else {
        e *= (re + dre); //release
        dre *= 0.9999f; //increase release rate
      }

      x = db * a * e; //VCA
 
      *++out1 = x;	
      *++out2 = x;
    }
  }
  if (fabs(e) > 1.0e-10) {
    env = e;
  } else {
    env = 0.0f;
  }
  if (fabs(dre) > 1.0e-10) {
    drel = dre;
  } else {
    drel = 0.0f;
  }
  return true;
}

const char *Envelope::describe_value(int param, int value) {
  static char txt[20];
  switch (param) {
  case 0:
    switch (mode) {
    case 0:
      sprintf(txt, "L x |R|");
      break;
    case 1:
      sprintf(txt, "IN/ENV");
      break;
    case 2:
      sprintf(txt, "FLAT/ENV");
      break;
    }
    break;
  case 1:
    sprintf(txt, "%d ms", int(pow(10.0f, 3.0f * fParam2)));
    break;
  case 2:
    sprintf(txt, "%d ms", int(pow(10.0f, 4.0f * fParam3)));
    break;
  case 3:
    sprintf(txt, "%d dB", int(40 * fParam4 - 20));
    break;
  default:
    return 0;
  }
  return txt;
}
