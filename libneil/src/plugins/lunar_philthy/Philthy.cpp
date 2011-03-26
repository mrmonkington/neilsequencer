#include <algorithm>
#include <cstdio>
#include <string>

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
  switch (param) {
  case 0:
    switch (value) {
    case 0:
      sprintf(txt, "6L Multipeak");
      break;
    case 1:
      sprintf(txt, "6L Separated");
      break;
    case 2:
      sprintf(txt, "6L HiSquelch");
      break;
    case 3:
      sprintf(txt, "4L Skull D");
      break;
    case 4:
      sprintf(txt, "4L TwinPeaks");
      break;
    case 5:
      sprintf(txt, "4L Killah");
      break;
    case 6:
      sprintf(txt, "4L Phlatt");
      break;
    case 7:
      sprintf(txt, "2L Phlatt");
      break;
    case 8:
      sprintf(txt, "2L FrontFlt");
      break;
    case 9:
      sprintf(txt, "2L LaserOne");
      break;
    case 10:
      sprintf(txt, "2L FMish");
      break;
    case 11:
      sprintf(txt, "Notchez");
      break;
    case 12:
      sprintf(txt, "6H Relaxed");
      break;
    case 13:
      sprintf(txt, "6B Plain");
      break;
    case 14:
      sprintf(txt, "6X BatGuy");
      break;
    case 15:
      sprintf(txt, "6X Vocal1");
      break;
    case 16:
      sprintf(txt, "6X Vocal2");
      break;
    }
    break;
  default:
    return 0;
  }
  return txt;
}
