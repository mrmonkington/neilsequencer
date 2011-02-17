#include "envelope.hpp"

envelope::envelope() {
  envcoef = 0;
  suscounter = 0;
}

envelope::~envelope() {

}

void envelope::reset() {
  if (envstate == ENV_NONE) {
    envstate = ENV_ATT;
    envcoef = 1.0f / (float)a;
  }
  else {
    envstate = ENV_CLICK;
    envcoef = envvol / 32.0f;
  }
}

void envelope::attack(int newv) {
  a = newv;
}

void envelope::decay(int newv) {
  d = newv;
}

void envelope::sustain(int newv) {
  s = newv;
}

void envelope::release(int newv) {
  r = newv;
}

void envelope::sustainv(float newv) {
  susvol = newv;
}

void envelope::stop() {
  envvol = 0;
  envstate = ENV_NONE;
}

void envelope::noteoff() {
  if (envstate != ENV_NONE) {
    envstate = ENV_REL;
    envcoef = envvol / (float)r;
  }
}

inline float envelope::res(void) {
  if (envstate != ENV_NONE) {
    // Attack
    if (envstate == ENV_ATT) {
      envvol += envcoef;
      if (envvol > 1.0f) {
	envvol = 1.0f;
	envstate = ENV_DEC;
	envcoef = (1.0f - susvol) / (float)d;
      }
    }
    // Decay
    if (envstate == ENV_DEC) {
      envvol -= envcoef;
      if (envvol < susvol) {
	envvol = susvol;
	envstate = ENV_SUS;
	suscounter = 0;
      }
    }
    // Sustain
    if (envstate == ENV_SUS) {
      // When in sustain do nothing.
    }
    // Release
    if (envstate == ENV_REL) {
      envvol -= envcoef;
      if (envvol <= 0.0) {
	envvol = 0.0;
	envstate = ENV_NONE;
      }
    }
    if (envstate == ENV_CLICK) {
      envvol -= envcoef;
      if (envvol <= 0.0) {
	envvol = 0.0;
	envstate = ENV_ATT;
	envcoef = 1.0f / (float)a;
      }
    }
    return envvol;
  }
  else 
    return 0;
}
