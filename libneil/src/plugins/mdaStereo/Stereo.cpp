#include <cstdio>
#include <cmath>
#include <cstring>

#include "Stereo.hpp"

Stereo::Stereo() {
  global_values = &gval;
  track_values = 0;
  attributes = 0;
  fParam1 = (float)0.78; // Haas/Comb width
  fParam2 = (float)0.43; // delay
  fParam3 = (float)0.50; // balance
  fParam4 = (float)0.00; // mod
  fParam5 = (float)0.50; // rate
}

void Stereo::init(zzub::archive *pi) {
  size = 4800;
  bufpos = 0;
  buffer = new float[size];
  memset(buffer, 0, size * sizeof(float));
  //calcs here!
  phi = 0;
  dphi = (float)(3.141 * pow(10.0, -2.0 + 3.0 * fParam5) / _master_info->samples_per_second); 
  mod = (float)(2100.0 * pow(fParam4, 2));

  if (fParam1 < 0.5) {
    fli = (float)(0.25 + (1.5 * fParam1));
    fld = 0.0;
    fri = (float)(2.0 * fParam1);
    frd = (float)(1.0 - fri);
  } else {
    fli = (float)(1.5 - fParam1);
    fld = (float)(fParam1 - 0.5);
    fri = fli;
    frd = -fld;
  }
  fdel = (float)(20.0 + 2080.0 * pow(fParam2, 2));
  if (fParam3 > 0.5) {
    fli *= (float)((1.0 - fParam3) * 2.0);
    fld *= (float)((1.0 - fParam3) * 2.0);
  } else {
    fri *= (2 * fParam3);
    frd *= (2 * fParam3);
  }
  fri *= (float)(0.5 + fabs(fParam1 - 0.5));
  frd *= (float)(0.5 + fabs(fParam1 - 0.5));
  fli *= (float)(0.5 + fabs(fParam1 - 0.5));
  fld *= (float)(0.5 + fabs(fParam1 - 0.5));
}

void Stereo::destroy() {
  if (buffer) {
    delete [] buffer;
  }
}
	
void Stereo::process_events() {
  if (gval.width != 65535) {
    fParam1 = gval.width * 0.001f;
  }
  if (gval.delay != 65535) {
    fParam2 = gval.delay * 0.001f;
  }
  if (gval.balance != 65535) {
    fParam3 = gval.balance * 0.001f;
  }
  if (gval.mod != 65535) {
    fParam4 = gval.mod * 0.001f;
  }
  if (gval.rate != 65535) {
    fParam5 = gval.rate * 0.001f;
  }
  //calcs here
  dphi = (float)(3.141 * pow(10.0, -2.0 + 3.0 * fParam5) / _master_info->samples_per_second); 
  mod = (float)(2100.0 * pow(fParam4, 2));

  if (fParam1 < 0.5) {
    fli = (float)(0.25 + (1.5 * fParam1));
    fld = 0.0;
    fri = (float)(2.0 * fParam1);
    frd = (float)(1.0 - fri);
  } else {
    fli = (float)(1.5 - fParam1);
    fld = (float)(fParam1 - 0.5);
    fri = fli;
    frd = -fld;
  }
  fdel = (float)(20.0 + 2080.0 * pow(fParam2, 2));
  if (fParam3 > 0.5) {
    fli *= (float)((1.0 - fParam3) * 2.0);
    fld *= (float)((1.0 - fParam3) * 2.0);
  } else {
    fri *= (2 * fParam3);
    frd *= (2 * fParam3);
  }
  fri *= (float)(0.5 + fabs(fParam1 - 0.5));
  frd *= (float)(0.5 + fabs(fParam1 - 0.5));
  fli *= (float)(0.5 + fabs(fParam1 - 0.5));
  fld *= (float)(0.5 + fabs(fParam1 - 0.5));
}

bool Stereo::process_stereo(float **pin, float **pout, int sampleFrames, int mode) {
  float *in1 = pin[0];
  float *in2 = pin[1];
  float *out1 = pout[0];
  float *out2 = pout[1];
  float a, b, c, d;	
  float li, ld, ri, rd, del, ph = phi, dph = dphi, mo = mod;
  int tmp, bp = bufpos;
  
  li = fli;
  ld = fld;
  ri = fri;
  rd = frd;
  del = fdel;
    
  --in1;	
  --in2;	
  --out1;
  --out2;
  if (mo > 0.f) {
    // modulated delay
    while (--sampleFrames >= 0) {
      a = *++in1 + *++in2; // sum to mono
		  
      *(buffer + bp) = a; // write
      tmp = (bp + (int)(del + fabs(mo * sin(ph)))) % 4410;
      b = *(buffer + tmp);

      c = (a * li) - (b * ld); // output
      d = (a * ri) - (b * rd);
		      
      bp = (bp - 1); 
      if (bp < 0) {
	bp = 4410; //buffer position
      }
      
      ph = ph + dph;

      *++out1 = c;
      *++out2 = d;
    }
  } else {
    while (--sampleFrames >= 0) {
      a = *++in1 + *++in2; // sum to mono
		  
      *(buffer + bp) = a; // write
      tmp = (bp + (int)(del) ) % 4410;
      b = *(buffer + tmp);

      c = (a * li) - (b * ld); // output
      d = (a * ri) - (b * rd);
		      
      bp = (bp - 1); 
      if (bp < 0) {
	bp = 4410; // buffer position
      }

      *++out1 = c;
      *++out2 = d;
    }
  }
  bufpos = bp;
  phi = (float)fmod(ph, 6.2831853f);
  return true;
}

const char *Stereo::describe_value(int index, int value) {
  static char txt[20];
  switch (index) {
  case 0:
    if (fParam1 < 0.5) {
      sprintf(txt, "%d Haas", int(200.0 * fabs(fParam1 - 0.5)));
    } else {
      sprintf(txt, "%d Comb", int(200.0 * fabs(fParam1 - 0.5)));
    }
    break;
  case 1:
    sprintf(txt, "%.2f ms", 1000.0f * fdel / _master_info->samples_per_second);
    break;
  case 2:
    sprintf(txt, "%d", int(200.0 * (fParam3 - 0.5)));
    break;
  case 3:
    if (mod > 0.0f) {
      sprintf(txt, "%.2f ms", 1000.0 * mod / _master_info->samples_per_second);
    } else {
      sprintf(txt, "OFF");
    }
    break;
  case 4:
    sprintf(txt, "%.2f sec", pow(10.0, 2.0 - 3.0 * fParam5));
    break;
  default:
    return 0;
  }
  return txt;
}
