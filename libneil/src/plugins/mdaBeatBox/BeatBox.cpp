#include <cstdio>
#include <cmath>
#include <cstring>
#include <cstdlib>

#include "BeatBox.hpp"

BeatBox::BeatBox() {
  global_values = &gval;
  track_values = 0;
  attributes = 0;
}

void BeatBox::init(zzub::archive *pi) {
  fParam1 = 0.30f; //hat thresh
  fParam2 = 0.45f; //hat rate
  fParam3 = 0.50f; //hat mix
  fParam4 = 0.46f; //kick thresh 	
  fParam5 = 0.15f; //kick key
  fParam6 = 0.50f; //kick mix
  fParam7 = 0.50f; //snare thresh
  fParam8 = 0.70f; //snare key
  fParam9 = 0.50f; //snare mix
  fParam10 = 0.00f; //dynamics
  fParam11 = 0.00f; //record 
  fParam12 = 0.00f; //thru mix
  hbuflen = 20000;
  kbuflen = 20000;
  sbuflen = 60000;
  if (_master_info->samples_per_second > 49000) { 
    hbuflen *= 2; 
    kbuflen *= 2; 
    sbuflen *= 2; 
  }

  hbuf = new float[hbuflen];
  sbuf = new float[sbuflen]; 
  sbuf2 = new float[sbuflen];
  kbuf = new float[kbuflen];

  synth();

  //calcs here
  hthr = (float)pow(10.f, 2.f * fParam1 - 2.f);
  hdel = (int)((0.04 + 0.20 * fParam2) * _master_info->samples_per_second);
  sthr = (float)(40.0 * pow(10.f, 2.f * fParam7 - 2.f));
  sdel = (int)(0.12 * _master_info->samples_per_second);
  kthr = (float)(220.0 * pow(10.f, 2.f * fParam4 - 2.f));
  kdel = (int)(0.10 * _master_info->samples_per_second);

  hlev = (float)(0.0001f + fParam3 * fParam3 * 4.f);
  klev = (float)(0.0001f + fParam6 * fParam6 * 4.f);
  slev = (float)(0.0001f + fParam9 * fParam9 * 4.f);
 
  kww = (float)pow(10.0, -3.0 + 2.2 * fParam5);
  ksf1 = (float)cos(3.1415927 * kww); //p
  ksf2 = (float)sin(3.1415927 * kww); //q

  ww = (float)pow(10.0, -3.0 + 2.2 * fParam8);
  sf1 = (float)cos(3.1415927 * ww); //p
  sf2 = (float)sin(3.1415927 * ww); //q
  sf3 = 0.991f; //r
  sfx = 0; 
  ksfx = 0;
  rec = 0; 
  recx = 0; 
  recpos = 0;

  mix = fParam12;
  dyna = (float)pow(10.0, -1000.0 / _master_info->samples_per_second);
  dynr = (float)pow(10.0, -6.0 / _master_info->samples_per_second);
  dyne = 0.f; 
  dynm = fParam10;
}

void BeatBox::destroy() {
  if (hbuf) {
    delete[] hbuf;
  }
  if (kbuf) {
    delete[] kbuf;
  }
  if (sbuf) {
    delete[] sbuf;
  }
  if (sbuf2) { 
    delete[] sbuf2;
  }
}

