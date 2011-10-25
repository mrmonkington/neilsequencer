#include <cstdio>
#include <cmath>
#include <cstring>

#include "RePsycho.hpp"

RePsycho::RePsycho() {
  global_values = &gval;
  track_values = 0;
  attributes = 0;
}

void RePsycho::init(zzub::archive *pi) {
  //inits here!
  fParam1 = (float)0.6; //thresh
  fParam2 = (float)0.5; //env
  fParam3 = (float)1.0; //tune
  fParam4 = (float)1.0; //mix
  fParam5 = (float)0.45; //minimum chunk length
  fParam6 = (float)1.0; //fine tune
  fParam7 = (float)0.4; //quality
  size = 22050;
  buffer = new float[size];
  buffer2 = new float[size];
  suspend(); // flush buffer
  //calcs here!
  buf = 0.0; 
  buf2 = 0.0;
  tim = size + 1;
  dtim = 441 + int(0.5 * size * fParam5);
  fil = 0.0;
  thr = (float)pow(10.0,(1.5 * fParam1) - 1.5);
  if (fParam2 > 0.5) { 
    env = (float)(1.0 + 0.003 * pow(fParam2 - 0.5, 5.0)); 
  } else { 
    env = (float)(1.0 + 0.025 * pow(fParam2 - 0.5, 5.0)); 
  }
  tun = (float)(((int(fParam3 * 24.0) - 24.0) + (fParam6 - 1.0)) / 24.0);
  tun = (float)pow(10.0, 0.60206 * tun);
  wet = (float)(0.5 * sqrt(fParam4));
  dry = (float)sqrt(1.0 - fParam4);
}

void RePsycho::destroy() {
  if (buffer) {
    delete [] buffer;
  }
  if (buffer2) {
    delete [] buffer2;
  }
}

void RePsycho::suspend() {
  memset(buffer, 0, size * sizeof(float));
  memset(buffer2, 0, size * sizeof(float));
}
	
void RePsycho::process_events() {
  if (gval.thresh != 65535) {
    fParam1 = gval.thresh * 0.001f;
  }
  if (gval.env != 65535) {
    fParam2 = gval.env * 0.001f;
  }
  if (gval.tune != 65535) {
    fParam3 = gval.tune * 0.001f;
  }
  if (gval.mix != 65535) {
    fParam4 = gval.mix * 0.001f;
  }
  if (gval.min_length != 65535) {
    fParam5 = gval.min_length * 0.001f;
  }
  if (gval.fine_tune != 65535) {
    fParam6 = gval.fine_tune * 0.001f;
  }
  if (gval.quality != 65535) {
    fParam7 = gval.quality * 0.001f;
  }
  dtim = 441 + int(0.5 * size * fParam5);
  thr = (float)pow(10.0,(1.5 * fParam1) - 1.5); 
  if (fParam2 > 0.5) { 
    env = (float)(1.0 + 0.003 * pow(fParam2 - 0.5,5.0)); 
  } else { 
    env = (float)(1.0 + 0.025 * pow(fParam2 - 0.5,5.0)); 
  }
  tun = (float)(((int(fParam3 * 24.0) - 24.0) + (fParam6 - 1.0)) / 24.0);
  tun = (float)pow(10.0, 0.60206 * tun);
  wet = (float)(0.5 * sqrt(fParam4));
  dry = (float)sqrt(1.0 - fParam4);
}

