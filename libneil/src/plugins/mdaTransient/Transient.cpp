#include <cstdio>
#include <cmath>

#include "Transient.hpp"

Transient::Transient() {
  global_values = &gval;
  track_values = 0;
  attributes = 0;
}

void Transient::init(zzub::archive *pi) {
  fParam1 = (float)0.50; // attack 		
  fParam2 = (float)0.50; // release
  fParam3 = (float)0.50; // output
  fParam4 = (float)0.49; // filter
  fParam5 = (float)0.35; // att-rel
  fParam6 = (float)0.35; // rel-att
  process_events();
}
	
void Transient::process_events() {
  if (gval.attack != 65535) {
    fParam1 = gval.attack * 0.001f;
  }
  if (gval.release != 65535) {
    fParam2 = gval.release * 0.001f;
  }
  if (gval.output != 65535) {
    fParam3 = gval.output * 0.001f;
  }
  if (gval.filter != 65535) {
    fParam4 = gval.filter * 0.001f;
  }
  if (gval.att_rel != 65535) {
    fParam5 = gval.att_rel * 0.001f;
  }
  if (gval.rel_att != 65535) {
    fParam6 = gval.rel_att * 0.001f;
  }
  // calcs here
  dry = (float)(pow(10.0, (2.0 * fParam3) - 1.0));
  if (fParam4 > 0.50) {
    fili = 0.8f - 1.6f * fParam4;
    filo = 1.f + fili;
    filx = 1.f;
  } else {
    fili = 0.1f + 1.8f * fParam4;
    filo = 1.f - fili;
    filx = 0.f;
  }

  if (fParam1 > 0.5) {  
    att1 = (float)pow(10.0, -1.5);
    att2 = (float)pow(10.0, 1.0 - 5.0 * fParam1);
  } else {
    att1 = (float)pow(10.0, -4.0 + 5.0 * fParam1);
    att2 = (float)pow(10.0, -1.5);
  }
  rel12 = 1.f - (float)pow(10.0, -2.0 - 4.0 * fParam5);

  if (fParam2 > 0.5) {
    rel3 = 1.f - (float)pow(10.0, -4.5);
    rel4 = 1.f - (float)pow(10.0, -5.85 + 2.7 * fParam2);
  } else {
    rel3 = 1.f - (float)pow(10.0, -3.15 - 2.7 * fParam2);
    rel4 = 1.f - (float)pow(10.0, -4.5);
  }
  att34 = (float)pow(10.0, - 4.0 * fParam6);
}

bool Transient::process_stereo(float **pin, float **pout, int sampleFrames, int mode) {
  float *in1 = pin[0];
  float *in2 = pin[1];
  float *out1 = pout[0];
  float *out2 = pout[1];
  float a, b, e, f, g, i;
  float e1 = env1, e2 = env2, e3 = env3, e4 = env4, y = dry; 
  float a1 = att1, a2 = att2, r12 = rel12, a34 = att34, r3 = rel3, r4 = rel4;
  float fi = fili, fo = filo, fx = filx, fb1 = fbuf1, fb2 = fbuf2;
  
  --in1;	
  --in2;	
  --out1;
  --out2;
  
  while (--sampleFrames >= 0) {
    a = *++in1;
    b = *++in2;
		
    fb1 = fo * fb1 + fi * a;  
    fb2 = fo * fb2 + fi * b; 
    e = fb1 + fx * a;       
    f = fb2 + fx * b;

    i = a + b; 
    i = (i > 0) ? i : -i; 
    e1 = (i > e1) ? e1 + a1 * (i - e1) : e1 * r12;
    e2 = (i > e2) ? e2 + a2 * (i - e2) : e2 * r12;
    e3 = (i > e3) ? e3 + a34 * (i - e3) : e3 * r3;
    e4 = (i > e4) ? e4 + a34 * (i - e4) : e4 * r4;
    g = (e1 - e2 + e3 - e4);

    *++out1 = y * (a + e * g);	
    *++out2 = y * (b + f * g);	
  }
  if (e1 < 1.0e-10) { 
    env1 = 0.f; 
    env2 = 0.f;
    env3 = 0.f; 
    env4 = 0.f; 
    fbuf1 = 0.f; 
    fbuf2 = 0.f; 
  } else { 
    env1 = e1;  
    env2 = e2;  
    env3 = e3;  
    env4 = e4; 
    fbuf1 = fb1; 
    fbuf2 = fb2; 
  }
  return true;
}

const char *Transient::describe_value(int param, int value) {
  static char txt[20];
  switch (param) {
  case 0:
    sprintf(txt, "%d%%", int(200 * fParam1 - 100));
    break;
  case 1:
    sprintf(txt, "%d%%", int(200 * fParam2 - 100));
    break;
  case 2:
    sprintf(txt, "%d dB", int(40 * fParam3 - 20));
    break;
  case 3:
    sprintf(txt, "%d Lo<>Hi", int(20 * fParam4 - 10));
    break;
  case 4:
    sprintf(txt, "%d%%", int(100 * fParam5));
    break;
  case 5:
    sprintf(txt, "%d%%", int(100 * fParam6));
    break;
  default:
    return 0;
  }
  return txt;
}
