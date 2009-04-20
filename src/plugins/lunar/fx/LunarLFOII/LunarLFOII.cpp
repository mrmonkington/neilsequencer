#include "LunarLFOII.h"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>

class LunarLFOII : public lunar::fx<LunarLFOII> {
private:
  static const int tsize = 2048;
  bool invert;
  float rate, amp, offset;

  inline float lookup(float table[], float phi, 
		      float offset, float amp) {
    float temp = offset + table[(int)(phi * tsize)] * amp;
    if (temp <= 0.0)
      return 0.0;
    if (temp >= 1.0)
      return 1.0;
    return temp;
  }

public:
  float phase;
  float val;
  int table;
  float tables[32][tsize];
  
  void init() {
    val = 0;
    phase = 0;
    table = 0;
    for (int i = 0; i < tsize; i++) {
      // sin
      tables[0][i] = sin(2 * M_PI * ((float)i / (float)tsize));
      // sin^3
      tables[1][i] = pow(sin(2 * M_PI * ((float)i / (float)tsize)), 3.0);
      // saw
      tables[2][i] = ((float)i / (float)tsize - 0.5) * 2.0;
      // saw^2
      tables[3][i] = (pow((float)i / (float)tsize, 2.0) - 0.5) * 2.0;
      // saw^3
      tables[4][i] = (pow((float)i / (float)tsize, 3.0) - 0.5) * 2.0;
      // pulse (50%)
      tables[5][i] = (((float)i / (float)tsize) < 0.5) ? -1.0 : 1.0;
      // pulse (40%)
      tables[6][i] = (((float)i / (float)tsize) < 0.4) ? -1.0 : 1.0;
      // pulse (30%)
      tables[7][i] = (((float)i / (float)tsize) < 0.3) ? -1.0 : 1.0;
      // pulse (20%)
      tables[8][i] = (((float)i / (float)tsize) < 0.2) ? -1.0 : 1.0;
      // pulse (10%)
      tables[9][i] = (((float)i / (float)tsize) < 0.1) ? -1.0 : 1.0;
    }
  }
  
  void exit() {
    delete this;
  }
  
  void process_events() {
    if (globals->wave)
      table = *globals->wave;
    if (globals->rate)
      rate = *globals->rate;
    if (globals->invert) {
      if (*globals->invert == 0)
	invert = false;
      else
	invert = true;
    }
    if (globals->offset)
      offset = *globals->offset;
    if (globals->amp)
      amp = *globals->amp;

    if (invert)
      phase -= 1.0 / rate;
    else
      phase += 1.0 / rate;
    
    if (phase <= 0.0)
      phase += 1.0;
    if (phase >= 1.0)
      phase -= 1.0;

    val = lookup(tables[table], phase, offset, amp);
  }
  
  void process_controller_events() {
    controllers->out1 = &val;
  }
  
  void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {
  }
};

lunar_fx *new_fx() { return new LunarLFOII(); }
