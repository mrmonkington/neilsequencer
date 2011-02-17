#include "Controller.hpp"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>

class Controller : public lunar::fx<Controller> {
private:
  float value, power, min, max;
public:
  float val;   
  void init() {

  }
  
  void exit() {
    delete this;
  }
  
  void process_events() {
    if (globals->value)
      this->value = *globals->value;
    if (globals->power)
      this->power = *globals->power;
    if (globals->minimum)
      this->min = *globals->minimum;
    if (globals->maximum)
      this->max = *globals->maximum;
    float power;
    power = 
      this->power > 0.0 ? 
      (this->power + 1.0) : 
      1.0 / (-this->power + 1.0);
    val = this->min + (this->max - this->min) * pow(this->value, power);
  }
  
  void process_controller_events() {
    controllers->out = &val;
  }
  
  void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {

  }
};

lunar_fx *new_fx() { return new Controller(); }
