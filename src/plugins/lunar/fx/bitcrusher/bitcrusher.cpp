#include "bitcrusher.h"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>

class bitcrusher : public lunar::fx<bitcrusher> {
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
      crush = *globals->crush;
    if (globals->downsample)
      downsample = *globals->downsample;
  }

  void process_stereo(float *inL, float *inR, 
		      float *outL, float *outR, 
		      int n) {
    int i;
    for (i = 0; i < n; i++) {
      if (--counter == 0) {
	sampleL = inL[i];
	sampleR = inR[i];
	counter = downsample;
      }
      int sL = sampleL * 32768, sR = sampleR * 32768, index = 16 - crush;
      sL = sL >> index;
      sR = sR >> index;
      sL = sL << index;
      sR = sR << index;
      outL[i] = float(sL) / 32768.0;
      outR[i] = float(sR) / 32768.0;
    }
  }
};

lunar_fx *new_fx() { 
  return new bitcrusher(); 
}
