#include <cstdio>
#include <cmath>

#include "MultiBand.hpp"

MultiBand::MultiBand() {
  global_values = &gval;
  track_values = 0;
  attributes = 0;
  fParam1 = (float)1.00; // Listen: L/M/H/out
  fParam2 = (float)0.103; // xover1
  fParam3 = (float)0.878; // xover2
  fParam4 = (float)0.54; // L drive
  fParam5 = (float)0.00; // M drive
  fParam6 = (float)0.60; // H drive
  fParam7 = (float)0.45; // L trim
  fParam8 = (float)0.50; // M trim
  fParam9 = (float)0.50; // H trim
  fParam10 = (float)0.22; // attack
  fParam11 = (float)0.602; // release
  fParam12 = (float)0.55; // width
  fParam13 = (float)0.40; // MS swap
}

void MultiBand::init(zzub::archive *pi) {
  gain1 = 1.0;
  driv1 = (float)pow(10.0,(2.5 * fParam4) - 1.0);
  trim1 = (float)(0.5 + (4.0 - 2.0 * fParam10) * (fParam4 * fParam4 * fParam4)); 
  trim1 = (float)(trim1 * pow(10.0, 2.0 * fParam7 - 1.0));
  att1 = (float)pow(10.0, -0.05 -(2.5 * fParam10));
  rel1 = (float)pow(10.0, -2.0 - (3.5 * fParam11));

  gain2 = 1.0;
  driv2 = (float)pow(10.0,(2.5 * fParam5) - 1.0);
  trim2 = (float)(0.5 + (4.0 - 2.0 * fParam10) * (fParam5 * fParam5 * fParam5)); 
  trim2 = (float)(trim2 * pow(10.0, 2.0 * fParam8 - 1.0));
  att2 = (float)pow(10.0, -0.05 -(2.0 * fParam10));
  rel2 = (float)pow(10.0, -2.0 - (3.0 * fParam11));
  
  gain3 = 1.0;
  driv3 = (float)pow(10.0, (2.5 * fParam6) - 1.0);
  trim3 = (float)(0.5 + (4.0 - 2.0 * fParam10) * (fParam6 * fParam6 * fParam6)); 
  trim3 = (float)(trim3 * pow(10.0, 2.0 * fParam9 - 1.0));
  att3 = (float)pow(10.0, -0.05 -(1.5 * fParam10));
  rel3 = (float)pow(10.0, -2.0 - (2.5 * fParam11));

  switch (int(fParam1 * 10.0)) {
    case 0: 
      trim2 = 0.0; 
      trim3 = 0.0; 
      slev = 0.0; 
      break;
    case 1: 
    case 2: 
      trim1 = 0.0; 
      trim3 = 0.0; 
      slev = 0.0; 
      break;
    case 3: 
    case 4: 
      trim1 = 0.0; 
      trim2 = 0.0; 
      slev = 0.0; 
      break;
    default: 
      slev = fParam12; 
      break;
  }

  fi1 = (float)pow(10.0, fParam2 - 1.70); 
  fo1 = (float)(1.0 - fi1);
  fi2 = (float)pow(10.0, fParam3 - 1.05); 
  fo2 = (float)(1.0 - fi2);
  mswap = 0;
}
	
