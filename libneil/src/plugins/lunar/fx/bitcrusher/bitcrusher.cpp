#include "bitcrusher.h"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>

class bitcrusher : public lunar::fx<bitcrusher> {
private:
  inline int keep_bits_from_16(int input, int keep_bits) {
    // Only keep the last 16 bits.
    input &= 0xffff;
    input &= (-1 << (16 - keep_bits));
    // Only keep the last 16 bits.
    input &= 0xffff;
    return input;
  }

public:
  float sampleL, sampleR;
  int crush, downsample, counter;
  
  void init() {
    crush = 16;
    downsample = 1;
    sampleL = 0.0;
    sampleR = 0.0;
    counter = downsample;
  }

  void exit() {
    delete this;
  }

  void process_events() {
    if (globals->crush)
      this->crush = *globals->crush;
    if (globals->downsample)
      this->downsample = *globals->downsample;
  }

  void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {
    for (int i = 0; i < n; i++) {
      // Only play back every <downsample> samples.
      if (--this->counter == 0) {
	this->sampleL = inL[i];
	this->sampleR = inR[i];
	this->counter = downsample;
      }
      // Bitcrush the samples.
      int in_left = (int)round((sampleL + 1.0) * 0.5 * 65536.0);
      int in_right = (int)round((sampleR + 1.0) * 0.5 * 65536.0);
      int out_left = keep_bits_from_16(in_left, this->crush);
      int out_right = keep_bits_from_16(in_right, this->crush);
      outL[i] = (((float)out_left / 65536.0) - 0.5) * 2.0;
      outR[i] = (((float)out_right / 65536.0) - 0.5) * 2.0;
    }
  }
};

lunar_fx *new_fx() { 
  return new bitcrusher(); 
}
