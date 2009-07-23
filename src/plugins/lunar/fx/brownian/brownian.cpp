
#include "brownian.h"
#include <lunar/fx.hpp>

class brownian : public lunar::fx<brownian> {
private:
  float val, rate, bmin, bmax, step;
  int seed, counter;

  int rand1(void) {
    seed = seed * 1103515245 + 12345;
    return (unsigned int)(seed / 65536) % 32768;
  }

  float rnd() {
    return ((float)rand1() / 32767.0 - 0.5) * 2.0;
  }

public:
  
  void init() {
    seed = 1;
    counter = 0;
    val = rate = bmin = bmax = step = 0.0;
  }
  
  void exit() {
    delete this;
  }
  
  void process_events() {
    if (globals->seed)
      seed = *globals->seed;
    if (globals->rate)
      rate = *globals->rate;
    if (globals->bmin)
      bmin = *globals->bmin;
    if (globals->bmax)
      bmax = *globals->bmax;
    if (globals->step)
      step = *globals->step;
    
    if (counter++ <= rate) {
      ;
    } else {
      val = val + rnd() * step;
      if (val >= bmax)
	val = bmax;
      if (val <= bmin)
	val = bmin;
      counter = 0;
    }
  }
  
  void process_controller_events() {
    controllers->out1 = &val;
  }
  
  void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {
    
  }
  
};

lunar_fx *new_fx() { return new brownian(); }
