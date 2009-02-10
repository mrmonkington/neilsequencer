
#include "lfo.h"
#include <lunar/fx.hpp>

class lfo : public lunar::fx<lfo> {
private:
  static const int tsize = 2048;
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
  float tsin[tsize], tsaw1[tsize], tsaw2[tsize], tsqr[tsize];
  float phase;
  float val;
  int table;
  
  void init() {
    val = 0;
    phase = 0;
    table = 0;
    for (int i = 0; i < tsize; i++)
      tsin[i] = sin(2 * M_PI * ((float)i / (float)tsize));
    for (int i = 0; i < tsize; i++)
      tsaw1[i] = ((float)i / (float)tsize - 0.5) * 2.0;
    for (int i = 0; i < tsize; i++)
      tsaw2[i] = (0.5 - (float)i / (float)tsize) * 2.0;
    for (int i = 0; i < tsize; i++)
      tsqr[i] = (((float)i / (float)tsize) < 0.5) ? -1.0 : 1.0;
  }
  
  void exit() {
    delete this;
  }
  
  void process_events() {
    if (globals->wave)
      table = *globals->wave;
    if (globals->rate)
      rate = *globals->rate;
    if (globals->offset)
      offset = *globals->offset;
    if (globals->amp)
      amp = *globals->amp;

    phase += 1.0 / (transport->ticks_per_second / rate);
    if (phase >= 1.0)
      phase -= 1.0;
    switch (table) {
    case 0:
      val = lookup(tsin, phase, offset, amp);
      break;
    case 1:
      val = lookup(tsaw1, phase, offset, amp);
      break;
    case 2:
      val = lookup(tsaw2, phase, offset, amp);
      break;
    case 3:
      val = lookup(tsqr, phase, offset, amp);
      break;
    }
  }
  
  void process_controller_events() {
    controllers->out1 = &val;
  }
  
  void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {
  }
  
};

lunar_fx *new_fx() { return new lfo(); }
