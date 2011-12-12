#include <cstdio>
#include <cmath>
#include <cstdlib>

#include "VocInput.hpp"

VocInput::VocInput() {
  global_values = &gval;
  track_values = 0;
  attributes = 0;
}

void VocInput::init(zzub::archive *pi) {
  param[0] = 0.25f; // Tracking Off / On / Quant
  param[1] = 0.50f; // Pitch
  param[2] = 0.20f; // Breath Noise
  param[3] = 0.50f; // Voiced/Unvoiced Thresh
  param[4] = 0.35f; // Max Freq
  suspend();
  resume();
}

void VocInput::resume() // update internal parameters...
{
  float fs, ifs;
  fs = _master_info->samples_per_second;
  ifs = 1.0f / fs;

  track = (int)(2.99f * param[0]);
  pmult = (float)pow(1.0594631f, floor(48.0f * param[1] - 24.0f));
  if (track == 0) {
    pstep = 110.0f * pmult * ifs;
  }

  noise = 6.0f * param[2];
  lfreq = 660.0f * ifs;
  minp = (float)pow(16.0f, 0.5f - param[4]) * fs / 440.0f;
  maxp = 0.03f * fs;
  root = log10(8.1757989f * ifs);
  vuv = param[3] * param[3];
}

void VocInput::suspend() // clear any buffers...
{
  lbuf0 = lbuf1 = lbuf2 = lbuf3 = 0.0f;
  pstep = sawbuf = lenv = 0.0f;
}

void VocInput::midi2string(int n, char *text)
{
  char t[8];
  int o, s, p = 0;

  t[p++] = ' ';
  t[p++] = ' ';
  t[p++] = ' ';

  o = int(n / 12.f); 
  s = n - (12 * o); 
  o -= 2;

  switch (s)
  {
    case 0: 
      t[p++] = 'C';            
      break;
    case 1: 
      t[p++] = 'C'; 
      t[p++] = '#'; 
      break;
    case 2: 
      t[p++] = 'D';             
      break;
    case 3: 
      t[p++] = 'D'; 
      t[p++] = '#'; 
      break;
    case 4: 
      t[p++] = 'E';             
      break;
    case 5: 
      t[p++] = 'F';             
      break;
    case 6: 
      t[p++] = 'F'; 
      t[p++] = '#'; 
      break;
    case 7: 
      t[p++] = 'G';            
      break;
    case 8: 
      t[p++] = 'G'; 
      t[p++] = '#'; 
      break;
    case 9: 
      t[p++] = 'A';             
      break;
    case 10: 
      t[p++] = 'A'; 
      t[p++] = '#'; 
      break;
    default: 
      t[p++] = 'B';             
  }    
  t[p++] = ' ';
  
  if (o < 0) { 
    t[p++] = '-';  
    o = -o; 
  }
  t[p++] = (char)(48 + (o % 10)); 
  
  t[p] = 0; 
  sprintf(text, "%s Hz", t);
}
	
void VocInput::process_events() {
  if (gval.tracking != 65535) {
    param[0] = gval.tracking * 0.001f;
  }
  if (gval.pitch != 65535) {
    param[1] = gval.pitch * 0.001f;
  }
  if (gval.breath != 65535) {
    param[2] = gval.breath * 0.001f;
  }
  if (gval.sthresh != 65535) {
    param[3] = gval.sthresh * 0.001f;
  }
  if (gval.maxfreq != 65535) {
    param[4] = gval.maxfreq * 0.001f;
  }
  resume();
}

bool VocInput::process_stereo(float **pin, float **pout, int sampleFrames, int mode) {
  float *in1 = pin[0];
  float *in2 = pin[1];
  float *out1 = pout[0];
  float *out2 = pout[1];
  float a, b;
  float ds = pstep, s = sawbuf, n = noise;
  float l0 = lbuf0, l1 = lbuf1, l2 = lbuf2, l3 = lbuf3; 
  float le = lenv, he = henv, et = lfreq * 0.1f, lf = lfreq, v = vuv, mn = minp, mx = maxp;
  float rootm = 39.863137f;
  int tr = track;

  --in1;
  --in2;
  --out1;
  --out2;
  while (--sampleFrames >= 0) {
    a = *++in1; 
    b = *++in2;

    l0 -= lf * (l1 + a); // fundamental filter (peaking 2nd-order 100Hz lpf)
    l1 -= lf * (l1 - l0);

    b = l0; 
    if (b < 0.0f) {
      b = -b;
    }
    le -= et * (le - b); // fundamental level

    b = (a + 0.03f) * v;
    if (b < 0.0f) {
      b = -b;
    }
    he -= et * (he - b); // overall level (+ constant so >f0 when quiet)

    l3 += 1.0f;
    if (tr > 0) {
      // pitch tracking
      if (l1 > 0.0f && l2 <= 0.0f) {
	// found +ve zero crossing
        if (l3 > mn && l3 < mx) {
	  // ...in allowed range
          mn = 0.6f * l3; //new max pitch to discourage octave jumps!
          l2 = l1 / (l1 - l2); //fractional period...
          ds = pmult / (l3 - l2); //new period

          if (tr == 2) {
	    // quantize pitch
            ds = rootm * (float)(log10(ds) - root);
            ds = (float)pow(1.0594631, floor(ds + 0.5) + rootm * root);   
          }
        }
        l3 = l2; // restart period measurement
      }
      l2 = l1; // remember previous sample
    }
  
    b = 0.00001f * (float)((rand() & 32767) - 16384); // sibilance
    if (le > he) {
      b *= s * n; //...or modulated breath noise
    }
    b += s; 
    s += ds; 
    if (s > 0.5f) {
      s -= 1.0f; // badly aliased sawtooth!
    }

    *++out1 = a;
    *++out2 = b;
  }
  sawbuf = s;

  if (fabs(he) > 1.0e-10) {
    henv = he;
  } else {
    henv = 0.0f; //catch denormals
  }
  if (fabs(l1) > 1.0e-10) { 
    lbuf0 = l0; 
    lbuf1 = l1; 
    lenv = le; 
  } else { 
    lbuf0 = lbuf1= lenv = 0.0f; 
  }
  lbuf2 = l2, lbuf3 = l3;
  if (tr) {
    pstep = ds; 
  }
  return true;
}

const char *VocInput::describe_value(int index, int value) {
  static char txt[20];
  switch (index) {
  case 0:
    switch (track) {
    case 0:
      sprintf(txt, "OFF");
      break;
    case 1:
      sprintf(txt, "FREE");
      break;
    case 2:
      sprintf(txt, "QUANT");
      break;
    }
    break;
  case 1:
    if (track) {
      sprintf(txt, "%d", int(48.0f * param[1] - 24.0f));
    } else {
      midi2string(int(48.0f * param[1] + 21.0f), txt);
    }
    break;
  case 4:
    midi2string(int(48.0f * param[4] + 45.0f), txt);
    break;
  default:
    sprintf(txt, "%.0f%%", 100.0f * param[index]);
  }
  return txt;
}
