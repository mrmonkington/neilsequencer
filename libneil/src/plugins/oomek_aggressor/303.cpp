#include <cstdio>
#include <cmath>

#include "303.hpp"
#include "fft.h"

Aggressor::Aggressor() 
{
  global_values = &gval;
  track_values = &tval;
  attributes = 0;
}

Aggressor::~Aggressor() 
{

}

void Aggressor::init(zzub::archive* pi) 
{
  int i;
  int j;

  tunelv = 0;
  amlv = 0.0f;
  oscphase = 0.0;
  oscphaseinc = 0.0;
  osctype = false;
  slidestate = false;
  slidestateold = false;
  Envmodinc = 0.0;
  Accphase1 = 600;
  Accphase2 = 600;
  Accphase3 = 600;
  Accphasel1 = 12200;
  Accphasel2 = 12200;
  Accphasel3 = 12200;
  p_Accphasel1 = accl + 12200 - 1;
  p_Accphasel2 = accl + 12200 - 1;
  p_Accphasel3 = accl + 12200 - 1;
  vcaphase = 1600;

  Flpfold = (float)pow((gval.cutoff / 240.0f), 2.0f) * 0.8775f + 0.1225f;
  Qlpf = (float)pow((gval.resonance / 128.0f), 0.5f);
  Envmod = (float)gval.envmod / 128.0f;

  /// vca table filling///
  for (i = 0; i < 200; i++) 
    vca[i] = 1.0f - (float)pow(((200 - i) / 200.0f), 2.0f);
  for (i = 200; i < 1200; i++) 
    vca[i] = (float)pow(((1200 - i) / 1000.0f), 2.0f) * 0.25f + 0.75f;
  for (i = 1200; i < 1600; i++) 
    vca[i] = (float)pow(((1600 - i) / 400.0f), 1.25f) * 0.75f;
  vca[1600] = 0.0f;

  /// vca acccent table filling///
  for (i = 0; i < 200; i++) 
    accl[i] = (1.0f - (float)pow(((200 - i) / 200.0f), 2.0f)) * 1.5f;
  for (i = 200; i < 12200; i++) 
    accl[i] = (float)pow(((12200 - i) / 12000.0f), 1.0f) * 1.5f;
  accl[12200] = 0.0f;

  /// accent table filling///
  for (i = 0; i < 60; i++) 
    acc[i] = 1.0f - (float)pow(((60 - i) / 60.0f), 2.0f);
  for (i = 60; i < 600; i++) 
    acc[i] = (float)pow(((600 - i) / 540.0f), 4.0f);
  acc[600] = 0.0f;

  /// oscillator pitch table ///
  i = 0;
  do {
    oscpitch[i] = 2048.0f * (440.0f / _master_info->samples_per_second) * 
      (float)(pow(2.0, ((((i + 1) / 100.0) - 69) / 12.0)));	
    i++;
  } while (i != 120 * 100);

  /// Saw oscillator table ///
  for (j = 0; j < 40; j++) {
    for (i = 0; i < 4096; i++) {
      fftable[i] = 0;
    }
    for (i = 1; i < 900 / (float)pow(2.0, ((j / 4.0))); i += 1) {
      fftable[i * 2] = 0;
      fftable[i * 2 + 1] = 1.0f / float(i);
    }
    IFFT(fftable, 2048, 1);	
    for (i = 0; i < 2048; i++) {
      oscsaw[i + 2048 * j] = fftable[i * 2];
    }
  }

  /// Square oscillator table ///
  for (j = 0; j < 40; j++) {
    for (i = 0; i < 4096; i++) {
      fftable[i] = 0;
    }
    for (i = 1; i < 900 / (float)pow(2.0, ((j / 4.0))); i += 2) {
      fftable[i * 2] = 0;
      fftable[i * 2 + 1] = 1.0f / float(i);
    }
    IFFT(fftable, 2048, 1);
    for (i = 0; i < 2048; i++) {
      oscsqa[i + 2048 * j] = 0.5f * oscsaw[i + 2048 * j] + fftable[i * 2];
    }
  }
}

inline float Aggressor::fscale(float x)
{
  x = x / (_master_info->samples_per_second / 44100.0f);
  float wynik = (((((-2.7528f * x) + 3.0429f) * x) + 1.718f) * x) - 
    0.9984f; 
  return wynik;
}

void Aggressor::destroy() 
{

}

void Aggressor::stop() {

}

void Aggressor::set_track_count(int tracks) 
{

}

