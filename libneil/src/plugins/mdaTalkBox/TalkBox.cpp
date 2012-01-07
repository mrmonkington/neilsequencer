#include <cstdio>
#include <cmath>
#include <cstring>

#include "TalkBox.hpp"

TalkBox::TalkBox() {
  global_values = &gval;
  track_values = 0;
  attributes = 0;
}

void TalkBox::init(zzub::archive *pi) {
  param[0] = 0.5f; // wet
  param[1] = 0.0f; // dry
  param[2] = 0.0f; // swap
  param[3] = 1.0f; // quality
  // initialise...
  buf0 = new float[BUF_MAX];
  buf1 = new float[BUF_MAX];
  window = new float[BUF_MAX];
  car0 = new float[BUF_MAX];
  car1 = new float[BUF_MAX];
  N = 1; // trigger window recalc
  K = 0;
  suspend();
}

void TalkBox::destroy() // destroy any buffers...
{
  if (buf0) {
    delete[] buf0;
  }
  if (buf1) {
    delete[] buf1;
  }
  if (window) {
    delete[] window;
  }
  if (car0) {
    delete[] car0;
  }
  if (car1) {
    delete[] car1;
  }
}

void TalkBox::resume() // update internal parameters...
{
  float fs = _master_info->samples_per_second;
  if (fs < 8000.0f) {
    fs = 8000.0f;
  }
  if (fs > 96000.0f) {
    fs = 96000.0f;
  }
  
  swap = (param[2] > 0.5f) ? 1 : 0;

  int n = (int)(0.01633f * fs);
  if (n > BUF_MAX) {
    n = BUF_MAX;
  }
  
  O = (int)((0.0001f + 0.0004f * param[3]) * fs);

  if (n != N) {
    N = n;
    float dp = TWO_PI / (float)N;
    float p = 0.0f;
    for (n = 0; n < N; n++) {
      window[n] = 0.5f - 0.5f * (float)cos(p);
      p += dp;
    }
  }
  wet = 0.5f * param[0] * param[0];
  dry = 2.0f * param[1] * param[1];
}


void TalkBox::suspend() // clear any buffers...
{
  pos = K = 0;
  emphasis = 0.0f;
  FX = 0;

  u0 = u1 = u2 = u3 = u4 = 0.0f;
  d0 = d1 = d2 = d3 = d4 = 0.0f;

  memset(buf0, 0, BUF_MAX * sizeof(float));
  memset(buf1, 0, BUF_MAX * sizeof(float));
  memset(car0, 0, BUF_MAX * sizeof(float));
  memset(car1, 0, BUF_MAX * sizeof(float));
}
	
void TalkBox::process_events() {
  if (gval.wet != 65535) {
    param[0] = gval.wet * 0.001f;
  }
  if (gval.dry != 65535) {
    param[1] = gval.dry * 0.001f;
  }
  if (gval.swap != 65535) {
    param[2] = gval.swap * 0.001f;
  }
  if (gval.quality != 65535) {
    param[3] = gval.quality * 0.001f;
  }
  resume();
}

