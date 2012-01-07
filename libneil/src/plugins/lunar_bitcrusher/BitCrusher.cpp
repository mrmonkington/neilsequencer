#include <algorithm>
#include <cstdio>

#include "BitCrusher.hpp"

LunarBitCrusher::LunarBitCrusher() {
  global_values = &gval;
  attributes = 0;
  track_values = 0;
}

void LunarBitCrusher::init(zzub::archive *pi) {
  crush = 16;
  downsample = 1;
  sampleL = 0.0;
  sampleR = 0.0;
  counter = downsample;
}
	
void LunarBitCrusher::process_events() {
  if (gval.crush != 0xff) {
    crush = gval.crush;
  }
  if (gval.downsample != 0xff) {
    downsample = gval.downsample;
  }
}

bool LunarBitCrusher::process_stereo(float **pin, float **pout, int n, int mode) {
  if (mode == zzub::process_mode_write || mode == zzub::process_mode_no_io) {
    return false;
  }
  if (mode == zzub::process_mode_read) {
    return true;
  }
  for (int i = 0; i < n; i++) {
    // Only play back every <downsample> samples.
    if (--counter == 0) {
      sampleL = pin[0][i];
      sampleR = pin[1][i];
      counter = downsample;
    }
    // Bitcrush the samples.
    int in_left = (int)round((sampleL + 1.0) * 0.5 * 65536.0);
    int in_right = (int)round((sampleR + 1.0) * 0.5 * 65536.0);
    int out_left = keep_bits_from_16(in_left, crush);
    int out_right = keep_bits_from_16(in_right, crush);
    pout[0][i] = (((float)out_left / 65536.0) - 0.5) * 2.0;
    pout[1][i] = (((float)out_right / 65536.0) - 0.5) * 2.0;
  } 
  return true;
}

const char *LunarBitCrusher::describe_value(int param, int value) {
  static char txt[20];
  switch(param) {
  case 0:
    sprintf(txt, "%d bits", value);
    break;
  case 1:
    sprintf(txt, "%d times", value);
    break;
  default:
    return 0;
  }
  return txt;
}