void Aggressor::process_events() 
{
  if (gval.osctype != zzub::switch_value_none) {
    if (gval.osctype == zzub::switch_value_on) {
      osctype = true;
    } else {
      osctype = false;
    }
  }

  slidestateold = slidestate;

  if (tval.slide != zzub::switch_value_on) {
    slidestate = false;
  } else {
    slidestate = true;	
  }
			
  if (gval.finetune != 0xff) {
    tunelv = gval.finetune - 100;
  }

  if (gval.cutoff != 0xff) {
    Flpfnew = (float)gval.cutoff * 0.004166666666f;
    Flpfnew = Flpfnew * Flpfnew * 0.8775f + 0.1225f;
    Flpfsl = (Flpfnew - Flpfold) / 
      _master_info->samples_per_tick * 1.0f * PITCHRESOLUTION;
  }
	
  if (gval.resonance != 0xff) {
    Qlpfnew = (float)pow((gval.resonance * 0.0078125f), 0.5f);
    Qlpfsl = (Qlpfnew - Qlpf) / 
      _master_info->samples_per_tick * 1.0f * PITCHRESOLUTION;
  }

  if (gval.envmod != 0xff) {
    Envmodnew = (float)gval.envmod * 0.0078125f;
    Envmodsl = (Envmodnew - Envmod) / 
      _master_info->samples_per_tick * 1.0f * PITCHRESOLUTION;
  }

  if (gval.decay != 0xff) {
    Decay = (float)pow((gval.decay * 0.0078125f), 0.1f) * 0.992f;
  }

  if (gval.acclevel != 0xff) {
    Acclevel = ((float)gval.acclevel / 64.0f);
  }

  if (gval.volume != 0xff) {
    amlv = (float)gval.volume * 81.92f;
  }

  if ((tval.note != zzub::note_value_none) && 
      (tval.note != zzub::note_value_off)) {
    if ((tval.note >= zzub::note_value_min + 1) && 
	(tval.note <= zzub::note_value_max - 1)) {
      newnote = (float)(((tval.note >> 4) * 12 + (tval.note & 0x0f) - 1) * 
			100.0) + tunelv;
      if (slidestateold == true) {
	slidenote = (newnote - oldnote) / 
	  _master_info->samples_per_tick * 3.0f * PITCHRESOLUTION;
      } else {
	slidenote = newnote - oldnote;
	vcaphase = 0;
	DoNothing = false;
	Envmodinc = 0.0f;
	Accstate = false;
	if (tval.accent == zzub::switch_value_on) {
	  Accstate = true;
	  if (Accphaseid > 2) {
	    Accphaseid = 0;
	  }
	  Accphaseid ++;
	  switch (Accphaseid) {
	  case 1: 
	    Accphase1 = Accphasel1 = 0; 
	    p_Accphasel1 = accl; 
	    break;
	  case 2: 
	    Accphase2 = Accphasel2 = 0; 
	    p_Accphasel2 = accl; 
	    break;
	  case 3: 
	    Accphase3 = Accphasel3 = 0; 
	    p_Accphasel3 = accl; 
	    break;
	  }
	}
      }
    }
  }
}

