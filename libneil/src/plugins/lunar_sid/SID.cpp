#include <cstdio>

#include "SID.hpp"

SID::SID() {
  global_values = &gval;
  track_values = &gval;
  attributes = 0;
}

void SID::init(zzub::archive *pi) {

}
	
void SID::process_events() {
  if (gval.chipset != zzub::switch_value_none) {
    
  }
  if (gval.cutoff != 65535) {
    
  }
  if (gval.resonance != 255) {
    
  }
  if (gval.mode != 255) {
    
  }
  if (gval.volume != 255) {
    
  }
  if (tval.note != zzub::note_value_none) {
    
  }
  if (tval.effect != 0) {
    
  }
  if (tval.effectvalue != 0) {
    
  }
  if (tval.pw != 65535) {
    
  }
  if (tval.wave != 255) {
    
  }
  if (tval.filtervoice != zzub::switch_value_none) {
    
  }
  if (tval.ringmod != zzub::switch_value_none) {
    
  }
  if (tval.sync != zzub::switch_value_none) {
    
  }
  if (tval.attack != 255) {
    
  }
  if (tval.decay != 255) {
    
  }
  if (tval.sustain != 255) {
    
  }
  if (tval.release != 255) {
    
  }
}

bool SID::process_stereo(float **pin, float **pout, int n, int mode) {
  return true;
}

const char *SID::describe_value(int param, int value) {
  static char txt[20];
  switch (param) {
  case 0:
    if (value == 0) {
      sprintf(txt, "%s", "MOS6581");
    } else if (value == 1) {
      sprintf(txt, "%s", "MOS8580");
    }
    break;
  case 1:
    sprintf(txt, "%d", value);
    break;
  case 2:
    sprintf(txt, "%d", value);
    break;
  case 3:
    if (value == 0) {
      sprintf(txt, "%s", "LP");
    } else if (value == 1) {
      sprintf(txt, "%s", "BP");
    } else if (value == 2) {
      sprintf(txt, "%s", "HP");
    } else if (value == 3) {
      sprintf(txt, "%s", "Disable V3");
    }
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
    if (value == 0) {
      sprintf(txt, "%s", "Triangle");
    } else if (value == 1) {
      sprintf(txt, "%s", "Saw");
    } else if (value == 2) {
      sprintf(txt, "%s", "Square");
    } else if (value == 3) {
      sprintf(txt, "%s", "Noise");
    }
    break;
  case 10:
  case 11:
  case 12:
    if (value == 0) {
      sprintf(txt, "%s", "On");
    } else if (value == 1) {
      sprintf(txt, "%s", "Off");
    }
    break;
  case 13:
  case 14:
  case 16:
    if (value == 0) {
      sprintf(txt, "%s", "2 ms");
    } else if (value == 1) {
      sprintf(txt, "%s", "8 ms");
    } else if (value == 2) {
      sprintf(txt, "%s", "16 ms");
    } else if (value == 3) {
      sprintf(txt, "%s", "24 ms");
    } else if (value == 4) {
      sprintf(txt, "%s", "38 ms");
    } else if (value == 5) {
      sprintf(txt, "%s", "56 ms");
    } else if (value == 6) {
      sprintf(txt, "%s", "68 ms");
    } else if (value == 7) {
      sprintf(txt, "%s", "80 ms");
    } else if (value == 8) {
      sprintf(txt, "%s", "100 ms");
    } else if (value == 9) {
      sprintf(txt, "%s", "240 ms");
    } else if (value == 10) {
      sprintf(txt, "%s", "500 ms");
    } else if (value == 11) {
      sprintf(txt, "%s", "800 ms");
    } else if (value == 12) {
      sprintf(txt, "%s", "1 s");
    } else if (value == 13) {
      sprintf(txt, "%s", "3 s"); 
    } else if (value == 14) {
      sprintf(txt, "%s", "5 s");
    } else if (value == 15) {
      sprintf(txt, "%s", "8 s");
    }
    break;
  case 15:
    sprintf(txt, "%d", value);
    break;
  default:
    return 0;
  }
  return txt;
}
