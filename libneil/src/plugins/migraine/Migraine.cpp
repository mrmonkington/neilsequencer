#include <cstdio>
#include <cmath>
#include <cstring>
#include <algorithm>

#include "Migraine.hpp"

float tiny = 1e-10;

Migraine::Migraine() 
{
  global_values = &gval;
  attributes = NULL;
  track_values = NULL;
}

Migraine::~Migraine() 
{

}

void Migraine::init(zzub::archive* pi) {

  int order, alg;
  param_cheb2 = param_cheb3 = param_cheb4 = param_cheb5 = 
    param_cheb6 = param_cheb7 = param_cheb8 = 0.0f;
  param_cheb2ret = param_cheb3ret = param_cheb4ret = 
    param_cheb5ret = param_cheb6ret = param_cheb7ret = param_cheb8ret = 0.0f;
  param_leg2 = param_leg3 = param_leg4 = param_leg5 = 
    param_leg6 = param_leg7 = param_leg8 = 0.0f;
  param_leg2ret = param_leg3ret = param_leg4ret = param_leg5ret = 
    param_leg6ret = param_leg7ret = param_leg8ret = 0.0f;
  param_inclip = param_outclip = 0;
  for (order = 0; order < 9; order++){
    for (alg = 0; alg < 14; alg++){
      coefs[order][alg] = 0.0f;
    }
  }
  dcspeedL = 0.0f;
  dcposL = 0.0f;
  dcspeedR = 0.0f;
  dcposR = 0.0f;
}

void Migraine::destroy() {

}