bool Aggressor::process_stereo(float **pin, float **pout, int n, int mode) 
{
  if (DoNothing == false) {
    DoNothing = true;
    for (int i = 0; i < n; i++) {
      if (pitchcounter == PITCHRESOLUTION - 1) {
  	/// Note slide computation ///
  	oldnote += slidenote;
  	if (slidenote > 0 && oldnote > newnote) {
  	  oldnote = newnote; 
	  slidenote = 0;
  	} else if (slidenote < 0 && oldnote < newnote) {
  	  oldnote = newnote; 
	  slidenote = 0;
  	}	
  	oscphaseinc = oscpitch[int(oldnote)];
		
  	/// Table Level computation ///
  	osclevel = 0;
  	osclevel = int(log2(oscphaseinc * oscphaseinc * 
			    oscphaseinc * oscphaseinc));
  	if (osclevel < 0) {
  	  osclevel = 0;
	}
  	/// Cutoff slide computation ///
  	Flpfold += Flpfsl;
  	if (Flpfsl > 0 && Flpfold > Flpfnew) {
  	  Flpfold = Flpfnew; 
  	  Flpfsl = 0;
  	} else if (Flpfsl < 0 && Flpfold < Flpfnew) {
  	  Flpfold = Flpfnew; 
  	  Flpfsl = 0;
  	}

  	/// Q slide computation ///
  	Qlpf += Qlpfsl;
  	if (Qlpfsl > 0 && Qlpf > Qlpfnew) {
  	  Qlpf = Qlpfnew; 
  	  Qlpfsl = 0;
  	} else if (Qlpfsl < 0 && Qlpf < Qlpfnew) {
  	  Qlpf = Qlpfnew; 
  	  Qlpfsl = 0;
  	}

  	/// Envmod slide computation ///
  	Envmod += Envmodsl;
  	if (Envmodsl > 0 && Envmod > Envmodnew) {
  	  Envmod = Envmodnew; 
  	  Envmodsl = 0;
  	} else if (Envmodsl < 0 && Envmod < Envmodnew) {
  	  Envmod = Envmodnew; 
  	  Envmodsl = 0;
  	}

  	/// Cutoff scale computation ///
  	if (Accstate == true) {
  	  Envmodinc += (0.125f);
	} else {
  	  Envmodinc += (0.125f * (1 - Decay));
	}

  	Envmodphase = (1.0f / (1 + Envmodinc));
  	Envmodphase =  (Envmodphase * 0.965f + 0.035f) * 
  	  Envmod + (Envmodphase * 0.05f + 0.1f) * (1.0f - Envmod);
 
  	EnvmodphaseY = ((Envmodphase - EnvmodphaseZ) * 0.2f) + 
  	  EnvmodphaseZ;
  	EnvmodphaseZ = EnvmodphaseY;

  	Cutfreq = EnvmodphaseY * (((acc[Accphase1] + acc[Accphase2] + 
  				    acc[Accphase3]) * Acclevel) + 1.0f);
  	Cutfreq = Cutfreq * Flpfold;
  	if (Cutfreq > 0.87f) {
  	  Cutfreq = 0.87f;
	}
  	Cutfreq = Cutfreq * (_master_info->samples_per_second * 0.5f);
	
  	if (Accphase1 < 600) {
  	  Accphase1++;
	}
  	if (Accphase2 < 600) {
  	  Accphase2++;
	}
  	if (Accphase3 < 600) {
  	  Accphase3++;
	}
  	Oscfreq = oscphaseinc * _master_info->samples_per_second / 2048.0f;

  	if (Cutfreq < Oscfreq) {
  	  Cutfreq = Oscfreq;
	}
  	Flpf = Cutfreq / (_master_info->samples_per_second * 0.5f);
	
  	Flpf = fscale(Flpf);
  	if (Flpf > 1) {
  	  Flpf = 1.0f;
	}
  	Qdown =  1.0f - (float)pow(0.75f, Cutfreq / Oscfreq);
  	cf = (Flpf * 1.00f) + 1;
  	Qlpfh = 5.9039f - 7.0114f * cf;
  	cf *= (Flpf + 1);
  	Qlpfh = Qlpfh - 0.416f * cf;
  	cf *= (Flpf + 1);
  	Qlpfh = Qlpfh + 10.655f * cf;
  	cf *= (Flpf + 1);
  	Qlpfh = Qlpfh - 11.753f * cf;
  	cf *= (Flpf + 1);
  	Qlpfh = Qlpfh + 5.398f * cf;
  	cf *= (Flpf + 1);
  	Qlpfh = Qlpfh - 0.9308f * cf;
  	Qlpfh = Qlpfh * Qlpf;
  	Qlpfh *= Qdown;
  	///////////////////////////	
  	pitchcounter = 0;
      } else { 
  	pitchcounter++; 
      }
      /// Waveform generation ///
      oscphase += oscphaseinc;
      if (oscphase >= 2048.0f) {
  	oscphase -= 2048.0f;
      }
      oscphaseint0 = int(oscphase);
      oscphaseint1 = oscphaseint0 + 1;
      if (oscphaseint1 >= 2048) {
  	oscphaseint1 = 0;
      }	
      if (osctype == false) {
  	out = oscsaw[oscphaseint0 + 2048 * osclevel] * 
  	  (1 - (oscphase - oscphaseint0))
  	  + oscsaw[oscphaseint1 + 2048 * osclevel] * 
  	  (oscphase-oscphaseint0);
      } else {
  	out = oscsqa[oscphaseint0 + 2048 * osclevel] * 
  	  (1 - (oscphase - oscphaseint0))
  	  + oscsqa[oscphaseint1 + 2048 * osclevel] * 
  	  (oscphase - oscphaseint0);
      }
      if (vcaphase < 1200) {
  	vcaphase++;
      }
      if ((_master_info->tick_position > 
  	   _master_info->samples_per_tick / 2) && 
  	  (vcaphase < 1600) && (slidestate == false)) {
  	vcaphase++;
      }
      /// Adding VCA ///			
      /// 3p hipass filter ///
      temp = vca[vcaphase];
      temp += *p_Accphasel1;
      temp += *p_Accphasel2;
      temp += *p_Accphasel3;
      out = out * temp;
      if (Accphasel1 < 12200) {
  	Accphasel1++;
  	p_Accphasel1++;
      }
      if (Accphasel2 < 12200) {
  	Accphasel2++;
  	p_Accphasel2++;
      }
      if (Accphasel3 < 12200) {
  	Accphasel3++;
  	p_Accphasel3++;
      }
      // 3p Lowpass Resonant VCF ///
      out = out * (Qlpfh + 1);
      out = out * amlv;
      out = out - Yc * Qlpfh;

      Flpfh = (Flpf + 1) * 0.5f;
      Xaz = Xa;
      Xa = out;
      Yaz = Ya;
      
      Ya = ((Xa + Xaz) * Flpfh) - (Flpf * Yaz);
      out = Ya;
      
      Xbz = Xb;
      Xb = out;
      Ybz = Yb;
      
      Yb = ((Xb + Xbz) * Flpfh) - (Flpf * Ybz);
      out = Yb;
      
      Xcz = Xc;
      Xc = out;
      Ycz = Yc;
      
      Yc = ((Xc + Xcz) * Flpfh) - (Flpf * Ycz);
      
      out = Yc;
      
      ///////////////////
      // Allpass shifter ///
      hFh = -0.998f; // -0.994f, 996
      
      hXa = out;
      hYa = hXa * hFh + hYaz;
      hYaz = hXa - hFh * hYa;
      out = hYa;
      
      // Clipper def -2, 2 ///
      if (out < -14.0f * 8192.0f) {
  	out = -14.0f * 8192.0f;
      }
      if (out > 14.0f * 8192.0f) {
  	out = 14.0f * 8192.0f;
      }
      ///////////////////
      hXb = out;
      hYb = hXb * hFh + hYbz;
      hYbz = hXb - hFh * hYb;
      out = hYb;
      ///////////////////
      ///////////////////
      pout[0][i] = out / 32768.0;
      pout[1][i] = out / 32768.0;
      if (!(pout[0][i] < (1.0 / 32768.0) && 
	    pout[0][i] > (-1.0 / 32768.0) && 
	    vcaphase == 1600)) {
      	DoNothing = false;
      }
      //pout[0]++;
    }
    return true;
  } else {
    do {
      /// Cutoff slide computation ///
      Flpfold += Flpfsl;
      if (Flpfsl > 0 && Flpfold > Flpfnew) {
  	      Flpfold = Flpfnew; 
  	      Flpfsl = 0;
      } else if (Flpfsl < 0 && Flpfold < Flpfnew) {
  	Flpfold = Flpfnew; 
  	Flpfsl = 0;
      }
      /// Q slide computation ///
      Qlpf += Qlpfsl;
      if (Qlpfsl > 0 && Qlpf > Qlpfnew) {
  	Qlpf = Qlpfnew; 
  	Qlpfsl = 0;
      } else if (Qlpfsl < 0 && Qlpf < Qlpfnew) {
  	Qlpf = Qlpfnew; 
  	Qlpfsl = 0;
      }
      /// Envmod slide computation ///
      Envmod += Envmodsl;
      if (Envmodsl > 0 && Envmod > Envmodnew) {
  	Envmod = Envmodnew; 
  	Envmodsl = 0;
      } else if (Envmodsl < 0 && Envmod < Envmodnew) {
  	Envmod = Envmodnew; 
  	Envmodsl = 0;
      }
    } while (--n);
    return false;    //def false
  }
}

const char *Aggressor::describe_value(int param, int value) 
{
  static char txt[16];
  switch(param) {
  case 0:
    switch(value) {
    case 0: 
      return "Saw";
    case 1: 
      return "Square";
    }
  case 1:
    return 0;
    break;
  case 2:
    return 0;
    break;
  case 3:
    return 0;
    break;
  case 4:
    return 0;
    break;
  case 5:
    sprintf(txt, "%i", value);
    return txt;
    break;
  case 6:
    sprintf(txt, "%i ct", value - 100);
    return txt;
    break;
  case 7:
    sprintf(txt, "%i%%", value);
    return txt;
    break;	
  case 8:
    return 0;
    break;
  case 9:
    return 0;
    break;
  case 10:
    return 0;
    break;
  default:
    return 0;
  }
}



