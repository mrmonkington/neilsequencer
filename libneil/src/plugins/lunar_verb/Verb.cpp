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
 
}

bool LunarVerb::process_stereo(float **pin, float **pout, int n, int mode) {
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
