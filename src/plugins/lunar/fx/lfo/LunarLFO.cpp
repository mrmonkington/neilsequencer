#include "LunarLFO.h"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>

class LunarLFO : public lunar::fx<LunarLFO> {
private:
  static const int tsize = 2048;
  float rate, min, max;

  inline float lookup(float table[], float phi, float min, float max) {
    float temp = min + table[(int)(phi * tsize)] * (max - min);
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
      float phase = (float)i / (float)tsize;
      // sin
      tables[0][i] = (sin(2 * M_PI * phase) + 1.0) * 0.5;
      // saw
      tables[1][i] = phase;
      // sqr
      tables[2][i] = phase < 0.5 ? 0.0 : 1.0;
      // tri
      tables[3][i] = phase < 0.5 ? phase * 2.0 : (1.0 - phase) * 2.0;
    }
  }
  
  void exit() {
    delete this;
  }
  
  void process_events() {
    if (globals->wave)
      table = *globals->wave;
    if (globals->rate) {
      phase = 0.0;
      rate = *globals->rate;
    }
    if (globals->minimum)
      min = *globals->minimum;
    if (globals->maximum)
      max = *globals->maximum;
    
    while (phase >= 1.0)
      phase -= 1.0;

    val = lookup(tables[table], phase, min, max);
    phase += 1.0 / rate;
  }
  
  void process_controller_events() {
    controllers->out1 = &val;
  }
  
  void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {

  }
};

lunar_fx *new_fx() { return new LunarLFO(); }