void Migraine::process_events() {
  if (gval.paraCheb2 != 0xFF) { 
    param_cheb2 = ((float)gval.paraCheb2 / 254.0f); 
    coefs[2][0] = 2.0f * param_cheb2 * param_cheb2 * param_cheb2ret;
    coefs[0][0] = param_cheb2ret;
  };
  if (gval.paraCheb3 != 0xFF) { 
    param_cheb3 = ((float)gval.paraCheb3 / 254.0f); 
    coefs[1][1] = -3.0f * param_cheb3 * param_cheb3ret;
    coefs[3][1] = 4.0f * param_cheb3 * param_cheb3 * param_cheb3 * param_cheb3ret;
  };
  if (gval.paraCheb4 != 0xFF) { 
    param_cheb4 = ((float)gval.paraCheb4 / 254.0f); 
    temp = param_cheb4 * param_cheb4;
    coefs[0][2] = param_cheb4ret;
    coefs[2][2] = -8.0f * temp * param_cheb4ret;
    coefs[4][2] = 8.0f * temp * temp * param_cheb4ret;
  };
  if (gval.paraCheb5 != 0xFF) { 
    param_cheb5 = ((float)gval.paraCheb5 / 254.0f); 
    temp = param_cheb5 * param_cheb5;
    coefs[1][3] = 5.0f * param_cheb5 * param_cheb5ret;
    coefs[3][3] = -20.0f * temp * param_cheb5 * param_cheb5ret;
    coefs[5][3] = 16.0f * temp * temp * param_cheb5 * param_cheb5ret;
  };
  if (gval.paraCheb6 != 0xFF) { 
    param_cheb6 = ((float)gval.paraCheb6 / 254.0f); 
    temp = param_cheb6 * param_cheb6;
    coefs[0][4] = -1.0f * param_cheb6ret;
    coefs[2][4] = 18.0f * temp * param_cheb6ret;
    coefs[4][4] = -48.0f * temp * temp * param_cheb6ret;
    coefs[6][4] = 32.0f * temp * temp * temp * param_cheb6ret;
  };
  if (gval.paraCheb7 != 0xFF) { 
    param_cheb7 = ((float)gval.paraCheb7 / 254.0f); 
    temp = param_cheb7 * param_cheb7;
    coefs[1][5] = -7.0f * param_cheb7 * param_cheb7ret;
    coefs[3][5] = 56.0f * param_cheb7 * temp * param_cheb7ret;
    coefs[5][5] = -112.0f * param_cheb7 * temp * temp * param_cheb7ret;
    coefs[7][5] = 64.0f * param_cheb7 * temp * temp * temp * param_cheb7ret;
  };
  if (gval.paraCheb8 != 0xFF) { 
    param_cheb8 = ((float)gval.paraCheb8 / 254.0f); 
    temp = param_cheb8 * param_cheb8;
    coefs[0][6] = param_cheb8ret;
    coefs[2][6] = -32.0f * temp * param_cheb8ret;
    coefs[4][6] = 160.0f * temp * temp * param_cheb8ret;
    coefs[6][6] = -256.0f * temp * temp * temp * param_cheb8ret;
    coefs[8][6] = 128.0f * temp * temp * temp * temp * param_cheb8ret;
  };
  if (gval.paraCheb2Ret != 0xFF) { 
    param_cheb2ret = ((float)gval.paraCheb2Ret / 254.0f); 
    coefs[2][0] = 2.0f * param_cheb2 * param_cheb2 * param_cheb2ret;
    coefs[0][0] = param_cheb2ret;
  };
  if (gval.paraCheb3Ret != 0xFF) { 
    param_cheb3ret = ((float)gval.paraCheb3Ret / 254.0f); 
    param_cheb3 = ((float)gval.paraCheb3 / 127.0f) - 1.0f; 
    coefs[1][1] = -3.0f * param_cheb3 * param_cheb3ret;
    coefs[3][1] = 4.0f * param_cheb3 * param_cheb3 * param_cheb3 * param_cheb3ret;
  };
  if (gval.paraCheb4Ret != 0xFF) { 
    param_cheb4ret = ((float)gval.paraCheb4Ret / 254.0f); 
    temp = param_cheb4 * param_cheb4;
    coefs[0][2] = param_cheb4ret;
    coefs[2][2] = -8.0f * temp * param_cheb4ret;
    coefs[4][2] = 8.0f * temp * temp * param_cheb4ret;
  };
  if (gval.paraCheb5Ret != 0xFF) { 
    param_cheb5 = ((float)gval.paraCheb5Ret / 254.0f); 
    temp = param_cheb5 * param_cheb5;
    coefs[1][3] = 5.0f * param_cheb5 * param_cheb5ret;
    coefs[3][3] = -20.0f * temp * param_cheb5 * param_cheb5ret;
    coefs[5][3] = 16.0f * temp * temp * param_cheb5 * param_cheb5ret;
  };
  if (gval.paraCheb6Ret != 0xFF) { 
    param_cheb6ret = ((float)gval.paraCheb6Ret / 254.0f); 
    temp = param_cheb6 * param_cheb6;
    coefs[0][4] = -1.0f * param_cheb6ret;
    coefs[2][4] = 18.0f * temp * param_cheb6ret;
    coefs[4][4] = -48.0f * temp * temp * param_cheb6ret;
    coefs[6][4] = 32.0f * temp * temp * temp * param_cheb6ret;
  };
  if (gval.paraCheb7Ret != 0xFF) { 
    param_cheb7ret = ((float)gval.paraCheb7Ret / 254.0f); 
    temp = param_cheb7 * param_cheb7;
    coefs[1][5] = -7.0f * param_cheb7 * param_cheb7ret;
    coefs[3][5] = 56.0f * param_cheb7 * temp * param_cheb7ret;
    coefs[5][5] = -112.0f * param_cheb7 * temp * temp * param_cheb7ret;
    coefs[7][5] = 64.0f * param_cheb7 * temp * temp * temp * param_cheb7ret;
  };
  if (gval.paraCheb8Ret != 0xFF) { 
    param_cheb8ret = ((float)gval.paraCheb8Ret / 254.0f); 
    temp = param_cheb8 * param_cheb8;
    coefs[0][6] = param_cheb8ret;
    coefs[2][6] = -32.0f * temp * param_cheb8ret;
    coefs[4][6] = 160.0f * temp * temp * param_cheb8ret;
    coefs[6][6] = -256.0f * temp * temp * temp * param_cheb8ret;
    coefs[8][6] = 128.0f * temp * temp * temp * temp * param_cheb8ret;
  };
  if (gval.paraLeg2 != 0xFF) { 
    param_leg2 = ((float)gval.paraLeg2 / 254.0f); 
    coefs[0][7] = -0.5f * param_leg2ret;
    coefs[2][7] = 1.5f * param_leg2 * param_leg2 * param_leg2ret;
  };
  if (gval.paraLeg3 != 0xFF) { 
    param_leg3 = ((float)gval.paraLeg3 / 254.0f); 
    coefs[1][8] = -3.0f * param_leg3 * param_leg3ret;
    coefs[3][8] = 2.5f * param_leg3 * param_leg3 * param_leg3 * param_leg3ret;
  };
  if (gval.paraLeg4 != 0xFF) { 
    param_leg4 = ((float)gval.paraLeg4 / 254.0f); 
    temp = param_leg4 * param_leg4;
    coefs[0][9] = 0.375f * param_leg4ret;
    coefs[2][9] = -3.75f * temp * param_leg4ret;
    coefs[4][9] = 4.375f * temp * temp * param_leg4ret;
  };
  if (gval.paraLeg5 != 0xFF) { 
    param_leg5 = ((float)gval.paraLeg5 / 254.0f); 
    temp = param_leg5 * param_leg5;
    coefs[1][10] = 1.875f * param_leg5 * param_leg5ret;
    coefs[3][10] = -8.75f * temp * param_leg5 * param_leg5ret;
    coefs[5][10] = 7.875f * temp * temp * param_leg5 * param_leg5ret;
  };
  if (gval.paraLeg6 != 0xFF) { 
    param_leg6 = ((float)gval.paraLeg6 / 254.0f); 
    temp = param_leg6 * param_leg6;
    coefs[0][11] = 0.3125f * param_leg6ret;
    coefs[2][11] = 6.5625f * temp * param_leg6ret;
    coefs[4][11] = -19.6875f * temp * temp * param_leg6ret;
    coefs[6][11] = 14.4375f * temp * temp * temp * param_leg6ret;
  };
  if (gval.paraLeg7 != 0xFF) { 
    param_leg7 = ((float)gval.paraLeg7 / 254.0f); 
    temp = param_leg7 * param_leg7;
    coefs[0][12] = 15.3125f * param_leg7ret;
    coefs[3][12] = 127.96875f * temp * param_leg7 * param_leg7ret;
    coefs[5][12] = -238.21875f * temp * temp * param_leg7 * param_leg7ret;
    coefs[7][12] = 147.46875f * temp * temp * temp * param_leg7 * param_leg7ret;
  };
  if (gval.paraLeg8 != 0xFF) { 
    param_leg8 = ((float)gval.paraLeg8 / 254.0f); 
    temp = param_leg8 * param_leg8;
    coefs[0][13] = 10.1171875f * param_leg8ret;
    coefs[2][13] = -324.84375f * temp * param_leg8ret;
    coefs[4][13] = 1786.640625f * temp * temp * param_leg8ret;
    coefs[6][13] = -2721.46875f * temp * temp * temp * param_leg8ret;
    coefs[8][13] = 1457.9296875f * temp * temp * temp * temp * param_leg8ret;
  };
  if (gval.paraLeg2Ret != 0xFF) { 
    param_leg2ret = ((float)gval.paraLeg2Ret / 254.0f); 
    coefs[0][7] = -0.5f * param_leg2ret;
    coefs[2][7] = 1.5f * param_leg2 * param_leg2 * param_leg2ret;
  };
  if (gval.paraLeg3Ret != 0xFF) { 
    param_leg3ret = ((float)gval.paraLeg3Ret / 254.0f); 
    coefs[1][8] = -3.0f * param_leg3 * param_leg3ret;
    coefs[3][8] = 2.5f * param_leg3 * param_leg3 * param_leg3 * param_leg3ret;
  };
  if (gval.paraLeg4Ret != 0xFF) { 
    param_leg4ret = ((float)gval.paraLeg4Ret / 254.0f); 
    temp = param_leg4 * param_leg4;
    coefs[0][9] = 0.375f * param_leg4ret;
    coefs[2][9] = -3.75f * temp * param_leg4ret;
    coefs[4][9] = 4.375f * temp * temp * param_leg4ret;
  };
  if (gval.paraLeg5Ret != 0xFF) { 
    param_leg5 = ((float)gval.paraLeg5Ret / 254.0f); 
    temp = param_leg5 * param_leg5;
    coefs[1][10] = 1.875f * param_leg5 * param_leg5ret;
    coefs[3][10] = -8.75f * temp * param_leg5 * param_leg5ret;
    coefs[5][10] = 7.875f * temp * temp * param_leg5 * param_leg5ret;
  };
  if (gval.paraLeg6Ret != 0xFF) { 
    param_leg6ret = ((float)gval.paraLeg6Ret / 254.0f); 
    temp = param_leg6 * param_leg6;
    coefs[0][11] = 0.3125f * param_leg6ret;
    coefs[2][11] = 6.5625f * temp * param_leg6ret;
    coefs[4][11] = -19.6875f * temp * temp * param_leg6ret;
    coefs[6][11] = 14.4375f * temp * temp * temp * param_leg6ret;
  };
  if (gval.paraLeg7Ret != 0xFF) { 
    param_leg7ret = ((float)gval.paraLeg7Ret / 254.0f); 
    temp = param_leg7 * param_leg7;
    coefs[0][12] = 15.3125f * param_leg7ret;
    coefs[3][12] = 127.96875f * temp * param_leg7 * param_leg7ret;
    coefs[5][12] = -238.21875f * temp * temp * param_leg7 * param_leg7ret;
    coefs[7][12] = 147.46875f * temp * temp * temp * param_leg7 * param_leg7ret;
  };
  if (gval.paraLeg8Ret != 0xFF) { 
    param_leg8ret = ((float)gval.paraLeg8Ret / 254.0f); 
    temp = param_leg8 * param_leg8;
    coefs[0][13] = 10.1171875f * param_leg8ret;
    coefs[2][13] = -324.84375f * temp * param_leg8ret;
    coefs[4][13] = 1786.640625f * temp * temp * param_leg8ret;
    coefs[6][13] = -2721.46875f * temp * temp * temp * param_leg8ret;
    coefs[8][13] = 1457.9296875f * temp * temp * temp * temp * param_leg8ret;
  };
  if (gval.paraDry != 0xFF) { 
    param_dry = ((float)gval.paraDry / 254.0f); 
  };
  if (gval.paraWet != 0xFF) { 
    param_wet = ((float)gval.paraWet / 254.0f); 
  };
  if (gval.paraIngain != 0xFF) { 
    param_ingain = ((float)gval.paraIngain / 127.0f); 
  };
  if (gval.paraInclip != 0xFF) { 
    param_inclip = ((int)gval.paraInclip);
  };
  if (gval.paraOutclip != 0xFF) { 
    param_outclip = ((int)gval.paraOutclip); 
  };
}

