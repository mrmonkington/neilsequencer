#include <algorithm>
#include <cstdio>

#include "Philthy.hpp"

LunarPhilthy::LunarPhilthy() {
  global_values = &gval;
  attributes = 0;
  track_values = 0;
}

void LunarPhilthy::update_filters() {
  fk1.setup(_master_info->samples_per_second, l_filter_type, l_cutoff, l_resonance, l_thevfactor);
  fk2.setup(_master_info->samples_per_second, l_filter_type, l_cutoff, l_resonance, l_thevfactor);		
}

void LunarPhilthy::init(zzub::archive *pi) {
  update_filters();
}
	
void LunarPhilthy::process_events() {
  int update = 0;
  if (gval.filter_type != 0xff) {
    l_filter_type = gval.filter_type;
    update = 1;
  }
  if (gval.cutoff != 0xff) {
    l_cutoff = gval.cutoff;
    update = 1;
  }
  if (gval.resonance != 0xff) {
    l_resonance = gval.resonance;
    update = 1;
  }
  if (gval.thevfactor != 0xff) {
    l_thevfactor = gval.thevfactor;
    update = 1;
  }
  if (update) {
    update_filters();
  }
}

bool LunarPhilthy::process_stereo(float **pin, float **pout, int n, int mode) {
  for (int i = 0; i < n; i++) {
    pout[0][i] = pin[0][i];
    pout[1][i] = pin[1][i];
  }
  fk1.process(pout[0], n);
  fk2.process(pout[1], n);
  return true;
}

const char *LunarPhilthy::describe_value(int param, int value) {
  static char txt[20];
  char types[16][30] = {
    "6L Multipeak",
    "6L Separated",
    "6L HiSquelch"
    "4L Skull D",
    "4L TwinPeaks",
    "4L Killah",
    "4L Phlatt",
    "2L Phlatt",
    "2L FrontFlt",
    "2L LaserOne",
    "2L FMish",
    "Notchez",
    "6H Relaxed",
    "6B Plain",
    "6X BatGuy",
    "6X Vocal1",
    "6X Vocal2"
  };
  switch (param) {
  case 0:
    sprintf(txt, "%s", types[value]);
    break;
  default:
    return 0;
  }
  return txt;
}