void MultiBand::process_events() {
  if (gval.listen != 65535) {
    fParam1 = gval.listen * 0.001f;
  }
  if (gval.xover1 != 65535) {
    fParam2 = gval.xover1 * 0.001f;
  }
  if (gval.xover2 != 65535) {
    fParam3 = gval.xover2 * 0.001f;
  }
  if (gval.ldrive != 65535) {
    fParam4 = gval.ldrive * 0.001f;
  }
  if (gval.mdrive != 65535) {
    fParam5 = gval.mdrive * 0.001f;
  }
  if (gval.hdrive != 65535) {
    fParam6 = gval.hdrive * 0.001f;
  }
  if (gval.ltrim != 65535) {
    fParam7 = gval.ltrim * 0.001f;
  }
  if (gval.mtrim != 65535) {
    fParam8 = gval.mtrim * 0.001f;
  }
  if (gval.htrim != 65535) {
    fParam9 = gval.htrim * 0.001f;
  }
  if (gval.attack != 65535) {
    fParam10 = gval.attack * 0.001f;
  }
  if (gval.release != 65535) {
    fParam11 = gval.release * 0.001f;
  }
  if (gval.width != 65535) {
    fParam12 = gval.width * 0.001f;
  }
  if (gval.msswap != 65535) {
    fParam13 = gval.msswap * 0.001f;
  }
  // calcs here
  driv1 = (float)pow(10.0, (2.5 * fParam4) - 1.0);
  trim1 = (float)(0.5 + (4.0 - 2.0 * fParam10) * (fParam4 * fParam4 * fParam4)); 
  trim1 = (float)(trim1 * pow(10.0, 2.0 * fParam7 - 1.0));
  att1 = (float)pow(10.0, -0.05 -(2.5 * fParam10));
  rel1 = (float)pow(10.0, -2.0 - (3.5 * fParam11));

  driv2 = (float)pow(10.0, (2.5 * fParam5) - 1.0);
  trim2 = (float)(0.5 + (4.0 - 2.0 * fParam10) * (fParam5 * fParam5 * fParam5)); 
  trim2 = (float)(trim2 * pow(10.0, 2.0 * fParam8 - 1.0));
  att2 = (float)pow(10.0, -0.05 -(2.0 * fParam10));
  rel2 = (float)pow(10.0, -2.0 - (3.0 * fParam11));
  
  driv3 = (float)pow(10.0, (2.5 * fParam6) - 1.0);
  trim3 = (float)(0.5 + (4.0 - 2.0 * fParam10) * (fParam6 * fParam6 * fParam6)); 
  trim3 = (float)(trim3 * pow(10.0, 2.0 * fParam9 - 1.0));
  att3 = (float)pow(10.0, -0.05 -(1.5 * fParam10));
  rel3 = (float)pow(10.0, -2.0 - (2.5 * fParam11));

  switch (int(fParam1 * 10.0)) {
    case 0: 
      trim2 = 0.0; 
      trim3 = 0.0; 
      slev = 0.0; 
      break;
    case 1: 
    case 2: 
      trim1 = 0.0; 
      trim3 = 0.0; 
      slev = 0.0; 
      break;
    case 3: 
    case 4: 
      trim1 = 0.0; 
      trim2 = 0.0; 
      slev = 0.0; 
      break;
    default: 
      slev = fParam12; 
      break;
  }
  fi1 = (float)pow(10.0, fParam2 - 1.70); 
  fo1 = (float)(1.0 - fi1);
  fi2 = (float)pow(10.0, fParam3 - 1.05); 
  fo2 = (float)(1.0 - fi2);

  if (fParam13 > 0.5) {
    mswap = 1;
  } else {
    mswap = 0;
  }
}

