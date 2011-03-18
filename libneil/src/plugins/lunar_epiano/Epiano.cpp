#include <cstdio>
#include <cmath>

#include "Epiano.hpp"

LunarEpiano::LunarEpiano() 
{
  global_values = &gval;
  track_values = &tval;
  attributes = 0;
}

LunarEpiano::~LunarEpiano() 
{

}

void LunarEpiano::init(zzub::archive* pi) 
{
  waves = epianoData;
  //Waveform data and keymapping
  kgrp[ 0].root = 36; kgrp[ 0].high = 39; //C1
  kgrp[ 3].root = 43; kgrp[ 3].high = 45; //G1
  kgrp[ 6].root = 48; kgrp[ 6].high = 51; //C2
  kgrp[ 9].root = 55; kgrp[ 9].high = 57; //G2
  kgrp[12].root = 60; kgrp[12].high = 63; //C3
  kgrp[15].root = 67; kgrp[15].high = 69; //G3
  kgrp[18].root = 72; kgrp[18].high = 75; //C4
  kgrp[21].root = 79; kgrp[21].high = 81; //G4
  kgrp[24].root = 84; kgrp[24].high = 87; //C5
  kgrp[27].root = 91; kgrp[27].high = 93; //G5
  kgrp[30].root = 96; kgrp[30].high = 999; //C6
		
  kgrp[0].pos = 0; kgrp[0].end = 8476; kgrp[0].loop = 4400;  
  kgrp[1].pos = 8477; kgrp[1].end = 16248; kgrp[1].loop = 4903;  
  kgrp[2].pos = 16249; kgrp[2].end = 34565; kgrp[2].loop = 6398;  
  kgrp[3].pos = 34566; kgrp[3].end = 41384; kgrp[3].loop = 3938;  
  kgrp[4].pos = 41385; kgrp[4].end = 45760; kgrp[4].loop = 1633; //was 1636;  
  kgrp[5].pos = 45761; kgrp[5].end = 65211; kgrp[5].loop = 5245;  
  kgrp[6].pos = 65212; kgrp[6].end = 72897; kgrp[6].loop = 2937;  
  kgrp[7].pos = 72898; kgrp[7].end = 78626; kgrp[7].loop = 2203; //was 2204;  
  kgrp[8].pos = 78627; kgrp[8].end = 100387; kgrp[8].loop = 6368;  
  kgrp[9].pos = 100388; kgrp[9].end = 116297; kgrp[9].loop = 10452;  
  kgrp[10].pos = 116298; kgrp[10].end = 127661; kgrp[10].loop = 5217; //was 5220; 
  kgrp[11].pos = 127662; kgrp[11].end = 144113; kgrp[11].loop = 3099;  
  kgrp[12].pos = 144114; kgrp[12].end = 152863; kgrp[12].loop = 4284;  
  kgrp[13].pos = 152864; kgrp[13].end = 173107; kgrp[13].loop = 3916;  
  kgrp[14].pos = 173108; kgrp[14].end = 192734; kgrp[14].loop = 2937;  
  kgrp[15].pos = 192735; kgrp[15].end = 204598; kgrp[15].loop = 4732;  
  kgrp[16].pos = 204599; kgrp[16].end = 218995; kgrp[16].loop = 4733;  
  kgrp[17].pos = 218996; kgrp[17].end = 233801; kgrp[17].loop = 2285;  
  kgrp[18].pos = 233802; kgrp[18].end = 248011; kgrp[18].loop = 4098;  
  kgrp[19].pos = 248012; kgrp[19].end = 265287; kgrp[19].loop = 4099;  
  kgrp[20].pos = 265288; kgrp[20].end = 282255; kgrp[20].loop = 3609;  
  kgrp[21].pos = 282256; kgrp[21].end = 293776; kgrp[21].loop = 2446;  
  kgrp[22].pos = 293777; kgrp[22].end = 312566; kgrp[22].loop = 6278;  
  kgrp[23].pos = 312567; kgrp[23].end = 330200; kgrp[23].loop = 2283;  
  kgrp[24].pos = 330201; kgrp[24].end = 348889; kgrp[24].loop = 2689;  
  kgrp[25].pos = 348890; kgrp[25].end = 365675; kgrp[25].loop = 4370;  
  kgrp[26].pos = 365676; kgrp[26].end = 383661; kgrp[26].loop = 5225;  
  kgrp[27].pos = 383662; kgrp[27].end = 393372; kgrp[27].loop = 2811;  
  kgrp[28].pos = 383662; kgrp[28].end = 393372; kgrp[28].loop = 2811; //ghost
  kgrp[29].pos = 393373; kgrp[29].end = 406045; kgrp[29].loop = 4522;  
  kgrp[30].pos = 406046; kgrp[30].end = 414486; kgrp[30].loop = 2306;  
  kgrp[31].pos = 406046; kgrp[31].end = 414486; kgrp[31].loop = 2306; //ghost
  kgrp[32].pos = 414487; kgrp[32].end = 422408; kgrp[32].loop = 2169;  
		
  //extra xfade looping...
  for (long k = 0; k < 28; k++) {
    long p0 = kgrp[k].end;
    long p1 = kgrp[k].end - kgrp[k].loop;
    float xf = 1.0f;
    float dxf = -0.02f;
    while (xf > 0.0f) {
      waves[p0] = (short)((1.0f - xf) * (float)waves[p0] + xf * (float)waves[p1]);
      p0--;
      p1--;
      xf += dxf;
    }
  }
		
  //initialise...
  for (long v = 0; v < NVOICES; v++) {
    voice[v].note = 0;
    voice[v].env = 0.0f;
    voice[v].dec = 0.99f; //all notes off
  }
  notes[0] = EVENTS_DONE;
  volume = 0.2f;
  muff = 160.0f;
  sustain = 0;
  tl = tr = lfo0 = dlfo = 0.0f;
  lfo1 = 1.0f;
      
  sizevel = 0;	// this is uninitialized in the original epiano (?)
  //update();
	
  Fs = _master_info->samples_per_second;
  iFs = 1.0f / Fs;
  dlfo = 6.283f * iFs * (float)exp(6.22f * param[5] - 2.61f); //lfo rate 
}

void LunarEpiano::destroy() 
{

}

void LunarEpiano::stop() {

}

void LunarEpiano::set_track_count(int tracks) 
{

}

void LunarEpiano::process_events() 
{
 
}

bool LunarEpiano::process_stereo(float **pin, float **pout, int n, int mode) 
{
  return true;
}

const char *LunarEpiano::describe_value(int param, int value) 
{
  return 0;
}



