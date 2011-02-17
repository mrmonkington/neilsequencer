#include <cstdio>
#include <cmath>
#include <cstring>
#include <algorithm>

#include "mdaDubDelay.hpp"

mdaDubDelay::mdaDubDelay() 
{
  global_values = &gval;
  attributes = NULL;
  track_values = NULL;
  //inits here!
  fParam0 = 0.30f; //delay
  fParam1 = 0.70f; //feedback
  fParam2 = 0.40f; //tone
  fParam3 = 0.00f; //lfo depth
  fParam4 = 0.50f; //lfo speed
  fParam5 = 0.33f; //wet mix
  fParam6 = 0.50f; //output
  size = 323766; 
  buffer = new float[size + 2]; //spare just in case!
  ipos = 0;
  fil0 = 0.0f;
  env = 0.0f;
  phi = 0.0f;
  dlbuf = 0.0f;
  suspend(); //flush buffer
}

void mdaDubDelay::suspend()
{
  memset(buffer, 0, size * sizeof(float));
}

mdaDubDelay::~mdaDubDelay() 
{
  if (buffer) 
    delete [] buffer;
  delete this;
}

void mdaDubDelay::init(zzub::archive* pi) {

}

void mdaDubDelay::destroy() {

}

void mdaDubDelay::process_events() {
  float fs = _master_info->samples_per_second;
  if (fs < 8000.0f) 
    fs = 44100.0f; //??? bug somewhere!
  if (gval.paraDelay != 0xff) {
    fParam0 = gval.paraDelay / 254.0;
  }
  if (gval.paraFeedback != 0xff) {
    fParam1 = gval.paraFeedback / 254.0;
  }
  if (gval.paraFbTone != 0xff) {
    fParam2 = gval.paraFbTone / 254.0;
  }
  if (gval.paraLFODep != 0xff) {
    fParam3 = gval.paraLFODep / 254.0;
  }
  if (gval.paraLFORate != 0xff) {
    fParam4 = gval.paraLFORate / 254.0;
  }
  if (gval.paraFXMix != 0xff) {
    fParam5 = gval.paraFXMix / 254.0;
  }
  if (gval.paraOutput != 0xff) {
    fParam6 = gval.paraOutput / 254.0;
  }
  //calcs here
  ///CHANGED///del = fParam0 * fParam0 * fParam0 * (float)size;
  del = fParam0 * fParam0 * (float)size;
  mod = 0.049f * fParam3 * del;
  fil = fParam2;
  if (fParam2 > 0.5f) {
    fil = 0.5f * fil - 0.25f; 
    lmix = -2.0f * fil;
    hmix = 1.0f;
  } else { 
    hmix = 2.0f * fil; 
    lmix = 1.0f - hmix;
  }
  fil = (float)exp(-6.2831853f * pow(10.0f, 2.2f + 4.5f * fil) / fs);
  fbk = (float)fabs(2.2f * fParam1 - 1.1f);
  if (fParam1 > 0.5f) 
    rel = 0.9997f; 
  else 
    rel = 0.8f; //limit or clip
  wet = 1.0f - fParam5;
  wet = fParam6 * (1.0f - wet * wet); //-3dB at 50% mix
  dry = fParam6 * 2.0f * (1.0f - fParam5 * fParam5);
  //100-sample steps
  dphi = 628.31853f * (float)pow(10.0f, 3.0f * fParam4 - 2.0f) / fs;
}

bool mdaDubDelay::process_stereo(float **pin, float **pout, 
				 int sampleFrames, int mode) {
  float *in1 = pin[0];
  float *in2 = pin[1];
  float *out1 = pout[0];
  float *out2 = pout[1];
  float a, b, c, d, ol, w = wet, y = dry, 
    fb = fbk, dl = dlbuf, db = dlbuf, ddl = 0.0f;
  float lx = lmix, hx = hmix, f = fil, f0 = fil0, tmp;
  float e = env, g, r = rel; //limiter envelope, gain, release
  float twopi = 6.2831853f;
  int i = ipos, l, s = size, k = 0;
  --in1;	
  --in2;	
  --out1;
  --out2;
  while(--sampleFrames >= 0) {
    a = *++in1;
    b = *++in2;
    c = out1[1];
    d = out2[1]; 
    if (k == 0) {
      db += 0.01f * (del - db - mod - mod * (float)sin(phi));
      ddl = 0.01f * (db - dl);
      phi+=dphi; 
      if (phi > twopi) 
	phi -= twopi;
      k = 100;
    }
    k--;
    dl += ddl; //lin interp between points
    i--; 
    if (i < 0) 
      i = s;
    l = (int)dl;
    tmp = dl - (float)l; //remainder
    l += i; 
    if (l > s) 
      l -= (s + 1);
    ol = *(buffer + l); //delay output
    l++; 
    if (l > s) 
      l = 0; 
    ol += tmp * (*(buffer + l) - ol); //lin interp
    tmp = a + fb * ol; //mix input (left only!) & feedback
    f0 = f * (f0 - tmp) + tmp; //low-pass filter
    tmp = lx * f0 + hx * tmp;
    g = (tmp < 0.0f) ? -tmp : tmp; //simple limiter
    e *= r; 
    if (g > e) 
      e = g;
    if (e > 1.0f) 
      tmp /= e;
    *(buffer + i) = tmp; //delay input
    ol *= w; //wet
    *++out1 = c + y * a + ol; //dry
    *++out2 = d + y * b + ol; 
  }
  ipos = i;
  dlbuf = dl;
  if (fabs(f0) < 1.0e-10) { 
    fil0 = 0.0f; 
    env = 0.0f; 
  } else { 
    fil0 = f0; env = e; 
  } //trap denormals
  return true;
}

const char *mdaDubDelay::describe_value(int param, int value) {
  static const int LABEL_SIZE = 20;
  static char str[LABEL_SIZE];
  float fval = value / 254.0f;
  switch (param) {
  case 0:
    sprintf(str, "%d ms", 
	    int(fval * fval * size * 1000.0f / 
		_master_info->samples_per_second));
    break;
  case 1:
    sprintf(str, "%d", int(220 * fval - 110));
    break;
  case 2:
    sprintf(str, "%d", int(200 * fval - 100));
    break;
  case 3:
    sprintf(str, "%d %%", int(100 * fval));
    break;
  case 4:
    sprintf(str, "%.2f s", pow(10.0, 2.0 - 3.0 * fval));
    break;
  case 5:
    sprintf(str, "%d %%", int(100 * fval));
    break;
  case 6:
    sprintf(str, "%d dB", int(20 * log10(2.0 * fval)));
    break;
  default:
    return 0;
  }
  return str;
}