bool MultiBand::process_stereo(float **pin, float **pout, int sampleFrames, int mode) {
  float *in1 = pin[0];
  float *in2 = pin[1];
  float *out1 = pout[0];
  float *out2 = pout[1];
  float a, b, c, d, l = fb3, m, h, s, sl = slev, tmp1, tmp2, tmp3;	
  float f1i = fi1, f1o = fo1, f2i = fi2, f2o = fo2, b1 = fb1, b2 = fb2;
  float g1 = gain1, d1 = driv1, t1 = trim1, a1 = att1, r1 = 1.f - rel1;
  float g2 = gain2, d2 = driv2, t2 = trim2, a2 = att2, r2 = 1.f - rel2;
  float g3 = gain3, d3 = driv3, t3 = trim3, a3 = att3, r3 = 1.f - rel3;
  int ms = mswap;

  --in1;	
  --in2;	
  --out1;
  --out2;
  while (--sampleFrames >= 0) {
    a = *++in1;		
    b = *++in2; // process from here...
		
    b = (ms) ? -b : b;

    s = (a - b) * sl; // keep stereo component for later
    a += b;
    b2 = (f2i * a) + (f2o * b2); // crossovers
    b1 = (f1i * b2) + (f1o * b1); 
    l = (f1i * b1) + (f1o * l);
    m = b2 - l; 
    h = a - b2;

    tmp1 = (l > 0) ? l : -l;  // l
    g1 = (tmp1 > g1) ? g1 + a1 * (tmp1 - g1) : g1 * r1;
    tmp1 = 1.f / (1.f + d1 * g1); 

    tmp2 = (m > 0) ? m : -m;
    g2 = (tmp2 > g2) ? g2 + a2 * (tmp2 - g2) : g2 * r2;
    tmp2 = 1.f / (1.f + d2 * g2); 

    tmp3 = (h > 0) ? h : -h;
    g3 = (tmp3 > g3) ? g3 + a3 * (tmp3 - g3) : g3 * r3;
    tmp3 = 1.f / (1.f + d3 * g3); 
            
    a = (l * tmp3 * t1) + (m * tmp2 * t2) + (h * tmp3 * t3);
    c = a + s; // output
    d = (ms) ? s - a : a - s;
    
    *++out1 = c;
    *++out2 = d;
  }
  gain1 = (g1 < 1.0e-10) ? 0.f : g1;
  gain2 = (g2 < 1.0e-10) ? 0.f : g2;
  gain3 = (g3 < 1.0e-10) ? 0.f : g3;
  if (fabs(b1) < 1.0e-10) { 
    fb1 = 0.f; 
    fb2 = 0.f; 
    fb3 = 0.f; 
  } else { 
    fb1 = b1;  
    fb2 = b2;  
    fb3 = l;   
  }
  return true;
}

const char *MultiBand::describe_value(int index, int value) {
  static char txt[20];
  switch (index) {
  case 0:
    switch (int(fParam1 * 10.0)) {
    case 0:
      sprintf(txt, "Low");
      break;
    case 1:
    case 2:
      sprintf(txt, "Mid");
      break;
    case 3:
    case 4:
      sprintf(txt, "High");
      break;
    default:
      sprintf(txt, "Output");
      break;
    }
    break;
  case 1:
    sprintf(txt, "%d Hz", (int)(_master_info->samples_per_second * fi1 * (0.098 + 0.09 * fi1 + 0.5 * (float)pow(fi1, 8.2f))));
    break;
  case 2:
    sprintf(txt, "%d Hz", (int)(_master_info->samples_per_second * fi2 * (0.015 + 0.15 * fi2 + 0.9 * (float)pow(fi2, 8.2f))));
    break;
  case 3:
    sprintf(txt, "%d dB", int(30.0 * fParam4));
    break;
  case 4:
    sprintf(txt, "%d dB", int(30.0 * fParam5));
    break;
  case 5:
    sprintf(txt, "%d dB", int(30.0 * fParam6));
    break;
  case 6:
    sprintf(txt, "%d dB", int(40.0 * fParam7 - 20.0));
    break;
  case 7:
    sprintf(txt, "%d dB", int(40.0 * fParam8 - 20.0));
    break;
  case 8:
    sprintf(txt, "%d dB", int(40.0 * fParam9 - 20.0));
    break;
  case 9:
    sprintf(txt, "%d ms", int(-301030.1 / (_master_info->samples_per_second * log10(1.0 - att2))));
    break;
  case 10:
    sprintf(txt, "%d ms", int(-301.0301 / (_master_info->samples_per_second * log10(1.0 - rel2))));
    break;
  case 11:
    sprintf(txt, "%d %%", int(200 * fParam12));
    break;
  case 12:
    if (mswap) {
      sprintf(txt, "S");
    } else {
      sprintf(txt, "M");
    }
    break;
  default:
    return 0;
  }
  return txt;
}
