#include <cmath>

#include "Adsr.hpp"

namespace lanternfish {
  Adsr::Adsr() {
    set_attack_time(1024);
    set_decay_time(1024);
    set_sustain_level(0.5);
    set_release_time(1024);
    set_power(1.0);
    this->current_stage = NONE_STAGE;
  }
  
  Adsr::~Adsr() {
    
  }
  
  void Adsr::set_attack_time(int time) {
    this->attack_time = time;
    this->attack_delta = 1.0 / (float)time;
  }

  void Adsr::set_decay_time(int time) {
    this->decay_time = time;
    this->decay_delta = 1.0 / (float)time;
  }
  
  void Adsr::set_sustain_level(float level) {
    this->sustain_level = level;
  }
  
  void Adsr::set_release_time(int time) {
    this->release_time = time;
    this->release_delta = 1.0 / (float)time;
  }
  
  void Adsr::set_power(float power) {
    this->power = power;
  }
  
  void Adsr::note_on() {
    this->value = 0.0;
    this->current_stage = ATTACK_STAGE;
  }
  
  void Adsr::note_off() {
    this->value = sustain_level;
    this->current_stage = RELEASE_STAGE;
  }
  
  void Adsr::process(int n) {
    if (this->out.size() != n) {
      this->out.resize(n);
    }
    float return_value;
    if (this->current_stage != NONE_STAGE) {
      for (int i = 0; i < n; i++) {
	switch(this->current_stage) {
	case ATTACK_STAGE:
	  return_value = pow(this->value, this->power);
	  this->value += this->attack_delta;
	  if (this->value >= 1.0) {
	    this->value = 0.0;
	    this->current_stage = DECAY_STAGE;
	  }
	  break;
	case DECAY_STAGE:
	  return_value = 
	    1.0 - pow(this->value, this->power) * (1.0 - this->sustain_level);
	  this->value += this->decay_delta;
	  if (this->value >= 1.0) {
	    this->value = 0.0;
	    this->current_stage = SUSTAIN_STAGE;
	  }
	  break;
	case SUSTAIN_STAGE:
	  this->out[i] = this->sustain_level;
	  break;
	case RELEASE_STAGE:
	  return_value =
	    this->sustain_level - 
	    pow(this->value, this->power) * this->sustain_level;
	  this->value += this->release_delta;
	  if (this->value >= 1.0) {
	    this->value = 0.0;
	    this->current_stage = NONE_STAGE;
	  }
	  break;
	}
	this->out[i] = return_value;
      }
    } else {
      for (int i = 0; i < n; i++) {
	this->out[i] = 0.0;
      }
    }
  }
}
