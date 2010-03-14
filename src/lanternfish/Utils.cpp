#include <cmath>
#include <cstdio>

#include "Utils.hpp"

namespace lanternfish {
  float *load_to_float_mono(short *in, int channels, int bytes, int samples) {
    float *out = new float[samples];
    float sample;
    for (int i = 0; i < samples; i++) {
      sample = float(in[channels * i]) / (pow(2.0, bytes * 8) * 0.5);
      out[i] = sample;
    }
    return out;
  }
}
