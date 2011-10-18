#include <cstdio>
#include <cmath>

#include "Vocoder.hpp"

Vocoder::Vocoder() {
  global_values = &gval;
  track_values = 0;
  attributes = 0;
}

void Vocoder::init(zzub::archive *pi) {
  suspend();
}

void Vocoder::suspend() ///clear any buffers...
{
  int i, j;
  for (i = 0; i < nbnd; i++) { 
    for (j = 3; j < 12; j++) {
      f[i][j] = 0.0f; //zero band filters and envelopes
    }
  }
  kout = 0.0f;
  kval = 0;
}


void Vocoder::resume() {
  double tpofs = 6.2831853 / _master_info->samples_per_second;
  double rr, th;
  float sh;
  int i;
  swap = 1; 
  if (param[0] > 0.5f) {
    swap = 0;
  }
  gain = (float)pow(10.0f, 2.0f * param[1] - 3.0f * param[5] - 2.0f);
  thru = (float)pow(10.0f, 0.5f + 2.0f * param[1]);
  high =  param[3] * param[3] * param[3] * thru;
  thru *= param[2] * param[2] * param[2];
  if (param[7] < 0.5f) {
    nbnd = 8;
    f[1][2] = 3000.0f;
    f[2][2] = 2200.0f;
    f[3][2] = 1500.0f;
    f[4][2] = 1080.0f;
    f[5][2] = 700.0f;
    f[6][2] = 390.0f;
    f[7][2] = 190.0f;
  } else {
    nbnd = 16;
    f[1][2] = 5000.0f; //+1000
    f[2][2] = 4000.0f; //+750
    f[3][2] = 3250.0f; //+500
    f[4][2] = 2750.0f; //+450
    f[5][2] = 2300.0f; //+300
    f[6][2] = 2000.0f; //+250
    f[7][2] = 1750.0f; //+250
    f[8][2] = 1500.0f; //+250
    f[9][2] = 1250.0f; //+250
    f[10][2] = 1000.0f; //+250
    f[11][2] =  750.0f; //+210
    f[12][2] =  540.0f; //+190
    f[13][2] =  350.0f; //+155
    f[14][2] =  195.0f; //+100
    f[15][2] =   95.0f;
  }

  if (param[4] < 0.05f) {
    //freeze
    for (i = 0; i < nbnd; i++) {
      f[i][12] = 0.0f;
    }
  } else {
    f[0][12] = (float)pow(10.0, -1.7 - 2.7f * param[4]); //envelope speed
    rr = 0.022f / (float)nbnd; //minimum proportional to frequency to stop distortion
    for (i = 1; i < nbnd; i++) {                   
      f[i][12] = (float)(0.025 - rr * (double)i);
      if (f[0][12] < f[i][12]) {
	f[i][12] = f[0][12];
      }
    }
    f[0][12] = 0.5f * f[0][12]; //only top band is at full rate
  }

  rr = 1.0 - pow(10.0f, -1.0f - 1.2f * param[5]);
  sh = (float)pow(2.0f, 3.0f * param[6] - 1.0f); //filter bank range shift 

  for (i = 1; i < nbnd; i++) {
    f[i][2] *= sh;
    th = acos((2.0 * rr * cos(tpofs * f[i][2])) / (1.0 + rr * rr));
    f[i][0] = (float)(2.0 * rr * cos(th)); //a0
    f[i][1] = (float)(-rr * rr);           //a1
                //was .98
    f[i][2] *= 0.96f; //shift 2nd stage slightly to stop high resonance peaks
    th = acos((2.0 * rr * cos(tpofs * f[i][2])) / (1.0 + rr * rr));
    f[i][2] = (float)(2.0 * rr * cos(th));
  }
}
	
void Vocoder::process_events() {
  if (gval.input_select != 65535) {
    param[0] = gval.input_select * 0.001f;
  }
  if (gval.output_db != 65535) {
    param[1] = gval.output_db * 0.001f;
  }
  if (gval.hi_thru != 65535) {
    param[2] = gval.hi_thru * 0.001f;
  }
  if (gval.hi_band != 65535) {
    param[3] = gval.hi_band * 0.001f;
  }
  if (gval.envelope != 65535) {
    param[4] = gval.envelope * 0.001f;
  }
  if (gval.filter_q != 65535) {
    param[5] = gval.filter_q * 0.001f;
  }
  if (gval.freq_range != 65535) {
    param[6] = gval.freq_range * 0.001f;
  }
  if (gval.num_bands != 65535) {
    param[7] = gval.num_bands * 0.001f;
  }
  resume();
}