bool Migraine::process_stereo(float **pin, float **pout, 
			      int numsamples, int mode) {
  if (mode == zzub::process_mode_write)
    return false;
  if (mode == zzub::process_mode_no_io)
    return false;
  if (mode == zzub::process_mode_read)
    return true;
  float inL, inR, outL, outR, xL, xR, xL2, xR2, xL4, xR4;
  int i;
  float on;
  int madeanoise = 0;
  on = param_dry + param_cheb2ret + param_cheb3ret + param_cheb4ret + 
    param_cheb5ret + param_cheb6ret + param_cheb7ret + param_cheb8ret + 
    param_leg2ret + param_leg3ret + param_leg4ret + param_leg5ret + 
    param_leg6ret + param_leg7ret + param_leg8ret;
  if (on) {
    for (i = 0; i < numsamples * 2; i++) {
      inL = pin[0][i] * param_ingain;
      inR = pin[1][i] * param_ingain;
      xL = inL;
      xR = inR;
      if (xL > 1.0f) 
	xL = 1.0f; //hardclip input (always)
      if (xL < -1.0f) 
	xL = -1.0f;		
      if (xR > 1.0f) 
	xR = 1.0f;
      if (xR < -1.0f) 
	xR = -1.0f;
      if (param_inclip > 0) {
	xL = (xL * (1.5f - (0.5f * xL * xL)));
	xR = (xR * (1.5f - (0.5f * xR * xR)));
      }
      xL2 = (xL * xL) + tiny;
      xL4 = (xL2 * xL2) + tiny;
      xR2 = (xR * xR) + tiny;
      xR4 = (xR2 * xR2) + tiny;
      outL = 0.0f;
      outR = 0.0f;
      if (param_cheb2ret != 0) {
	if (param_cheb2 != 0) {
	  outL += ((xL2 * coefs[2][0]) + coefs[0][0]);
	  outR += ((xR2 * coefs[2][0]) + coefs[0][0]);
	}
      }
      if (param_cheb3ret != 0) {
	if (param_cheb3 != 0) {
	  outL += (xL * ((coefs[1][1]) + (xL2 * coefs[3][1])));
	  outR += (xR * ((coefs[1][1]) + (xR2 * coefs[3][1])));
	}
      }
      if (param_cheb4ret != 0) {
	if (param_cheb4 != 0) {
	  outL += (coefs[0][2] + (xL2 * coefs[2][2]) + (xL4 * coefs[4][2]));
	  outR += (coefs[0][2] + (xR2 * coefs[2][2]) + (xR4 * coefs[4][2]));
	}
      }
      if (param_cheb5ret != 0) {
	if (param_cheb5 != 0) {
	  outL += xL * ((coefs[1][3]) + (xL2 * coefs[3][3]) + 
			(xL4 * coefs[5][3]));
	  outR += xR * ((coefs[1][3]) + (xR2 * coefs[3][3]) + 
			(xR4 * coefs[5][3]));
	}
      }
      if (param_cheb6ret != 0) {
	if (param_cheb6 != 0) {
	  outL += (coefs[0][4] + (xL2 * ((coefs[2][4]) + (xL2 * coefs[4][4]) + 
					 (xL4 * coefs[6][4]))));
	  outR += (coefs[0][4] + (xR2 * ((coefs[2][4]) + (xR2 * coefs[4][4]) + 
					 (xL4 * coefs[6][4]))));
	}
      }
      if (param_cheb7ret != 0) {
	if (param_cheb7 != 0) {
	  outL += xL * ((coefs[1][5]) + (xL2 * coefs[3][5]) + 
			(xL4 * ((coefs[5][5]) + (xL2 * coefs[7][5]))));
	  outR += xR * ((coefs[1][5]) + (xR2 * coefs[3][5]) + 
			(xR4 * ((coefs[5][5]) + (xR2 * coefs[7][5]))));
	}
      }
      if (param_cheb8ret != 0) {
	if (param_cheb8 != 0) {
	  outL += (coefs[0][6] + 
		   (xL2 * ((coefs[2][6]) + (xL2 * coefs[4][6]) + 
			   (xL4 * ((coefs[6][6]) + (xL2 * coefs[8][6]))))));
	  outR += (coefs[0][6] + 
		   (xR2 * ((coefs[2][6]) + (xR2 * coefs[4][6]) + 
			   (xR4 * ((coefs[6][6]) + (xR2 * coefs[8][6]))))));
	}
      }
      if (param_leg2ret != 0) {
	if (param_leg2 != 0) {
	  outL += (coefs[0][7] + (xL2 * coefs[2][7]));
	  outR += (coefs[0][7] + (xR2 * coefs[2][7]));
	}
      }
      if (param_leg3ret != 0) {
	if (param_leg3 != 0) {
	  outL += (xL * ((coefs[1][8]) + (xL2 * coefs[3][8])));
	  outR += (xR * ((coefs[1][8]) + (xR2 * coefs[3][8])));
	}
      }
      if (param_leg4ret != 0) {
	if (param_leg4 != 0) {
	  outL += (coefs[0][9] + (xL2 * ((coefs[2][9]) + (xL2 * coefs[4][9]))));
	  outR += (coefs[0][9] + (xR2 * ((coefs[2][9]) + (xR2 * coefs[4][9]))));
	}
      }
      if (param_leg5ret != 0) {
	if (param_leg5 != 0) {
	  outL += xL * ((coefs[1][10]) + 
			(xL2 * ((coefs[3][10]) + (xL2 * coefs[5][10]))));
	  outR += xR * ((coefs[1][10]) + 
			(xR2 * ((coefs[3][10]) + (xR2 * coefs[5][10]))));
	}
      }
      if (param_leg6ret != 0) {
	if (param_leg6 != 0) {
	  outL += (coefs[0][11] + 
		   (xL2 * ((coefs[2][11]) + 
			   (xL2 * ((coefs[4][11]) + (xL2 * coefs[6][11]))))));
	  outR += (coefs[0][11] + 
		   (xR2 * ((coefs[2][11]) + 
			   (xR2 * ((coefs[4][11]) + (xR2 * coefs[6][11]))))));
	}
      }
      if (param_leg7ret != 0) {
	if (param_leg7 != 0) {
	  outL += (coefs[0][12] + 
		   (xL * ((xL2 * coefs[3][12]) + 
			  (xL4 * ((coefs[5][12]) + (xL2 * coefs[7][12]))))));
	  outR += (coefs[0][12] + 
		   (xR * ((xR2 * coefs[3][12]) + 
			  (xR4 * ((coefs[5][12]) + (xR2 * coefs[7][12]))))));
	}
      }
      if (param_leg8ret != 0) {
	if (param_leg8 != 0) {
	  outL += (coefs[0][13] + 
		   (xL2 * ((coefs[2][13]) + (xL2 * coefs[4][13]) + 
			   (xL4 * ((coefs[6][13]) + (xL2 * coefs[8][13]))))));
	  outR += (coefs[0][13] + 
		   (xR2 * ((coefs[2][13]) + (xR2 * coefs[4][13]) + 
			   (xR4 * ((coefs[6][13]) + (xR2 * coefs[8][13]))))));
	}
      }
      outL = outL * (param_wet);
      outR = outR * (param_wet);
      if (param_dry > 0) {
	outL += (xL * param_dry);                      
	outR += (xR * param_dry);
      }                      
      // hardclip output, always
      if (outL > 1.0f) 
	outL = 1.0f;
      if (outL < -1.0f) 
	outL = -1.0f;
      if (outR > 1.0f) 
	outR = 1.0f;
      if (outR < -1.0f) 
	outR = -1.0f;
      if(param_outclip > 0) { //softclipper
	outL = (outL * (1.5f - (0.5f * outL * outL)));
	outR = (outR * (1.5f - (0.5f * outR * outR)));
      }
      outL = outL * 32768.0f;
      outR = outR * 32768.0f;
      //DC filter
      dcspeedL = dcspeedL + (outL - dcposL) * 0.000004567f;
      dcposL = dcposL + dcspeedL;
      dcspeedL = dcspeedL * 0.96f;
      outL = (outL - dcposL);
      dcspeedR = dcspeedR + (outR - dcposR) * 0.000004567f;
      dcposR = dcposR + dcspeedR;
      dcspeedR = dcspeedR * 0.96f;
      outR = (outR - dcposR);
      if (outL != 0) 
	madeanoise = 1;
      if (outR != 0)
	madeanoise = 1;
      pout[0][i] = outL / 32768.0;
      pout[1][i] = outR / 32768.0;
    };
    if (madeanoise) {
      return true;
    }
    else {
      return false;
    } //because all of the samples were 0, switch the machine off
  } else {
    //i.e. if on=0; this is from the if statement right at the start
    return false;
  } //end else
}

const char *Migraine::describe_value(int param, int value) {
  static char txt[16];
  if (param == 0) {
    sprintf(txt, "%.1f%%", ((float)value / 127.0f * 100.0f));
    return txt;
  } else if (param == 1) {
    return 0;
  } else if (param < 32) {
    sprintf(txt, "%.1f%%", ((float)value / 254.0f * 100.0f));
    return txt;
  } else {
    return 0;
  }
}



