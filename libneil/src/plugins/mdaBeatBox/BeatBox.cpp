#include <cstdio>
#include <cmath>

#include "BeatBox.hpp"

BeatBox::BeatBox() {
  global_values = &gval;
  track_values = 0;
  attributes = 0;
}

void BeatBox::init(zzub::archive *pi) {

}
	
void BeatBox::process_events() {
  if (gval.hat_thresh != 65535) {
    fParam1 = gval.hat_thresh * 0.001f;
  }
  if (gval.hat_rate != 65535) {
    fParam2 = gval.hat_rate * 0.001f;
  }
  if (gval.hat_mix != 65535) {
    fParam3 = gval.hat_mix * 0.001f;
  }
  if (gval.kick_thresh != 65535) {
    fParam4 = gval.kick_thresh * 0.001f;
  }
  if (gval.kick_key != 65535) {
    fParam5 = gval.kick_key * 0.001f;
  }
  if (gval.kick_mix != 65535) {
    fParam6 = gval.kick_mix * 0.001f;
  }
  if (gval.snare_thresh != 65535) {
    fParam7 = gval.snare_thresh * 0.001f;
  }
  if (gval.snare_key != 65535) {
    fParam8 = gval.snare_key * 0.001f;
  }
  if (gval.snare_mix != 65535) {
    fParam9 = gval.snare_mix * 0.001f;
  }
  if (gval.dynamics != 65535) {
    fParam10 = gval.dynamics * 0.001f;
  }
  if (gval.record != 65535) {
    fParam11 = gval.record * 0.001f;
  }
  if (gval.thru_mix != 65535) {
    fParam12 = gval.thru_mix * 0.001f;
  }
  //calcs here
  hthr = (float)pow(10.f, 2.f * fParam1 - 2.f);
  hdel = (int)((0.04 + 0.20 * fParam2) * _master_info->samples_per_second);
  sthr = (float)(40.0 * pow(10.f, 2.f * fParam7 - 2.f));
  kthr = (float)(220.0 * pow(10.f, 2.f * fParam4 - 2.f));

  hlev = (float)(0.0001f + fParam3 * fParam3 * 4.f);
  klev = (float)(0.0001f + fParam6 * fParam6 * 4.f);
  slev = (float)(0.0001f + fParam9 * fParam9 * 4.f);
 
  wwx = ww;
  ww = (float)pow(10.0, -3.0 + 2.2 * fParam8);
  sf1 = (float)cos(3.1415927 * ww); //p
  sf2 = (float)sin(3.1415927 * ww); //q
  //sfx = 0; ksfx = 0;

  kwwx = kww;
  kww = (float)pow(10.0, -3.0 + 2.2 * fParam5);
  ksf1 = (float)cos(3.1415927 * kww); //p
  ksf2 = (float)sin(3.1415927 * kww); //q

  if (wwx != ww) {
    sfx = (int)(2 * _master_info->samples_per_second); 
  }
  if (kwwx != kww) {
    ksfx = (int)(2 * _master_info->samples_per_second);
  }
  
  rec = (int)(4.9 * fParam11); 
  if ((rec != recx) && (recpos > 0)) {
    //finish sample
    switch(rec) {
    case 2:
      while (recpos < hbuflen) {
	*(hbuf + recpos++) = 0.f; 
      }
      break;
    case 3: 
      while (recpos < kbuflen) {
	*(kbuf + recpos++) = 0.f; 
	break;
      }
    case 4: 
      while (recpos < sbuflen) { 
	*(sbuf  + recpos) = 0.f; 
	*(sbuf2 + recpos) = 0.f; 
	recpos++; 
      } 
      break;
    }
  }
  recpos = 0; recx = rec;
  mix = fParam12;
  dynm = fParam10;
}

bool BeatBox::process_stereo(float **pin, float **pout, int n, int mode) {
  return true;
}

const char *BeatBox::describe_value(int param, int value) {
  static char txt[20];
  switch (param) {
  case 0:
    sprintf(txt, "%.2f dB", 40.0 * fParam1 - 40.0);
    break;
  case 1:
    sprintf(txt, "%d ms", int(1000.f * hdel / _master_info->samples_per_second));
    break;
  case 2:
    sprintf(txt, "%d dB", int(20.f * log10(hlev)));
    break;
  case 3:
    sprintf(txt, "%.2f dB", 40.0 * fParam4 - 40.0);
    break;
  case 4:
    sprintf(txt, "%d Hz", int(0.5 * kww * _master_info->samples_per_second));
    break;
  case 5:
    sprintf(txt, "%d dB", int(20.f * log10(klev)));
    break;
  case 6:
    sprintf(txt, "%.2f dB", 40.0 * fParam7 - 40.0);
    break;
  case 7:
    sprintf(txt, "%d Hz", int(0.5 * ww * _master_info->samples_per_second));
    break;
  case 8:
    sprintf(txt, "%d dB", int(20.f * log10(slev)));
    break;
  case 9:
    sprintf(txt, "%d %%", int(100.f * fParam10));
    break;
  case 11:
    sprintf(txt, "%d dB", int(20.f * log10(fParam12)));
    break;
  case 10:
    switch (rec) {
    case 0:
      sprintf(txt, "-");
      break;
    case 1:
      sprintf(txt, "MONITOR");
      break;
    case 2:
      sprintf(txt, "-> HAT");
      break;
    case 3:
      sprintf(txt, "-> KIK");
      break;
    case 4:
      sprintf(txt, "-> SNR");
      break;
    }
    break;
  default:
    return 0;
  }
  return txt;
}
