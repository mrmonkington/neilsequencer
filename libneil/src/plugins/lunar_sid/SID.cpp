#include <cstdio>
#include <cstring>
#include <cmath>

#include "sidemu.h"
#include "SID.hpp"

SIDMachine::SIDMachine() {
  global_values = &gval;
  track_values = &gval;
  attributes = 0;
}

void SIDMachine::init(zzub::archive *pi) {
    clockrate = PALCLOCKRATE;
    samplerate = _master_info->samples_per_second;
    emu.set_sampling_parameters(clockrate, SAMPLE_FAST, samplerate);
    //emu.reset();
    //emu.set_chip_model(MOS8580);
    emu.set_chip_model(MOS6581);
    // reset states
    memset(voices, 0, sizeof(voices));
    memset(regs, 0, sizeof(regs));
    resonance = 0;
    mode = 0;
    volume = 0;
    cycles = 0;
    flush_regs = false;
}

void SIDMachine::sid_write(int reg, int value) {
  //emu.clock(9);
  //printf("reg %x: %x\n", reg, value);
  emu.write(reg, value);
  cycles += 9;
}
	
void SIDMachine::process_events() {
  bool reg18_changed = false; // mode/vol
  bool reg17_changed = false;
  if (gval.chipset != zzub::switch_value_none) {
    if (gval.chipset == 0) {
      emu.set_chip_model(MOS6581);
    } else {
      emu.set_chip_model(MOS8580);
    }
  }
  if (gval.cutoff != 65535) {
    int cutoff = gval.cutoff;
    unsigned char fclo = cutoff & 0x0F;
    unsigned char fchi = cutoff >> 4;
    regs[0x15] = int(fclo / 2);
    regs[0x16] = int(fchi);
    // sid_write(0x15, fclo);
    // sid_write(0x16, fchi);
  }
  if (gval.resonance != 255) {
    resonance = gval.resonance;
    reg17_changed = true;
  }
  if (gval.mode != 255) {
    mode = 1 << gval.mode;
    reg18_changed = true;
  }
  if (gval.volume != 255) {
    volume = gval.volume;
    reg18_changed = true;
  }
  int filters = 0;
  for (int t = 0; t < 3; t++) {
    bool reg00_changed = false; // reg 00 and 01: freq
    bool reg02_changed = false; // sustain+release
    bool reg04_changed = false; // wave + flags
    bool reg05_changed = false; // attack+decay
    bool reg06_changed = false; // attack+decay
    bool no_new_note = false;
    if (voices[t].filter) {
      filters |= (1 << t);
    }
    if (tval[t].note != zzub::note_value_none) {
      voices[t].note = tval[t].note;
      if (tval[t].note == 0) {
	// stop voice
	voices[t].on = false;
	reg04_changed = true; // disable voice
      } else {
	// set up note
	float tnote = tval[t].note;
	float Fout = 440.0f * pow(2.0f, (tnote - 69.0f) / 12.0f) * samplerate / samplerate;
	voices[t].freq = Fout;//Fout / 0.058725357f; // Fout = (Fn * 0.058725357) Hz for PAL
	reg18_changed = true; // set new volume with note
	//sid_write(0x18, 15); // max volume
	//sid_write(0x05 + t*7, 0xCC); // attack/&decay
	//sid_write(0x06 + t*7, 0xCC); // sustain/release
	//sid_write(0x02 + t*7, 0x2C); // pw lo
	//sid_write(0x03 + t*7, 0x0A); // pw hi
	voices[t].on = true;
	reg00_changed = true; // new freq
	reg04_changed = true; // enable voice
      }
    }
    if (tval[t].effect != 0) {
      int eff = tval[t].effect;
      if (eff == 0x30) {
	no_new_note = true;
      }
    }
    if (tval[t].effectvalue != 0) {
      
    }
    if (tval[t].pw != 65535) {
      int pw = tval[t].pw;
      unsigned char pwlo = pw & 0xFF;
      unsigned char pwhi = pw >> 8;
      regs[0x02 + t * 7] = pwlo;
      regs[0x03 + t * 7] = pwhi;
      //sid_write(0x02 + t*7, pwlo);
      //sid_write(0x03 + t*7, pwhi);
    }
    if (tval[t].wave != 255) {
      voices[t].wave = 1 << (tval[t].wave);
      reg04_changed = true;
    }
    if (tval[t].filtervoice != zzub::switch_value_none) {
      voices[t].filter = tval[t].filtervoice;
      reg17_changed = true;
    }
    if (tval[t].ringmod != zzub::switch_value_none) {
      voices[t].ringmod = tval[t].ringmod;
      reg04_changed;
    }
    if (tval[t].sync != zzub::switch_value_none) {
      voices[t].sync = tval[t].sync;
      reg04_changed;
    }
    if (tval[t].attack != 255) {
      voices[t].attack = tval[t].attack;
      reg05_changed = true;
    }
    if (tval[t].decay != 255) {
      voices[t].decay = tval[t].decay;
      reg05_changed = true;
    }
    if (tval[t].sustain != 255) {
      voices[t].sustain = tval[t].sustain;
      reg06_changed = true;
    }
    if (tval[t].release != 255) {
      voices[t].release = tval[t].release;
      reg06_changed = true;
    }
    // write out changed registers for this voice:
    if (reg00_changed) {
      // FREQUENCY = (REGISTER VALUE * CLOCK)/16777216 Hz
      int value = voices[t].freq * 16777216.0f / clockrate;
      unsigned char freqhi = value >> 8;
      unsigned char freqlo = value & 0xFF;
      int tlo = freqtbllo[voices[t].note];
      int thi = freqtblhi[voices[t].note];
      //printf("calculated freqs: %x %x; table freqs: %x, %x", freqhi, freqlo, thi, tlo)
      regs[0x00 + t*7] = tlo;
      regs[0x01 + t*7] = thi;
      //sid_write(0x00 + t*7, tlo);
      //sid_write(0x01 + t*7, thi);
      //sid_write(0x00 + t*7, freqlo);
      //sid_write(0x01 + t*7, freqhi);
    }
    if (reg05_changed) {
      unsigned char attackdecay = (voices[t].attack << 4) | voices[t].decay;
      regs[0x05 + t*7] = attackdecay;
      //sid_write(0x05 + t*7, attackdecay);
    }
    if (reg06_changed) {
      unsigned char sustainrelease = (voices[t].sustain << 4) | voices[t].release;
      regs[0x06 + t*7] = sustainrelease;
      //sid_write(0x06 + t*7, sustainrelease);
    }
    // reg04 = wave + enable, do this last
    if (reg04_changed) {
      unsigned char ctrl =
	(voices[t].wave << 4) |
	(voices[t].ringmod << 2) |
	(voices[t].sync << 1) |
	(voices[t].on);
      regs[0x04 + t*7] = ctrl;
      //sid_write(0x04 + t*7, ctrl);
    }
  }
  // write out global registers
  if (reg17_changed) {
    unsigned char resfilt = (resonance << 4) | filters;
    regs[0x17] = resfilt;
    //sid_write(0x17,  resfilt);
  }
  if (reg18_changed) {
    unsigned char modevol = (mode << 4) | volume;
    regs[0x18] = modevol;
    //sid_write(0x18,  modevol);
  }
  flush_regs = true;
}

