#include <cmath>
#include <cstdio>

#include "Adsr.hpp"

#define LOWER_BOUND 0.00001

#define log_b(b,x) (log(x)/log(b))

namespace lanternfish {
  Adsr::Adsr() {
    set_sustain_level(0.5);
    set_attack_time(1024);
    set_decay_time(1024);
    set_release_time(1024);
    set_peak_level(1.0);
    current_stage = NONE_STAGE;
    log_c = 8.0;
  }
  
  Adsr::~Adsr() {

  }
  
  void Adsr::set_attack_time(int time) {
    if (time > 0)
      attack_time = time;
    else
      attack_time = 1;
  }

  void Adsr::set_decay_time(int time) {
    if (time > 0)
      decay_time = time;
    else
      decay_time = 1;
  }

  void Adsr::set_peak_level(float level) {
    if (level > LOWER_BOUND)
      peak_level = level;
    else
      peak_level = LOWER_BOUND;
  }
  
  void Adsr::set_sustain_level(float level) {
    if (level > LOWER_BOUND)
      sustain_level = level;
    else
      sustain_level = LOWER_BOUND;
  }
  
  void Adsr::set_release_time(int time) {
    if (time > 0)
      release_time = time;
    else
      release_time = 1;
  }
    
  void Adsr::note_on() {
    current_stage = ATTACK_STAGE;
    coeff = (log_b(log_c, peak_level) - log_b(log_c, value)) / 
      float(attack_time);
  }
  
  void Adsr::note_off() {
    if (current_stage != NONE_STAGE) {
      current_stage = RELEASE_STAGE;
      coeff = (log_b(log_c, LOWER_BOUND) - log_b(log_c, value)) / 
	float(release_time);
    }
  }

  void Adsr::set_log_c(float log_c) {
    this->log_c = log_c;
  }

  void Adsr::print_stats() {
    printf("stage = %d\n", current_stage);
    printf("coeff = %.3f\n", coeff);
  }
  
  void Adsr::process(float *out, int n) {
    for (int i = 0; i < n; i++) {
      switch(this->current_stage) {
      case NONE_STAGE:
	value = LOWER_BOUND;
	break;
      case ATTACK_STAGE:
	value += coeff * value;
	if (value >= peak_level) {
	  current_stage = DECAY_STAGE;
	  coeff = (log_b(log_c, sustain_level) - log_b(log_c, value)) / 
	    float(decay_time);
	}
	break;
      case DECAY_STAGE:
	value += coeff * value;
	if (value <= sustain_level) {
	  current_stage = SUSTAIN_STAGE;
	}
	break;
      case SUSTAIN_STAGE:
	value = sustain_level;
	break;
      case RELEASE_STAGE:
	value += coeff * value;
	if (value <= LOWER_BOUND) {
	  current_stage = NONE_STAGE;
	  value = LOWER_BOUND;
	}
	break;
      }
      out[i] = value;
    }
  }
}
