#include <cstdio>

#include "MultiBand.hpp"

MultiBand::MultiBand() {
  global_values = &gval;
  track_values = 0;
  attributes = 0;
}

void MultiBand::init(zzub::archive *pi) {

}
	
void MultiBand::process_events() {
  if (gval.listen != 65535) {
    
  }
  if (gval.xover1 != 65535) {
    
  }
  if (gval.xover2 != 65535) {
    
  }
  if (gval.ldrive != 65535) {
    
  }
  if (gval.mdrive != 65535) {
    
  }
  if (gval.hdrive != 65535) {
    
  }
  if (gval.ltrim != 65535) {
    
  }
  if (gval.mtrim != 65535) {
    
  }
  if (gval.htrim != 65535) {
    
  }
  if (gval.attack != 65535) {
    
  }
  if (gval.release != 65535) {
    
  }
  if (gval.width != 65535) {
    
  }
  if (gval.msswap != 65535) {
    
  }
}

bool MultiBand::process_stereo(float **pin, float **pout, int n, int mode) {
  return true;
}

const char *MultiBand::describe_value(int param, int value) {
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
  case 5:
    sprintf(txt, "%d", value);
    break;
  case 6:
    sprintf(txt, "%d", value);
    break;
  case 7:
    sprintf(txt, "%d", value);
    break;
  case 8:
    sprintf(txt, "%d", value);
    break;
  case 9:
    sprintf(txt, "%d", value);
    break;
  case 10:
    sprintf(txt, "%d", value);
    break;
  case 11:
    sprintf(txt, "%d", value);
    break;
  case 12:
    sprintf(txt, "%d", value);
    break;
  default:
    return 0;
  }
  return txt;
}
