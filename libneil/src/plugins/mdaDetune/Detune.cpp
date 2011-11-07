#include <cstdio>
#include <cmath>
#include <cstring>

#include "Detune.hpp"

Detune::Detune() {
  global_values = &gval;
  track_values = 0;
  attributes = 0;
}

void Detune::init(zzub::archive *pi) {
  param[0] = 0.20f; //fine
  param[1] = 0.90f; //mix
  param[2] = 0.50f; //output
  param[3] = 0.50f; //chunksize

  ///initialise...
  suspend();
  
  semi = 3.0f * 0.20f * 0.20f * 0.20f;
  dpos2 = (float)pow(1.0594631f, semi);
  dpos1 = 1.0f / dpos2;
  wet = 1.0f;
  dry = wet - wet * 0.90f * 0.90f;
  wet = (wet + wet - wet * 0.90f) * 0.90f;
}

void Detune::suspend() ///clear any buffers...
{
  memset(buf, 0, sizeof(buf));
  memset(win, 0, sizeof(win));
  pos0 = 0; 
  pos1 = pos2 = 0.0f;
  
  //recalculate crossfade window
  buflen = 1 << (8 + (int)(4.9f * param[3]));
  if (buflen > BUFMAX) {
    buflen = BUFMAX;
  }
  bufres = 1000.0f * (float)buflen / _master_info->samples_per_second;

  int i; // hanning half-overlap-and-add
  double p = 0.0, dp = 6.28318530718 / buflen;
  for (i = 0; i < buflen; i++) {
    win[i] = (float)(0.5 - 0.5 * cos(p)); 
    p += dp; 
  }
}
	
void Detune::process_events() {
  if (gval.fine != 65535) {
    param[0] = gval.fine * 0.001f;
    semi = 3.0f * param[0] * param[0] * param[0];
    dpos2 = (float)pow(1.0594631f, semi);
    dpos1 = 1.0f / dpos2;
  }
  if (gval.mix != 65535) {
    param[1] = gval.mix * 0.001f;
    wet = (float)pow(10.0f, 2.0f * param[2] - 1.0f);
    dry = wet - wet * param[1] * param[1];
    wet = (wet + wet - wet * param[1]) * param[1];
  }
  if (gval.output != 65535) {
    param[2] = gval.output * 0.001f;
    wet = (float)pow(10.0f, 2.0f * param[2] - 1.0f);
    dry = wet - wet * param[1] * param[1];
    wet = (wet + wet - wet * param[1]) * param[1];
  }
  if (gval.chunksize != 65535) {
    param[3] = gval.chunksize * 0.001f;
    int tmp = 1 << (8 + (int)(4.9f * param[3]));
    if (tmp != buflen) {
      // recalculate crossfade window
      buflen = tmp;
      if (buflen > BUFMAX) {
	buflen = BUFMAX;
      }
      bufres = 1000.0f * (float)buflen / _master_info->samples_per_second;
	
      int i; // hanning half-overlap-and-add
      double p = 0.0, dp = 6.28318530718 / buflen;
      for (i = 0; i < buflen; i++) { 
	win[i] = (float)(0.5 - 0.5 * cos(p)); 
	p += dp; 
      }
    }
  }
}

bool Detune::process_stereo(float **pin, float **pout, int sampleFrames, int mode) {
  float *in1 = pin[0];
  float *in2 = pin[1];
  float *out1 = pout[0];
  float *out2 = pout[1];
  float a, b, c, d;
  float x, w = wet, y = dry, p1 = pos1, p1f, d1 = dpos1;
  float p2 = pos2, d2 = dpos2;
  int p0 = pos0, p1i, p2i;
  int l = buflen - 1, lh = buflen >> 1;
  float lf = (float)buflen;

  --in1;
  --in2;
  --out1;
  --out2;
  while (--sampleFrames >= 0) {
    a = *++in1;
    b = *++in2;

    c = y * a;
    d = y * b;

    --p0 &= l;
    *(buf + p0) = w * (a + b); // input

    p1 -= d1;
    if (p1 < 0.0f) {
      p1 += lf; // output
    }
    p1i = (int)p1;
    p1f = p1 - (float)p1i;
    a = *(buf + p1i);
    ++p1i &= l;
    a += p1f * (*(buf + p1i) - a); // linear interpolation

    p2i = (p1i + lh) & l; // 180-degree output
    b = *(buf + p2i);
    ++p2i &= l;
    b += p1f * (*(buf + p2i) - b); // linear interpolation

    p2i = (p1i - p0) & l; // crossfade
    x = *(win + p2i);
    c += b + x * (a - b);

    p2 -= d2; // repeat for downwards shift - can't see a more efficient way?
    if (p2 < 0.0f) {
      p2 += lf; // output
    }
    p1i = (int)p2;
    p1f = p2 - (float)p1i;
    a = *(buf + p1i);
    ++p1i &= l;
    a += p1f * (*(buf + p1i) - a); // linear interpolation

    p2i = (p1i + lh) & l; // 180-degree ouptut
    b = *(buf + p2i);
    ++p2i &= l;
    b += p1f * (*(buf + p2i) - b); // linear interpolation

    p2i = (p1i - p0) & l; // crossfade
    x = *(win + p2i);
    d += b + x * (a - b);

    *++out1 = c;
    *++out2 = d;
  }
  pos0 = p0; pos1 = p1; pos2 = p2;
  return true;
}

const char *Detune::describe_value(int index, int value) {
  static char txt[20];
  switch (index) {
  case 1:
    sprintf(txt, "%.0f%%", 99.0f * param[index]);
    break;
  case 2:
    sprintf(txt, "%.1f dB", 40.0f * param[index] - 20.0f);
    break;
  case 3:
    sprintf(txt, "%.1f ms", bufres);
    break;
  default:
    sprintf(txt, "%.1f cents", 100.0f * semi);
    break;
  }
  return txt;
}