unsigned char SIDMachine::sid_getorder(unsigned char index)
{
  //if (adparam >= 0xf000)
  //return altsidorder[index];
  //else
  return sidorder[index];
}

void SIDMachine::process_stereo_goat(float *inL, float *inR, 
				   float *outL, float *outR, int n)
//int sid_fillbuffer(short *ptr, int samples)
{
  short buf[512];
  short* ptr = buf;
  int samples = n;  
  int tdelta;
  int tdelta2;
  int result;
  int total = 0;
  int c;
  int badline = 0;//rand() % NUMSIDREGS;
  tdelta = clockrate * samples / samplerate + 4;
  for (c = 0; c < NUMSIDREGS; c++) {
    unsigned char o = sid_getorder(c);
    // Extra delay for loading the waveform (and mt_chngate,x)
    if ((o == 4) || (o == 11) || (o == 18)) {
      tdelta2 = SIDWAVEDELAY;
      result = emu.clock(tdelta2, ptr, samples);
      total += result;
      ptr += result;
      samples -= result;
      tdelta -= SIDWAVEDELAY;
    }
    // Possible random badline delay once per writing
    /*if ((badline == c) && (residdelay))
      {
      tdelta2 = residdelay;
      result = sid->clock(tdelta2, ptr, samples);
      total += result;
      ptr += result;
      samples -= result;
      tdelta -= residdelay;
      }*/
    emu.write(o, regs[o]);
    tdelta2 = SIDWRITEDELAY;
    result = emu.clock(tdelta2, ptr, samples);
    total += result;
    ptr += result;
    samples -= result;
    tdelta -= SIDWRITEDELAY;
  }
  result = emu.clock(tdelta, ptr, samples);
  total += result;
  for (int i = 0; i < n; i++) {
    short s = buf[i];//emu.output();
    float fs = (float)s / 32767.0f;
    outL[i] = fs;
    outR[i] = fs;
  }
}

bool SIDMachine::process_stereo(float **pin, float **pout, int n, int mode) {
  if (flush_regs) {
    for (int i = 0; i < 29; i++) {
      emu.write(i, regs[i]);
      cycles += 9;
    }
    flush_regs = false;
  }
  cycles = 0;
  short buf[512];
  //printf("tdelta %i, n %i\n", tdelta, n);
  int samples = n;
  while (samples > 0) {
    int tdelta = clockrate * samples / samplerate + 4;// - cycles;
    int result = emu.clock(tdelta, buf, n); // en c64 går i hva, 1mhz?
    samples -= result;
    if (result < n)
      printf("result: %i, was %i\n", result, n);
  }
  for (int i = 0; i < n; i++) {
    short s = buf[i];//emu.output();
    float fs = (float)s / 32767.0f;
    pout[0][i] = fs;
    pout[1][i] = fs;
  }
  return true;
}

const char *SIDMachine::describe_value(int param, int value) {
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
