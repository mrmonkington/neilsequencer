#include <cstdio>
#include <cmath>
#include <cstring>

#include "ThruZero.hpp"

ThruZero::ThruZero() {
  global_values = &gval;
  attributes = 0;
  track_values = 0;
  bufpos = 0;
  buffer = new float[BUFMAX];
  buffer2 = new float[BUFMAX];
  phi = fb = fb1 = fb2 = deps = 0.0f;
  suspend();
}

ThruZero::~ThruZero() {
  if (buffer) {
    delete[] buffer;
  }
  if (buffer2) {
    delete[] buffer2;
  }
}

void ThruZero::resume()
{
  rat = (float)(pow(10.0f, 3.f * param[0] - 2.f) * 2.f / 
		_master_info->samples_per_second);
  dep = 2000.0f * param[1] * param[1];
  dem = dep - dep * param[4];
  dep -= dem;
  wet = param[2];
  dry = 1.f - wet;
  if (param[0] < 0.01f) { 
    rat = 0.0f; 
    phi = (float)0.0f; 
  }
  fb = 1.9f * param[3] - 0.95f;
}


void ThruZero::suspend()
{
  if (buffer) {
    memset(buffer, 0, BUFMAX * sizeof(float));
  }
  if (buffer2) {
    memset(buffer2, 0, BUFMAX * sizeof(float));
  }
}

void ThruZero::init(zzub::archive *pi) {

}
	
void ThruZero::process_events() {
  if (gval.rate != 255) {
    param[0] = gval.rate * 0.01f;
  }
  if (gval.depth != 255) {
    param[1] = gval.depth * 0.01f;
  }
  if (gval.mix != 255) {
    param[2] = gval.mix * 0.01f;
  }
  if (gval.feedback != 255) {
    param[3] = gval.feedback * 0.01f;
    phi = 0.0f;
  }
  if (gval.depthmod != 255) {
    param[4] = gval.depthmod * 0.01f;
  }
  resume();
}

bool ThruZero::process_stereo(float **pin, float **pout, 
			      int sampleFrames, int mode) {
  float *in1 = pin[0];
  float *in2 = pin[1];
  float *out1 = pout[0];
  float *out2 = pout[1];
  float a, b, f = fb, f1 = fb1, f2 = fb2, ph = phi;	
  float ra = rat, de = dep, we = wet, dr = dry, ds = deps, dm = dem;
  uint32_t tmp, tmpi, bp = bufpos;
  float tmpf, dpt;
  --in1;	
  --in2;	
  --out1;
  --out2;
  while (--sampleFrames >= 0) {
    a = *++in1;		
    b = *++in2; 		
    ph += ra; 
    if (ph > 1.0f) 
      ph -= 2.0f;
    bp--; 
    bp &= 0x7FF;
    *(buffer  + bp) = a + f * f1; 
    *(buffer2 + bp) = b + f * f2;
    dpt = tmpf = dm + de * (1.0f - ph * ph); //delay mod shape
    tmp  = int(tmpf);
    tmpf -= tmp;
    tmp = (tmp + bp) & 0x7FF;
    tmpi = (tmp + 1) & 0x7FF;         
    f1 = *(buffer  + tmp);  //try adding a constant to reduce denormalling
    f2 = *(buffer2 + tmp);
    f1 = tmpf * (*(buffer  + tmpi) - f1) + f1; //linear interpolation
    f2 = tmpf * (*(buffer2 + tmpi) - f2) + f2;
    a = a * dr - f1 * we; 
    b = b * dr - f2 * we;
    *++out1 = a;
    *++out2 = b;
  }
  if (fabs(f1) > 1.0e-10) { 
    fb1 = f1; 
    fb2 = f2; 
  } else {
    fb1 = fb2 = 0.0f; //catch denormals
  }
  phi = ph;
  deps = ds;
  bufpos = bp;
  return true;
}

const char *ThruZero::describe_value(int index, int value) {
  static char txt[20];
  switch (index) {
  case 0:
    if (param[0] < 0.01f) {
      sprintf(txt, "-"); 
    }
    else {
      sprintf(txt, "%.2f sec", (float)pow(10.0f, 2.0f - 3.0f * param[index]));
    }
    break;
  case 1:
    sprintf(txt, "%.2f ms", 1000.f * dep / _master_info->samples_per_second);
    break;
  case 3:
    sprintf(txt, "%.0f %%", 200.0f * param[index] - 100.0f);
    break;
  default:
    sprintf(txt, "%.0f %%", 100.0f * param[index]);
    break;
  }
  return txt;
}
