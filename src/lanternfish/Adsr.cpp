#include <cmath>

#include "Adsr.hpp"

namespace lanternfish {
  Adsr::Adsr() {
    set_sustain_level(0.5);
    set_attack_time(1024);
    set_decay_time(1024);
    set_release_time(1024);
    set_power(1.0);
    this->current_stage = NONE_STAGE;
  }
  
  Adsr::~Adsr() {

  }
  
  void Adsr::set_attack_time(int time) {
    attack_time = time;
    attack_delta = 1.0 / (float)time;
  }

  void Adsr::set_decay_time(int time) {
    decay_time = time;
    decay_delta = -(1.0 - sustain_level) / (float)time;
  }
  
  void Adsr::set_sustain_level(float level) {
    sustain_level = level;
    decay_delta = -(1.0 - sustain_level) / (float)decay_time;
    release_delta = -sustain_level / (float)release_time;
  }
  
  void Adsr::set_release_time(int time) {
    release_time = time;
    release_delta = -sustain_level / (float)time;
  }
  
  void Adsr::set_power(float power) {
    this->power = power;
  }
  
  void Adsr::note_on() {
    current_stage = ATTACK_STAGE;
  }
  
  void Adsr::note_off() {
    this->value = 0.0;
    this->current_stage = RELEASE_STAGE;
  }
  
  void Adsr::process(float *out, int n) {
    if (this->current_stage != NONE_STAGE) {
      for (int i = 0; i < n; i++) {
	switch(this->current_stage) {
	case ATTACK_STAGE:
	  value += attack_delta;
	  if (value >= 1.0) {
	    current_stage = DECAY_STAGE;
	  }
	  break;
	case DECAY_STAGE:
	  value += decay_delta;
	  if (value <= sustain_level) {
	    current_stage = SUSTAIN_STAGE;
	  }
	  break;
	case SUSTAIN_STAGE:
	  value = sustain_level;
	  break;
	case RELEASE_STAGE:
	  value += release_delta;
	  if (value <= 0.0) {
	    current_stage = NONE_STAGE;
	    value = 0.0;
	  }
	  break;
	}
	out[i] = value;
      }
    } else {
      for (int i = 0; i < n; i++) {
	out[i] = 0.0;
      }
    }
  }
}
