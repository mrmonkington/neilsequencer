#include <cmath>

#include "GrainEnv.hpp"

namespace lanternfish {
  GrainEnv::GrainEnv() {
    float phase;
    for (int i = 0; i < table_size; i++) {
      phase = (float)i / (float)table_size;
      this->attack_table[i] = (sin(1.5 * M_PI + M_PI * phase) + 1.0) * 0.5;
      this->release_table[i] = (sin(0.5 * M_PI + M_PI * phase) + 1.0) * 0.5;
    }
    this->counter = 0;
    this->stage = NONE_STAGE;
  }

  void GrainEnv::set_attack_length(int samples) {
    this->attack_length = samples;
  }

  void GrainEnv::set_sustain_length(int samples) {
    this->sustain_length = samples;
  }

  void GrainEnv::set_release_length(int samples) {
    this->release_length = samples;
  }

  void GrainEnv::trigger() {
    if (this->stage == NONE_STAGE) {
      this->stage = ATTACK_STAGE;
      this->counter = 0;
    }
  }

  bool GrainEnv::is_off() {
    return this->stage == NONE_STAGE;
  }
  
  void GrainEnv::process(float *out, int n) {
    float val, phi;
    for (int i = 0; i < n; i++) {
      switch (this->stage) {
      case ATTACK_STAGE:
	phi = (float)this->counter / (float)this->attack_length;
	val = attack_table[int(phi * table_size)];
	if (++this->counter >= this->attack_length) {
	  this->stage = SUSTAIN_STAGE;
	  this->counter = 0;
	}
	break;
      case SUSTAIN_STAGE:
	val = 1.0;
	if (++this->counter >= this->sustain_length) {
	  this->stage = RELEASE_STAGE;
	  this->counter = 0;
	}
	break;
      case RELEASE_STAGE:
	phi = (float)this->counter / (float)this->release_length;
	val = release_table[int(phi * table_size)];
	if (++this->counter >= this->release_length) {
	  this->stage = NONE_STAGE;
	  this->counter = 0;
	}
	break;
      case NONE_STAGE:
	val = 0.0;
	break;
      }
      out[i] = val;
    }
  }
}
