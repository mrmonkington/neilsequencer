#include <zzub/plugin.h>
#include <cstdlib>

#include "gAHDEnv.hpp"
#include "AHD4.hpp"
#include "CGrain.hpp"

void CGrain::Init() {
  IsActive = 0;
  Reverse = false;
  IsStereo = true;
  Duration = 1000;
  Pos = 0;
  Wave = 1;
  EnvType = 0;
  Amp = 1.0f;
  Offset = 0.0f;
  PanPos = 1.0f;
  RPan = LPan = 1.0f;
  Rate = 1.0f;
  outL = outR = 0.0f;
  gEnv.Init();
  GCount = 0;
  CDown = 0;
  EnvLen = 0;
  fEnvLen = 0.0f;
};

void CGrain::Set(int dur, double offs, int envt, float rate, float pan) {
  Offset = offs;
  Rate = rate;
  fEnvLen = 1.0f / dur;
  Duration = EnvLen = dur;
  GCount = 0;
  Pos = 0;
  LPan = RPan = 1.0f;
  if (pan < 0.5) 
    RPan = pan * 2;
  if (pan > 0.5) 
    LPan = (1.0f - pan) * 2;
}

void CGrain::SetWave(int wave, int stereo, const zzub::wave_level *pW) {
  Wave = wave;
  IsStereo = stereo;
  pwv = pW;
  if (!pwv) 
    return;
  spsMix = (float)pwv->samples_per_second / (float) * pSPS;
  Pos = float(Offset);
  lastNumSamples = pwv->sample_count;
  int iPos = (int)Pos;
  if ((Duration + (iPos / Rate)) > pwv->sample_count / Rate) {
    Duration = pwv->sample_count - (pwv->sample_count-iPos);
    if (Rate > 1.0) 
      Duration = (int)((pwv->sample_count - iPos) / Rate);
  }
  fEnvLen = 1.0f / Duration;       
}

inline int f2i(double d) {
  const double magic = 6755399441055744.0; // 2^51 + 2^52
  double tmp = (d - 0.5) + magic;
  return *(int*)&tmp;
}

void CGrain::SetMiPointers(int * sps) {
  pSPS = sps;
}

inline void CGrain::next_sample(float *left, float *right) {
  int x, x2;
  float frac;
  if (!IsActive) {
    *left = *right = 0.0f;
  } else {
    float env = ahd.ProcEnv();
    if (IsStereo) {
      x = f2i(Pos) << 1;
      x2 = x + 2;
      if(x2 >= pwv->sample_count * 2) 
	x2 = 0;
      frac = ((Pos) - (float)f2i(Pos));
      *left = pwv->samples[x] + frac * (pwv->samples[x2] - pwv->samples[x]);
      x++;
      x2++;
      *right = pwv->samples[x] + frac * (pwv->samples[x2] - pwv->samples[x]);
    } else {
      x = f2i(Pos);
      x2 = x + 1;
      if(x2 >= pwv->sample_count) 
	x2 = 0;
      *left = pwv->samples[x] + 
	((Pos) - (float)x)*(pwv->samples[x2] - pwv->samples[x]);
      *right = outL;
    }
    // Apply the envelope and panning.
    *left *= env * Amp * LPan;
    *right *= env * Amp * RPan;
    //move ahead a bit
    Pos += spsMix * Rate;
    GCount++;//increment grain len counter
    if (GCount > Duration || Pos > pwv->sample_count) {
      Pos = 0;
      IsActive = false;
      GCount = 0;
    }
  }
}

void CGrain::SetAmp(float t, float b, float wa) {
  float a = (float)rand() / RAND_MAX;
  Amp = ((t - b) * a + b) * wa; //wa is the wave amp...
}

void CGrain::SetEnv(int length, float amt, float skew) {
  ahd.SetEnv(length, amt, skew);
}

void CGrain::GenerateAdd(float *psamples, int numsamples, 
			 const zzub::wave_level *wave) {
  float left, right;
  this->pwv = wave;
  if (!pwv || lastNumSamples != pwv->sample_count) {
    IsActive = false;
  } else {
    for (int i = 0; i < numsamples * 2; i += 2) {
      if (--CDown < 0) 
	next_sample(&left, &right);
      psamples[i] += left;
      psamples[i + 1] += right;
    }
  }
}
