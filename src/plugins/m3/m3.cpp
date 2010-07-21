#include <cstdio>
#include <cmath>
#include <cstring>

#include "m3.hpp"

M3::M3() 
{
  global_values = &gval;
  track_values = &tval;
  attributes = 0;
}

M3::~M3() 
{

}

void M3::init(zzub::archive* pi) 
{

}

void M3::destroy() 
{

}

void M3::stop() {

}

void M3::set_track_count(int tracks) 
{

}

void M3::process_events() 
{

}

bool M3::process_stereo(float **pin, float **pout, int n, int mode) 
{
  return true;
}

inline float M3::LFOFreq(int v) {
  return (std::pow((v + 8.0) / (116.0 + 8.0), 4.0) - 0.000017324998565270) * 
    40.00072;
}

inline float M3::EnvTime(int v) {
  return std::pow((v + 2.0) / (127.0 + 2.0), 3.0) * 10000;
}

const char *M3::describe_value(int param, int value) 
{
  static char txt[20];
  switch(param) {
  case 1: // PW1
  case 3: // PW2
    std::sprintf(txt, "%u : %u", (int)(value * 100.0 / 127), 
		 100 - (int)(value * 100.0 / 127));
    return txt;
  case 4: // semi detune
    if (value == 0x40) 
      std::strcpy(txt, "none");
    else if (value > 0x40) 
      std::sprintf(txt, "+%i halfnotes", value - 0x40);
    else 
      std::sprintf(txt, "%i halfnotes", value - 0x40);
    return txt;
  case 5: // fine detune
    if (value == 0x40) 
      std::strcpy(txt, "none");
    else if (value > 0x40) 
      std::sprintf(txt, "+%i cents", (int)((value - 0x40) * 100.0 / 63));
    else 
      std::sprintf(txt, "%i cents", (int)((value - 0x40) * 100.0 / 63));
    return txt;
  case 6: // Sync
    if (value == 1) 
      std::strcpy(txt, "on");
    else 
      std::strcpy(txt, "off");
    return txt;
  case 7: // MixType
    switch(value) {
    case 0: 
      std::strcpy(txt, "add"); 
      break;
    case 1: 
      std::strcpy(txt, "difference"); 
      break;
    case 2: 
      std::strcpy(txt, "mul"); 
      break;
    case 3: 
      std::strcpy(txt, "highest amp"); 
      break;
    case 4: 
      std::strcpy(txt, "lowest amp"); 
      break;
    case 5: 
      std::strcpy(txt, "and"); 
      break;
    case 6: 
      std::strcpy(txt, "or"); 
      break;
    case 7: 
      std::strcpy(txt, "xor"); 
      break;
    case 8: 
      std::strcpy(txt, "random"); 
      break;
    default: 
      std::strcpy(txt, "Invalid!");
    }
    return txt;
  case 8: // Mix
    switch(value) {
    case 0: 
      std::strcpy(txt, "Osc1"); 
      break;
    case 127: 
      std::strcpy(txt, "Osc2"); 
      break;
    default: 
      std::sprintf(txt, "%u%% : %u%%", 100 - (int)(value * 100.0 / 127), 
		   (int)(value * 100.0 / 127));
    }
    return txt;
  case 11: // Pitch Env
  case 12: // Pitch Env
  case 16: // Amp Env
  case 17: // Amp Env
  case 18: // Amp Env
  case 22: // Filter Env
  case 23: // Filter Env
  case 24: // Filter Env
    std::sprintf(txt, "%.4f sec", EnvTime(value) / 1000);
    return txt;
  case 13: // PitchEnvMod
  case 25: // Filt ENvMod
    std::sprintf(txt, "%i", value - 0x40);
    return txt;
  case 19:
    switch (value) {
    case 0: 
      std::strcpy(txt, "lowpass"); 
      break;
    case 1: 
      std::strcpy(txt, "highpass"); 
      break;
    case 2: 
      std::strcpy(txt, "bandpass"); 
      break;
    case 3: 
      std::strcpy(txt, "bandreject"); 
      break;
    default: 
      std::strcpy(txt, "Invalid!");
    }
    return txt;
  case 26: // LFO1Dest
    switch( value) {
    case 0: 
      std::strcpy(txt, "none"); 
      break;
    case 1: 
      std::strcpy(txt, "osc1"); 
      break;
    case 2: 
      std::strcpy(txt, "p.width1"); 
      break;
    case 3: 
      std::strcpy(txt, "volume"); 
      break;
    case 4: 
      std::strcpy(txt, "cutoff"); 
      break;
    case 5: 
      std::strcpy(txt, "osc1+pw1"); 
      break; // 12
    case 6: 
      std::strcpy(txt, "osc1+volume"); 
      break; // 13
    case 7: 
      std::strcpy(txt, "osc1+cutoff"); 
      break; // 14
    case 8: 
      std::strcpy(txt, "pw1+volume"); 
      break; // 23
    case 9: 
      std::strcpy(txt, "pw1+cutoff"); 
      break; // 24
    case 10: 
      std::strcpy(txt, "vol+cutoff"); 
      break; // 34
    case 11: 
      std::strcpy(txt, "o1+pw1+vol"); 
      break; // 123
    case 12: 
      std::strcpy(txt, "o1+pw1+cut"); 
      break; // 124
    case 13: 
      std::strcpy(txt, "o1+vol+cut"); 
      break; // 134
    case 14: 
      std::strcpy(txt, "pw1+vol+cut"); 
      break; // 234
    case 15: 
      std::strcpy(txt, "all");
      break; // 1234
    default: 
      std::strcpy(txt, "Invalid!");
    }
    return txt;
  case 30: // LFO2Dest
    switch (value) {
    case 0: 
      std::strcpy(txt, "none"); 
      break;
    case 1: 
      std::strcpy(txt, "osc2"); 
      break;
    case 2: 
      std::strcpy(txt, "p.width2"); 
      break;
    case 3: 
      std::strcpy(txt, "mix"); 
      break;
    case 4: 
      std::strcpy(txt, "resonance"); 
      break;

    case 5: 
      std::strcpy(txt, "osc2+pw2"); 
      break; // 12
    case 6: 
      std::strcpy(txt, "osc2+mix"); 
      break; // 13
    case 7: 
      std::strcpy(txt, "osc2+res"); 
      break; // 14
    case 8: 
      std::strcpy(txt, "pw2+mix"); 
      break; // 23
    case 9: 
      std::strcpy(txt, "pw2+res"); 
      break; // 24
    case 10: 
      std::strcpy(txt, "mix+res"); 
      break; // 34

    case 11: 
      std::strcpy(txt, "o2+pw2+mix"); 
      break; // 123
    case 12: 
      std::strcpy(txt, "o2+pw2+res"); 
      break; // 124
    case 13: 
      std::strcpy(txt, "o2+mix+res"); 
      break; // 134
    case 14: 
      std::strcpy(txt, "pw2+mix+res"); 
      break; // 234
    case 15: 
      std::strcpy(txt, "all"); 
      break; // 1234
    default: 
      std::strcpy(txt, "Invalid!");
    }
    return txt;
  case 9: // SubOscWave
    if(value == 4) { 
      std::strcpy(txt, "random"); 
      return txt; 
      break; 
    }
  case 0: // OSC1Wave
  case 2: // OSC2Wave
  case 27: // LFO1Wave
  case 31: // LFO2Wave
    switch(value) {
    case 0: 
      std::strcpy(txt, "sine"); 
      break;
    case 1: 
      std::strcpy(txt, "saw"); 
      break;
    case 2: 
      std::strcpy(txt, "square"); 
      break;
    case 3: 
      std::strcpy(txt, "triangle"); 
      break;
    case 4: 
      std::strcpy(txt, "noise"); 
      break;
    case 5: 
      std::strcpy(txt, "random"); 
      break;
    default: 
      std::strcpy(txt, "Invalid!"); 
      break;
    }
    return txt;
  case 28: // LFO1Freq
  case 32: // LFO2Freq
    if (value <= 116) 
      std::sprintf(txt, "%.4f Hz", LFOFreq(value));
    else 
      std::sprintf(txt, "%u ticks", 1 << (value - 117));
    return txt;
  default: 
    return 0;
  }
}



