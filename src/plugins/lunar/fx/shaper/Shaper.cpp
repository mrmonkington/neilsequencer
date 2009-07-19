#include "CustomLFO.h"
#include <lunar/fx.hpp>

#define NSTEPS 8

class CustomLFO : public lunar::fx<CustomLFO> {
private:
  float steps[NSTEPS];
  float times[NSTEPS];
  float val, min, max;
  int counter, length, position, rate, step;

  inline float cubic(float y0, float y1, float y2, float y3, float mu) {
    // Cubic interpolation.
    float a0, a1, a2, a3, mu2;
    mu2 = mu * mu;
    a0 = y3 - y2 - y0 + y1;
    a1 = y0 - y1 - a0;
    a2 = y2 - y0;
    a3 = y1;
    return (a0 * mu * mu2 + a1 * mu2 + a2 * mu + a3);
  }

public:
  void init() {
    counter = 0;
    val = 0.0;
    step = 0;
  }
  
  void exit() {
    delete this;
  }
  
  void process_events() {
    if (globals->length)
      length = *globals->length;
    if (globals->step1)
      steps[0] = *globals->step1;
    if (globals->time1)
      times[0] = *globals->time1;
    if (globals->step2)
      steps[1] = *globals->step2;
    if (globals->time2)
      times[1] = *globals->time2;
    if (globals->step3)
      steps[2] = *globals->step3;
    if (globals->time3)
      times[2] = *globals->time3;
    if (globals->step4)
      steps[3] = *globals->step4;
    if (globals->time4)
      times[3] = *globals->time4;
    if (globals->step5)
      steps[4] = *globals->step5;
    if (globals->time5)
      times[4] = *globals->time5;
    if (globals->step6)
      steps[5] = *globals->step6;
    if (globals->time6)
      times[5] = *globals->time6;
    if (globals->step7)
      steps[6] = *globals->step7;
    if (globals->time7)
      times[6] = *globals->time7;
    if (globals->step8)
      steps[7] = *globals->step8;
    if (globals->time8)
      times[7] = *globals->time8;
    if (globals->minimum)
      this->min = *globals->minimum;
    if (globals->maximum)
      this->max = *globals->maximum;
  }
  
  void process_controller_events() {
    int i0, i1, i2, i3;
    i0 = (step - 1) % this->length;
    if (i0 < 0)
      i0 = this->length + i0;
    i1 = step;
    i2 = (step + 1) % this->length;
    i3 = (step + 2) % this->length;

    float mu = (float)counter / (float)times[step];

    val = cubic(steps[i0], steps[i1], steps[i2], steps[i3], mu);
    val = this->min + val * (this->max - this->min);

    if (++counter > times[step]) {
      counter = 0;
      if (++step >= this->length) {
	step = 0;
      }
    }
    
    if (val < 0.0)
      val = 0.0;
    if (val > 1.0)
      val = 1.0;
    controllers->out1 = &val;
  }  
};

lunar_fx *new_fx() { return new CustomLFO(); }