bool Vocoder::process_stereo(float **pin, float **pout, int sampleFrames, int mode) {
  float *in1 = pin[0];
  float *in2 = pin[1];
  float *out1 = pout[0];
  float *out2 = pout[1];
  float a, b, o = 0.0f, aa, bb, oo = kout, g = gain, ht = thru, hh = high, tmp;
  int i, k = kval, sw = swap, nb = nbnd;
  --in1;
  --in2;
  --out1;
  --out2;
  while (--sampleFrames >= 0) {
    a = *++in1; //speech  
    b = *++in2; //synth
    if (sw == 0) { 
      tmp = a; a = b; b = tmp; 
    } //swap channels

    tmp = a - f[0][7]; //integrate modulator for HF band and filter bank pre-emphasis
    f[0][7] = a;
    a = tmp;
 
    if (tmp < 0.0f) { 
      tmp = -tmp;
    }
    f[0][11] -= f[0][12] * (f[0][11] - tmp);      //high band envelope
    o = f[0][11] * (ht * a + hh * (b - f[0][3])); //high band + high thru
    
    f[0][3] = b; //integrate carrier for HF band

    if (++k & 0x1) {
      //this block runs at half sample rate
      oo = 0.0f;
      aa = a + f[0][9] - f[0][8] - f[0][8];  //apply zeros here instead of in each reson
      f[0][9] = f[0][8];  f[0][8] = a;
      bb = b + f[0][5] - f[0][4] - f[0][4]; 
      f[0][5] = f[0][4];  f[0][4] = b; 

      for (i = 1; i < nb; i++) {
	//filter bank: 4th-order band pass
        tmp = f[i][0] * f[i][3] + f[i][1] * f[i][4] + bb;
        f[i][4] = f[i][3];
        f[i][3] = tmp;
        tmp += f[i][2] * f[i][5] + f[i][1] * f[i][6];
        f[i][6] = f[i][5];
        f[i][5] = tmp;
	
        tmp = f[i][0] * f[i][7] + f[i][1] * f[i][8] + aa;
        f[i][8] = f[i][7];
        f[i][7] = tmp;
        tmp += f[i][2] * f[i][9] + f[i][1] * f[i][10];
        f[i][10] = f[i][9];
        f[i][9] = tmp;
        
        if (tmp < 0.0f) {
	  tmp = -tmp;
	}
        f[i][11] -= f[i][12] * (f[i][11] - tmp);
        oo += f[i][5] * f[i][11];
      }
    }
    o += oo * g; //effect of interpolating back up to Fs would be minimal (aliasing >16kHz)

    *++out1 = o;
    *++out2 = o;
  }

  kout = oo;  
  kval = k & 0x1;
  if (fabs(f[0][11]) < 1.0e-10) {
    f[0][11] = 0.0f; //catch HF envelope denormal
  }

  for (i = 1;i < nb; i++) {
    if (fabs(f[i][3]) < 1.0e-10 || fabs(f[i][7]) < 1.0e-10) {
      for (k = 3; k < 12; k++) {
	f[i][k] = 0.0f; //catch reson & envelope denormals
      }
    }
  }

  if (fabs(o) > 10.0f) {
    suspend(); //catch instability
  }
  return true;
}

const char *Vocoder::describe_value(int index, int value) {
  static char txt[20];
  switch (index) {
  case 0:
    if (swap) {
      sprintf(txt, "RIGHT");
    } else {
      sprintf(txt, "LEFT");
    }
    break;
  case 1:
    sprintf(txt, "%.1f dB", 40.0f * param[index] - 20.0f);
    break;
  case 4:
    if (param[index] < 0.05f) {
      sprintf(txt, "FREEZE");
    } else {
      sprintf(txt, "%.1f ms", (float)pow(10.0f, 1.0f + 3.0f * param[index]));
    }
    break;
  case 6:
    sprintf(txt, "%.0f Hz", 800.0f * (float)pow(2.0f, 3.0f * param[index] - 2.0f));
    break;
  case 7:
    if (nbnd == 8) {
      sprintf(txt, "8 BAND");
    } else { 
      sprintf(txt, "16 BAND");
    }
    break;
  default:
    sprintf(txt, "%.0f%%", 100.0f * param[index]);
    break;
  }
  return txt;
}