void BeatBox::synth()
{
  int t; 
  float e = 0.00012f, de, o, o1 = 0.f, o2 = 0.f, p = 0.2f, dp; 

  memset(hbuf, 0, hbuflen * sizeof(float)); //generate hi-hat
  de = (float)pow(10.0, -36.0 / _master_info->samples_per_second);
  for (t = 0; t < 5000; t++) { 
    o = (float)((rand() % 2000) - 1000); 
    *(hbuf + t) =  e * ( 2.f * o1 - o2 - o); 
    e *= de; 
    o2 = o1; 
    o1 = o;
  }

  memset(kbuf, 0, kbuflen * sizeof(float)); //generate kick
  de = (float)pow(10.0, -3.8 / _master_info->samples_per_second);
  e = 0.5f; 
  dp = 1588.f / _master_info->samples_per_second;
  for (t = 0; t < 14000; t++) { 
    *(kbuf + t) = e * (float)sin(p); 
    e *= de; 
    p = (float)fmod(p + dp * e, 6.2831853f);
  }

  memset(sbuf, 0, sbuflen * sizeof(float)); //generate snare
  de = (float)pow(10.0, -15.0 / _master_info->samples_per_second);
  e = 0.38f; 
  dp = 1103.f / _master_info->samples_per_second;
  for (t = 0; t < 7000; t++) { 
    o = (0.3f * o) + (float)((rand() % 2000) - 1000); 
    *(sbuf + t) = (float)(e * (sin(p) + 0.0004 * o)); 
    *(sbuf2 + t) = *(sbuf + t);
    e *= de; 
    p = (float)fmod(p + 0.025, 6.2831853);
  }
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

bool BeatBox::process_stereo(float **pin, float **pout, int sampleFrames, int mode) {
  // clean pout
  for (int i = 0; i < sampleFrames; i++) {
    pout[0][i] = 0.0;
    pout[1][i] = 0.0;
  }
  float *in1 = pin[0];
  float *in2 = pin[1];
  float *out1 = pout[0];
  float *out2 = pout[1];
  float a, b, e, o, hf = hfil, ht = hthr, mx3 = 0.f, mx1 = mix;
  int hp = hbufpos, hl = hbuflen - 2, hd = hdel;
  float kt = kthr;
  int kp = kbufpos, kl = kbuflen - 2, kd = kdel;  
  float st = sthr, s, f1 = sb1, f2 = sb2, b1 = sf1, b2 = sf2, b3 = sf3;
  float k, kf1 = ksb1, kf2 = ksb2, kb1 = ksf1, kb2 = ksf2;
  float hlv = hlev, klv = klev, slv = slev; 
  int sp = sbufpos, sl = sbuflen - 2, sd = sdel;  
  float ya = dyna, yr = dynr, ye = dyne, ym = dynm, mx4;

  if (sfx > 0) { 
    mx3 = 0.08f;
    slv = 0.f;
    klv = 0.f; 
    hlv = 0.f;
    mx1 = 0.f; 
    sfx -= sampleFrames;
  } //key listen (snare)

  if (ksfx > 0) {
    mx3 = 0.03f; 
    slv = 0.f; 
    klv = 0.f; 
    hlv = 0.f;
    mx1 = 0.f; 
    ksfx -= sampleFrames;
    b1 = ksf1; 
    b2 = ksf2; } //key listen (kick)

  --in1;	
  --in2;	
  --out1;
  --out2;

  if (rec == 0) {
    while (--sampleFrames >= 0) {
      a = *++in1;
      b = *++in2;
      e = a + b;
    
      ye = (e < ye) ? ye * yr : e - ya * (e - ye); //dynamics envelope

      hf = e - hf; //high filter
      if ((hp > hd) && (hf > ht)) {
	hp = 0;
      } else { 
	hp++; 
	if (hp > hl) {
	  hp = hl; 
	}
      }
      o = hlv * *(hbuf + hp); //hat
  
      k = e + (kf1 * kb1) - (kf2 * kb2); //low filter
      kf2 = b3 * ((kf1 * kb2) + (kf2 * kb1)); 
      kf1 = b3 * k;
      if ((kp > kd) && (k > kt)) {
	kp = 0;
      } else { 
	kp++; 
	if (kp > kl) {
	  kp=kl;
	} 
      }
      o += klv * *(kbuf + kp); //kick
  
      s = hf + (0.3f * e) + (f1 * b1) - (f2 * b2); //mid filter
      f2 = b3 * ((f1 * b2) + (f2 * b1)); 
      f1 = b3 * s;

      if ((sp > sd) && (s > st)) { 
	sp = 0; 
      } else { 
	sp++; 
	if (sp > sl) {
	  sp = sl; 
	}
      }
      
      mx4 = 1.f + ym * (ye + ye - 1.f); //dynamics

      *++out1 = mx1 * a + mx3 * s + mx4 * (o + slv * *(sbuf  + sp));	
      *++out2 = mx1 * a + mx3 * s + mx4 * (o + slv * *(sbuf2 + sp));	

      hf=e;
    }
  } else {
    //record
    while(--sampleFrames >= 0) {
      a = *++in1;
      b = *++in2;
      e = 0.5f * (a + b);

      if ((recpos == 0) && (fabs(e) < 0.004)) {
	e = 0.f;
      } else { 
       	switch(rec) {
	case 1: 
	  break; //echo
	case 2: 
	  if (recpos < hl) { 
	    *(hbuf + recpos++) = e; 
	  } else { 
	    e = 0.f; 
	  } 
	  break;
	case 3: 
	  if (recpos < kl) { 
	    *(kbuf + recpos++) = e; 
	  } else {
	    e = 0.f;
	  } 
	  break;
	case 4: 
	  if (recpos < sl) { 
	    *(sbuf + recpos) = a; 
	    *(sbuf2 + recpos) = b; 
	    recpos++; 
	  } else { 
	    e = 0.f; 
	  }
	  break;
        }
      }
      *++out1 = e;	
      *++out2 = e;	
    }
  }
  hfil = hf; hbufpos = hp; 
  sbufpos = sp; sb1 = f1; sb2 = f2;
  kbufpos = kp; ksb1 = kf1; ksb2 = kf2;
  dyne = ye;
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
    if (fParam12 == 0.0) {
      sprintf(txt, "-inf");
    } else {
      sprintf(txt, "%d dB", int(20.f * log10(fParam12)));      
    }
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
