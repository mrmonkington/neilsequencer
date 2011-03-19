#include <algorithm>
#include <cstdio>

#include "Bandisto.hpp"

LunarBandisto::LunarBandisto() {
  global_values = &gval;
  attributes = 0;
  track_values = 0;
}

void LunarBandisto::init(zzub::archive *pi) {
  //inits here!
  fParam1 = (float)1.00; //Listen: L/M/H/out
  fParam2 = (float)0.40; //xover1
  fParam3 = (float)0.50; //xover2
  fParam4 = (float)0.50; //L drive    (1)
  fParam5 = (float)0.50; //M drive
  fParam6 = (float)0.50; //H drive
  fParam7 = (float)0.50; //L trim     (2)
  fParam8 = (float)0.50; //M trim
  fParam9 = (float)0.50; //H trim
  fParam10 = (float)0.4; //transistor/valve

  //calcs here!
  driv1 = (float)pow(10.0,(6.0 * fParam4 * fParam4) - 1.0);
  driv2 = (float)pow(10.0,(6.0 * fParam5 * fParam5) - 1.0);
  driv3 = (float)pow(10.0,(6.0 * fParam6 * fParam6) - 1.0);

  valve = int(1.99 * fParam10);
  if (valve) {
    trim1 = (float)(0.5); 
    trim2 = (float)(0.5); 
    trim3 = (float)(0.5); 
  } else {
    trim1 = 0.3f * (float)pow(10.0, (4.0 * pow(fParam4, 3.f)));
    trim2 = 0.3f * (float)pow(10.0, (4.0 * pow(fParam5, 3.f)));
    trim3 = 0.3f * (float)pow(10.0, (4.0 * pow(fParam6, 3.f)));
  }
  trim1 = (float)(trim1 * pow(10.0, 2.0 * fParam7 - 1.0));
  trim2 = (float)(trim2 * pow(10.0, 2.0 * fParam8 - 1.0));
  trim3 = (float)(trim3 * pow(10.0, 2.0 * fParam9 - 1.0));

  switch (int(fParam1 * 5.0)) {
    case 0: 
      trim2 = 0.0; 
      trim3 = 0.0; 
      slev = 0.0; 
      break;
    case 1: 
      trim1 = 0.0; 
      trim3 = 0.0; 
      slev = 0.0; 
      break;
    case 2: 
      trim1 = 0.0; 
      trim2 = 0.0; 
      slev = 0.0; 
      break;
    default:
      slev = 0.5; 
      break;
  }
  fi1 = (float)pow(10.0, fParam2 - 1.70); 
  fo1 = (float)(1.0 - fi1);
  fi2 = (float)pow(10.0, fParam3 - 1.05); 
  fo2 = (float)(1.0 - fi2);

}
	
void LunarBandisto::process_events() {
  if (gval.listen != 0xff) {
    fParam1 = gval.listen * 0.01f;
  }
  if (gval.xover1 != 0xff) {
    fParam2 = gval.xover1 * 0.01f;
  }
  if (gval.xover2 != 0xff) {
    fParam3 = gval.xover2 * 0.01f;
  }
  if (gval.ldrive != 0xff) {
    fParam4 = gval.ldrive * 0.01f;
  }
  if (gval.mdrive != 0xff) {
    fParam5 = gval.mdrive * 0.01f;
  }
  if (gval.hdrive != 0xff) {
    fParam6 = gval.hdrive * 0.01f;
  }
  if (gval.ltrim != 0xff) {
    fParam7 = gval.ltrim * 0.01f;
  }
  if (gval.mtrim != 0xff) {
    fParam8 = gval.mtrim * 0.01f;
  }
  if (gval.htrim != 0xff) {
    fParam9 = gval.htrim * 0.01f;
  }
  if (gval.mode != 0xff) {
    fParam10 = gval.mode * 0.01f;
  }
    //calcs here
  driv1 = (float)pow(10.0,(6.0 * fParam4 * fParam4) - 1.0);
  driv2 = (float)pow(10.0,(6.0 * fParam5 * fParam5) - 1.0);
  driv3 = (float)pow(10.0,(6.0 * fParam6 * fParam6) - 1.0);

  valve = int(1.99 * fParam10);
  if(valve) {
    trim1 = (float)(0.5); 
    trim2 = (float)(0.5); 
    trim3 = (float)(0.5); 
  } else {
    trim1 = 0.3f * (float)pow(10.0,(4.0 * pow(fParam4, 3.f)));
    trim2 = 0.3f * (float)pow(10.0,(4.0 * pow(fParam5, 3.f)));
    trim3 = 0.3f * (float)pow(10.0,(4.0 * pow(fParam6, 3.f)));
  }
  trim1 = (float)(trim1 * pow(10.0, 2.0 * fParam7 - 1.0));
  trim2 = (float)(trim2 * pow(10.0, 2.0 * fParam8 - 1.0));
  trim3 = (float)(trim3 * pow(10.0, 2.0 * fParam9 - 1.0));

  switch (int(fParam1 * 5.0)) {
    case 0: 
      trim2 = 0.0; 
      trim3 = 0.0; 
      slev = 0.0; 
      break;
    case 1: 
      trim1 = 0.0; 
      trim3 = 0.0; 
      slev = 0.0; 
      break;
    case 2: 
      trim1 = 0.0; 
      trim2 = 0.0; 
      slev = 0.0; 
      break;
   default: 
     slev = 0.5; 
     break;
  }
  fi1 = (float)pow(10.0, fParam2 - 1.70); 
  fo1 = (float)(1.0 - fi1);
  fi2 = (float)pow(10.0, fParam3 - 1.05); 
  fo2 = (float)(1.0 - fi2);
}

bool LunarBandisto::process_stereo(float **pin, float **pout, int n, int mode) {
  return true;
}

const char *LunarBandisto::describe_value(int param, int value) {
  static char txt[20];
  switch (param) {
  case 0:
    switch (int(value * 0.01f * 5.0)) {
    case 0:
      sprintf(txt, "Low");
      break;
    case 1:
      sprintf(txt, "Mid");
      break;
    case 2:
      sprintf(txt, "High");
      break;
    default:
      sprintf(txt, "Output");
      break;
    }
    break;
  case 1:
    sprintf(txt, "%.2f Hz", _master_info->samples_per_second * fi1 * (0.098 + 0.09 * fi1 + 0.5 * pow(fi1, 8.2f)));
    break;
  case 2:
    sprintf(txt, "%.2f Hz", _master_info->samples_per_second * fi2 * (0.015 + 0.15 * fi2 + 0.9 * pow(fi2, 8.2f)));
    break;
  case 3:
  case 4:
  case 5:
    sprintf(txt, "%.2f dB", 60.0f * value * 0.01f);
    break;
  case 6:
  case 7:
  case 8:
    sprintf(txt, "%.2f dB", 40.0f * value * 0.01f - 20.0f);
    break;
  case 9:
    if (value * 0.01f > 0.5f) {
      sprintf(txt, "Unipolar");
    } else {
      sprintf(txt, "Bipolar");
    }
    break;
  default:
    return 0;
  }
  return txt;
}
