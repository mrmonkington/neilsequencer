#include <algorithm>
#include <cstdio>

#include "Verb.hpp"

LunarVerb::LunarVerb() {
  global_values = &gval;
  attributes = 0;
  track_values = 0;
}

void LunarVerb::init(zzub::archive *pi) {

}
	
void LunarVerb::process_events() {
  if (gval.roomsize != 0xffff) {
    setroomsize(gval.roomsize / (float)para_roomsize->value_max);
  }
  if (gval.damp != 0xffff) {
    setdamp(gval.damp / (float)para_damp->value_max);
  }
  if (gval.wet != 0xffff) {
    printf("%d\n", gval.wet);
    setwet(dbtoamp(-48.0f + (gval.wet / 6000.0f) * 60.0f, -48.0f));
  }
  if (gval.dry != 0xffff) {
    setdry(dbtoamp(-48.0f + (gval.dry / 6000.0f) * 60.0f, -48.0f));
  }
  if (gval.width != 0xffff) {
    setwidth(gval.width / (float)para_width->value_max);
  }
  if (gval.freeze != para_freeze->value_none) {
    setmode(gval.freeze);
  }
}

bool LunarVerb::process_stereo(float **pin, float **pout, int n, int mode) {
  processreplace(pin[0], pin[1], pout[0], pout[1], n, 1);
  return true;
}

const char *LunarVerb::describe_value(int param, int value) {
  static char txt[20];
  switch(param) {
  case 0:
  case 1:
  case 4:
    sprintf(txt, "%.2f", value * 0.01f);
    break;
  case 2:
  case 3:
    sprintf(txt, "%.2f dB", -48.0 + (value / 6000.0f) * 60.0);
    break;
  case 5:
    sprintf(txt, value ? "on" : "off");
    break;
  default:
    return 0;
  }
  return txt;
}
