#include <cstdio>
#include <cmath>
#include <algorithm>

#include "mverb.hpp"

MVerbPlugin::MVerbPlugin() {
  global_values = &gval;
  attributes = NULL;
  track_values = NULL;
}

MVerbPlugin::~MVerbPlugin() {
  delete this;
}

void MVerbPlugin::init(zzub::archive* pi) {
  reverb.reset(_master_info->samples_per_second);
}

void MVerbPlugin::destroy() {

}

void MVerbPlugin::process_events() {
  if (gval.paraDamping != 0xff) {
    reverb.setParameter(reverb.DAMPINGFREQ, gval.paraDamping / 254.0);
  }
  if (gval.paraDensity != 0xff) {
    reverb.setParameter(reverb.DENSITY, gval.paraDensity / 254.0);
  }
  if (gval.paraBandwidth != 0xff) {
    reverb.setParameter(reverb.BANDWIDTHFREQ, gval.paraBandwidth / 254.0);
  }
  if (gval.paraPredelay != 0xff) {
    reverb.setParameter(reverb.PREDELAY, gval.paraPredelay / 254.0);
  }
  if (gval.paraSize != 0xff) {
    reverb.setParameter(reverb.SIZE, gval.paraSize / 254.0);
  }
  if (gval.paraDecay != 0xff) {
    reverb.setParameter(reverb.DECAY, gval.paraDecay / 254.0);
  }
  if (gval.paraMix != 0xff) {
    reverb.setParameter(reverb.MIX, gval.paraMix / 254.0);
  }
  if (gval.paraEarlyMix != 0xff) {
    reverb.setParameter(reverb.EARLYMIX, gval.paraEarlyMix / 254.0);
  }
}

bool MVerbPlugin::process_stereo(float **pin, float **pout, int n, int mode) {
  reverb.process(pin, pout, n);
  return true;
}

const char *MVerbPlugin::describe_value(int param, int value) {
  static const int LABEL_SIZE = 20;
  static char str[LABEL_SIZE];
  sprintf(str, "%.2f%%", (value / 254.0) * 100.0);
  return str;
}