bool TalkBox::process_stereo(float **pin, float **pout, int sampleFrames, int mode) {
  if (mode == zzub::process_mode_write || mode == zzub::process_mode_no_io) {
    return false;
  }
  if (mode == zzub::process_mode_read) {
    return true;
  }
  float *in1 = pin[0];
  float *in2 = pin[1];
  if (swap) {
    in1 = pin[1];
    in2 = pin[0];
  }
  float *out1 = pout[0];
  float *out2 = pout[1];
  int p0 = pos, p1 = (pos + N / 2) % N;
  float e = emphasis, w, o, x, dr, fx = FX;
  float p, q, h0 = 0.3f, h1 = 0.77f;

  --in1;
  --in2;
  --out1;
  --out2;

  while (--sampleFrames >= 0) {
    o = *++in1;
    x = *++in2;
    dr = o;
    
    p = d0 + h0 *  x; 
    d0 = d1;  
    d1 = x  - h0 * p;

    q = d2 + h1 * d4; 
    d2 = d3;  
    d3 = d4 - h1 * q;  

    d4 = x;
    x = p + q;

    if (K++) {
      K = 0;
      car0[p0] = car1[p1] = x; // carrier input
      x = o - e;  
      e = o;  // 6dB/oct pre-emphasis
      w = window[p0]; 
      fx = buf0[p0] * w;  
      buf0[p0] = x * w;  // 50% overlapping hanning windows
      if (++p0 >= N) { 
	lpc(buf0, car0, N, O);  
	p0 = 0; 
      }

      w = 1.0f - w;  
      fx += buf1[p1] * w;  
      buf1[p1] = x * w;
      if (++p1 >= N) { 
	lpc(buf1, car1, N, O);  
	p1 = 0; 
      }
    }

    p = u0 + h0 * fx; u0 = u1;  u1 = fx - h0 * p;
    q = u2 + h1 * u4; u2 = u3;  u3 = u4 - h1 * q;  
    u4 = fx;
    x = p + q;

    o = wet * x + dry * dr;
    *++out1 = o;
    *++out2 = o;
  }
  emphasis = e;
  pos = p0;
  FX = fx;

  float den = 1.0e-10f; // (float)pow(10.0f, -10.0f * param[4]);
  if (fabs(d0) < den) {
    d0 = 0.0f;
  }
  if (fabs(d1) < den) {
    d1 = 0.0f;
  }
  if (fabs(d2) < den) {
    d2 = 0.0f;
  }
  if (fabs(d3) < den) {
    d3 = 0.0f;
  }
  if (fabs(u0) < den) {
    u0 = 0.0f;
  }
  if (fabs(u1) < den) {
    u1 = 0.0f;
  }
  if (fabs(u2) < den) {
    u2 = 0.0f;
  }
  if (fabs(u3) < den) {
    u3 = 0.0f;
  }
  if (zzub::buffer_has_signals(pout[0], BUF_MAX) ||
      zzub::buffer_has_signals(pout[1], BUF_MAX)) {
    return true;
  } else {
    return false;
  }
}

void TalkBox::lpc(float *buf, float *car, int n, int o)
{
  float z[ORD_MAX], r[ORD_MAX], k[ORD_MAX], G, x;
  int i, j, nn = n;

  for (j = 0; j <= o; j++, nn--) {
    // buf[] is already emphasized and windowed
    z[j] = r[j] = 0.0f;
    for (i = 0; i < nn; i++) {
      r[j] += buf[i] * buf[i + j]; // autocorrelation
    }
  }
  r[0] *= 1.001f; // stability fix

  float min = 0.00001f;
  if (r[0] < min) { 
    for (i = 0; i < n; i++) {
      buf[i] = 0.0f; 
      return; 
    }
  } 

  lpc_durbin(r, o, k, &G); // calc reflection coeffs

  for (i = 0; i <= o; i++) {
    if (k[i] > 0.995f) {
      k[i] = 0.995f;
    } else if (k[i] < -0.995f) {
      k[i] = -.995f;
    }
  }
  
  for (i = 0; i < n; i++) {
    x = G * car[i];
    for (j = o; j > 0; j--) { 
      // lattice filter
      x -= k[j] * z[j-1];
      z[j] = z[j-1] + k[j] * x;
    }
    buf[i] = z[0] = x; // output buf[] will be windowed elsewhere
  }
}

void TalkBox::lpc_durbin(float *r, int p, float *k, float *g) {
  int i, j;
  float a[ORD_MAX], at[ORD_MAX], e = r[0];
    
  for (i = 0; i <= p; i++) {
    a[i] = at[i] = 0.0f; //probably don't need to clear at[] or k[]
  }

  for (i = 1; i <= p; i++) {
    k[i] = -r[i];
    for (j = 1; j < i; j++) { 
      at[j] = a[j];
      k[i] -= a[j] * r[i-j]; 
    }
    if (fabs(e) < 1.0e-20f) { 
      e = 0.0f;  
      break; 
    }
    k[i] /= e;
    
    a[i] = k[i];
    for (j = 1; j < i; j++) {
      a[j] = at[j] + k[i] * at[i-j];
    }
    
    e *= 1.0f - k[i] * k[i];
  }

  if (e < 1.0e-20f) {
    e = 0.0f;
  }
  *g = (float)sqrt(e);
}

const char *TalkBox::describe_value(int index, int value) {
  static char txt[20];
  switch (index) {
  case 2:
    if (swap) {
      sprintf(txt, "LEFT");
    } else {
      sprintf(txt, "RIGHT");
    }
    break;
  case 3:
    sprintf(txt, "%4.0f", 5.0f + 95.0f * param[index] * param[index]);
    break;
  default:
    sprintf(txt, "%4.0f%%", 200.0f * param[index]);
  }
  return txt;
}