bool RePsycho::process_stereo(float **pin, float **pout, int sampleFrames, int mode) {
  float *in1 = pin[0];
  float *in2 = pin[1];
  float *out1 = pout[0];
  float *out2 = pout[1];
  float a, b, c, d;	
  float we = wet, dr = dry, tu = tun, en = env;
  float ga = gai, x = 0.0f, x2 = 0.0f, xx = buf, xx2 = buf2;
  float it1, it2;
  int ti = tim, dti = dtim, of1, of2;

  --in1;	
  --in2;	
  --out1;
  --out2;

  if (fParam7 > 0.5) {
    // high quality
    we = (float)(we * 2.0);
    while (--sampleFrames >= 0) {
      a = *++in1;		
      b = *++in2; // process from here...
		  
      if ((a+b > thr) && (ti > dti)) {
	// trigger
        ga = 1.0;
        ti = 0;
      }

      if (ti < 22050) {  
	// play out
        if (ti < 80) {
	  // fade in
          if (ti == 0) { 
	    xx = x; 
	    xx2 = x2; 
	  }

          *(buffer + ti) = a;
          *(buffer2 + ti) = b;
          x = *(buffer + int(ti * tu));
          x2 = *(buffer2 + int(ti * tu));
        
          x = (float)(xx * (1.0 - (0.0125 * ti)) + (x * 0.0125 * ti)); 
          x2 = (float)(xx2 * (1.0 - (0.0125 * ti)) + (x2 * 0.0125 * ti)); 
        } else {
          // update to/from buffer
          *(buffer + ti) = a;
          *(buffer2 + ti) = b;

          it1 = (float)(ti * tu); // interpolation
          of1 = (int)it1; 
	  of2 = of1 + 1; 
	  it1 = it1 - of1; 
	  it2 = (float)(1.0 - it1);

	  x = (it2 * *(buffer + of1)) + (it1 * *(buffer + of2));
	  x2 = (it2 * *(buffer2 + of1)) + (it1 * *(buffer2 + of2));
        }
        ti++;
        ga*= en;
      } else {
	// mute
        ga = 0;
      }
      
      c = (a * dr) + (x * ga * we); // output
      d = (b * dr) + (x2 * ga * we);
		      
      *++out1 = c;
      *++out2 = d;
    }
  } else {
    while (--sampleFrames >= 0) {
      a = *++in1;		
      b = *++in2; // process from here...
		  
      if ((a+b > thr) && (ti > dti)) {
	// trigger
        ga = 1.0;
        ti = 0;
      }

      if (ti < 22050) {  
	// play out
        if (ti < 80) {
	  //fade in
          if (ti == 0) {
	    xx = x;
	  }

          *(buffer + ti) = (a + b);
          x = *(buffer + int(ti * tu));
        
          x = (float)(xx * (1.0 - (0.0125 * ti)) + (x * 0.0125 * ti));
        } else {
          // update to/from buffer
          *(buffer + ti) = (a + b);
          x = *(buffer + int(ti * tu));
        }

        ti++;
        ga*= en;
      } else {
	// mute
        ga = 0;
      }
      
      c = (a * dr) + (x * ga * we); // output
      d = (b * dr) + (x * ga * we);
      
      *++out1 = c;
      *++out2 = d;
    }
  }
  tim = ti;
  gai = ga;
  buf = xx;
  buf2 = xx2;
  return true;
}

const char *RePsycho::describe_value(int param, int value) {
  static char txt[20];
  switch (param) {
  case 0:
    sprintf(txt, "%d dB", int(30.0 * fParam1 - 30.0));
    break;
  case 1:
    sprintf(txt, "%d %%", int((fParam2 - 0.5) * 100.0));
    break;
  case 2:
    sprintf(txt, "%d semi", int(24.0 * fParam3 - 24.0));
    break;
  case 3:
    sprintf(txt, "%d %%", int(100.0 * fParam4));
    break;
  case 4:
    sprintf(txt, "%d ms", int(1000.0 * dtim / _master_info->samples_per_second));
    break;
  case 5:
    sprintf(txt, "%d cent", int(99.0 * fParam6 - 99.0));
    break;
  case 6:
    if (fParam7 > 0.5) {
      sprintf(txt, "HIGH");
    } else {
      sprintf(txt, "LOW");
    }
    break;
  default:
    return 0;
  }
  return txt;
}
