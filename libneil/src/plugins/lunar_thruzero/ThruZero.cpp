#include "ThruZero.hpp"

ThruZero::ThruZero() {
  global_values = &gval;
  attributes = 0;
  track_values = 0;
}

void ThruZero::init(zzub::archive *pi) {

}
	
void ThruZero::process_events() {
  if (gvals.rate != 255) {
    
  }
  if (gvals.depth != 255) {
    
  }
  if (gvals.mix != 255) {
    
  }
  if (gvals.feedback != 255) {
    
  }
  if (gvals.depthmod != 255) {
    
  }
}

bool ThruZero::process_stereo(float **pin, float **pout, int n, int mode) {
  return true;
}

const char *ThruZero::describe_value(int param, int value) {
  static char txt[20];
  switch (param) {
  case 0:
    sprintf(txt, "%d", value);
    break;
  case 1:
    sprintf(txt, "%d", value);
    break;
  case 2:
    sprintf(txt, "%d", value);
    break;
  case 3:
    sprintf(txt, "%d", value);
    break;
  case 4:
    sprintf(txt, "%d", value);
    break;
  default:
    return 0;
  }
  return txt;
}
