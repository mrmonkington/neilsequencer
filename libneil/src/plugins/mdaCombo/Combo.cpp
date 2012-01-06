#include <cstdio>
#include <cmath>
#include <cstring>

#include "Combo.hpp"

Combo::Combo() {
  global_values = &gval;
  track_values = 0;
  attributes = 0;
}

void Combo::init(zzub::archive *pi) {
  //inits here!
  fParam1 = 1.00f; //select
  fParam2 = 0.50f; //drive
  fParam3 = 0.50f; //bias
  fParam4 = 0.50f; //output
  fParam5 = 0.40f; //stereo
  fParam6 = 0.00f; //hpf freq
  fParam7 = 0.50f; //hpf reso

  size = 1024;
  bufpos = 0;
  buffer = new float[size];
  buffe2 = new float[size];

  suspend(); // flush buffer

  process_events(); //go and set initial values!
}

void Combo::destroy() {
  if (buffer) {
    delete[] buffer;
  }
  if (buffe2) {
    delete[] buffe2;
  }
}

void Combo::suspend() 
{
  memset(buffer, 0, size * sizeof(float));
  memset(buffe2, 0, size * sizeof(float));
  ff1 = 0.f; 
  ff2 = 0.f; 
  ff3 = 0.f; 
  ff4 = 0.f; 
  ff5 = 0.f;
  ff6 = 0.f; 
  ff7 = 0.f; 
  ff8 = 0.f;
  ff9 = 0.f; 
  ff10 = 0.f;
  hh0 = hh1 = 0.0f;
}

float Combo::filterFreq(float hz)
{
  float j, k, r = 0.999f;
  j = r * r - 1;
  k = (float)(2.f - 2.f * r * r * cos(0.647f * hz / _master_info->samples_per_second));
  return (float)((sqrt(k * k - 4.f * j * j) - k) / (2.f * j));
}
	
void Combo::process_events() {
  if (gval.select != 65535) {
    fParam1 = gval.select * 0.001f;
  }
  if (gval.drive != 65535) {
    fParam2 = gval.drive * 0.001f;
  }
  if (gval.bias != 65535) {
    fParam3 = gval.bias * 0.001f;
  }
  if (gval.output != 65535) {
    fParam4 = gval.output * 0.001f;
  }
  if (gval.stereo != 65535) {
    fParam5 = gval.stereo * 0.001f;
  }
  if (gval.hpf_freq != 65535) {
    fParam6 = gval.hpf_freq * 0.001f;
  }
  if (gval.hpf_reso != 65535) {
    fParam7 = gval.hpf_reso * 0.001f;
  }
  //calcs here
  ster = 0; 
  if (fParam5 > 0.5) {
    ster = 1;
  }
  hpf = filterFreq(25.f);
  switch (int(fParam1 * 6.9)) {
  case 0: 
    trim = 0.5f; 
    lpf = 0.f; //DI
    mix1 = (float)0.0; 
    mix2 = (float)0.0;
    del1 = 0; 
    del2 = 0; 
    break;
  case 1: 
    trim = 0.53f; 
    lpf = filterFreq(2700.f); //speaker sim
    mix1 = (float)0.0; 
    mix2 = (float)0.0;
    del1 = 0; 
    del2 = 0;
    hpf = filterFreq(382.f); 
    break;
  case 2: 
    trim = 1.10f; 
    lpf = filterFreq(1685.f); //radio
    mix1 = -1.70f; 
    mix2 = 0.82f; 
    del1 = int(_master_info->samples_per_second / 6546.f);
    del2 = int(_master_info->samples_per_second / 4315.f); 
    break;
  case 3: 
    trim = 0.98f; 
    lpf = filterFreq(1385.f); //mesa boogie 1"
    mix1 = -0.53f; 
    mix2 = 0.21f;
    del1 = int(_master_info->samples_per_second / 7345.f);
    del2 = int(_master_info->samples_per_second / 1193.f); 
    break;	    
  case 4: 
    trim = 0.96f; 
    lpf = filterFreq(1685.f); //mesa boogie 8"
    mix1 = -0.85f; 
    mix2 = 0.41f; 
    del1 = int(_master_info->samples_per_second / 6546.f);
    del2 = int(_master_info->samples_per_second / 3315.f); 
    break;
  case 5: 
    trim = 0.59f; 
    lpf = filterFreq(2795.f); 
    mix1 = -0.29f; 
    mix2 = 0.38f; //Marshall 4x12" celestion
    del1 = int(_master_info->samples_per_second / 982.f);
    del2 = int(_master_info->samples_per_second / 2402.f);
    hpf = filterFreq(459.f); 
    break;
  case 6: 
    trim = 0.30f; 
    lpf = filterFreq(1744.f); //scooped-out metal 
    mix1 = -0.96f; 
    mix2 = 1.6f; 
    del1 = int(_master_info->samples_per_second / 356.f);
    del2 = int(_master_info->samples_per_second / 1263.f);
    hpf = filterFreq(382.f); 
    break;
  }
  mode = (fParam2 < 0.5) ? 1 : 0; 
  if (mode) {
    //soft clipping
    drive = (float)pow(10.f, 2.f - 6.f * fParam2);
    trim *= 0.55f + 150.f * (float)pow(fParam2, 4.0f);
  } else {
    //hard clipping
    drive = 1.f;
    clip = 11.7f - 16.f * fParam2;
    if (fParam2 > 0.7) {
      drive = (float)pow(10.0f, 7.f * fParam2 - 4.9f);
      clip = 0.5f;
    }
  }
  bias = 1.2f * fParam3 - 0.6f;
  if (fParam2 > 0.5) {
    bias /= (1.f + 3.f * (fParam2 - 0.5f));
  } else {
    bias /= (1.f + 3.f * (0.5f - fParam2));
  }
  trim *= (float)pow(10.f, 2.f * fParam4 - 1.f);
  if (ster) {
    trim *= 2.f;
  }

  hhf = fParam6;
  hhq = 1.1f - fParam7;
  if (fParam6 > 0.05f) {
    drive = drive * (1 + 0.1f * drive);
  }
}

bool Combo::process_stereo(float **pin, float **pout, int sampleFrames, int pmode) {
  if (pmode != zzub::process_mode_read_write) {
    bool buffers_empty = true;
    for (int i = 0; i < size; i++) {
      if (buffer[i] > 0.0001f || buffe2[i] > 0.0001f) {
	buffers_empty = false;
      }
    }
    if (buffers_empty) {
      return false;
    }
  }
  float *in1 = pin[0];
  float *in2 = pin[1];
  float *out1 = pout[0];
  float *out2 = pout[1];
  float a, b, trm, m1 = mix1, m2 = mix2, clp = clip;	
  float o = lpf, i = 1.f - lpf, o2 = hpf, bi = bias, drv = drive;
  float f1 = ff1, f2 = ff2, f3 = ff3, f4 = ff4, f5 = ff5;
  float a2, b2, f6 = ff6, f7 = ff7, f8 = ff8, f9 = ff9, f10 = ff10;
  float hf = hhf, hq = hhq, h0 = hh0, h1 = hh1;
  int d1 = del1, d2 = del2, bp = bufpos;
  
  trm = trim * i * i * i * i;

  --in1;	
  --in2;	
  --out1;
  --out2;

  if (ster) {
    //stereo
    while (--sampleFrames >= 0) {
      a = drv * (*++in1 + bi); 
      a2 = drv * (*++in2 + bi);

      if (mode) {
        b = (a > 0.f) ? a : -a;    
	b2 = (a2 > 0.f) ? a2 : -a2;
        b = a / (1.f + b);       
	b2 = a2 / (1.f + b2);
      } else {
        b = (a > clp) ? clp : a;  
	b2 = (a2 > clp) ? clp : a2; //distort
        b = (a < -clp) ? -clp : b;  
	b2 = (a2 < -clp) ? -clp : b2;
      }

      *(buffer + bp) = b;        
      *(buffe2 + bp) = b2; 
      b += (m1* *(buffer + ((bp + d1) % 1000))) + (m2* *(buffer + ((bp + d2) % 1000)));
      b2+= (m1* *(buffe2 + ((bp + d1) % 1000))) + (m2* *(buffe2 + ((bp + d2) % 1000)));
  
      f1 = o * f1 + trm * b;     
      f6 = o * f6 + trm * b2;
      f2 = o * f2 + f1;      
      f7 = o * f7 + f6;
      f3 = o * f3 + f2;      
      f8 = o * f8 + f7;
      f4 = o * f4 + f3;      
      f9 = o * f9 + f8; // -24dB/oct filter

      f5 = o2 * (f5 - f4) + f4; 
      f10 = o2 * (f10 - f9) + f9; // high pass
      b = f4 - f5;               
      b2 = f9 - f10;

      if (bp == 0) {
	bufpos = 999;
      } else {
	bufpos = bp - 1;
      }
 
      *++out1 = b;	
      *++out2 = b2;
    }
  } else {
    //mono
    if (mode) {
      //soft clip
      while (--sampleFrames >= 0) {
	a = drv * (*++in1 + *++in2 + bi);
          
        h0 += hf * (h1 + a); // resonant highpass (Chamberlin SVF)
        h1 -= hf * (h0 + hq * h1);
        a += h1;

        b = (a > 0.f) ? a : -a;
        b = a / (1.f + b);

        *(buffer + bp) = b; 
        b += (m1* *(buffer + ((bp + d1) % 1000))) + (m2* *(buffer + ((bp + d2) % 1000)));
    
        f1 = o * f1 + trm * b;
        f2 = o * f2 + f1;
        f3 = o * f3 + f2;
        f4 = o * f4 + f3; // -24dB/oct filter

        f5 = o2 * (f5 - f4) + f4; // high pass
        b = f4 - f5;

        bp = (bp == 0) ? 999 : bp - 1; // buffer position

        *++out1 = b;
	*++out2 = b;
      }
    } else {
      //hard clip
      while (--sampleFrames >= 0) {
	a = drv * (*++in1 + *++in2 + bi);
      
        h0 += hf * (h1 + a); // resonant highpass (Chamberlin SVF)
        h1 -= hf * (h0 + hq * h1);
        a += h1;

        b = (a > clp) ? clp : a; //distort
        b = (a < -clp) ? -clp : b;
      
        *(buffer + bp) = b; 
        b += (m1* *(buffer + ((bp + d1) % 1000))) + (m2* *(buffer + ((bp + d2) % 1000)));
    
        f1 = o * f1 + trm * b;
        f2 = o * f2 + f1;
        f3 = o * f3 + f2;
        f4 = o * f4 + f3; //-24dB/oct filter

        f5 = o2 * (f5 - f4) + f4; //high pass //also want smile curve here...
        b = f4 - f5;

        bp = (bp == 0) ? 999 : bp - 1; //buffer position

        *++out1 = b;
	*++out2 = b;
      }
    }
  }
  bufpos = bp;
  if (fabs(f1) < 1.0e-10) { 
    ff1 = 0.f; 
    ff2 = 0.f; 
    ff3 = 0.f; 
    ff4 = 0.f; 
    ff5 = 0.f;  
  } else { 
    ff1 = f1; 
    ff2 = f2;  
    ff3 = f3; 
    ff4 = f4;  
    ff5 = f5;   
  }
  if (fabs(f6) < 1.0e-10) { 
    ff6 = 0.f; 
    ff7 = 0.f; 
    ff8 = 0.f; 
    ff9 = 0.f; 
    ff10 = 0.f; 
  } else { 
    ff6 = f6;  
    ff7 = f7; 
    ff8 = f8;  
    ff9 = f9;  
    ff10 = f10; 
  }
  if (fabs(h0) < 1.0e-10) { 
    hh0 = hh1 = 0.0f; 
  } else { 
    hh0 = h0;
    hh1 = h1; 
  }
  return true;
}

const char *Combo::describe_value(int param, int value) {
  static char txt[20];
  switch (param) {
  case 0:
    switch (int(fParam1 * 6.9)) {
    case 0:
      sprintf(txt, "D.I.");
      break;
    case 1:
      sprintf(txt, "Spkr Sim");
      break;
    case 2:
      sprintf(txt, "Radio");
      break;
    case 3:
      sprintf(txt, "MB 1\"");
      break;
    case 4:
      sprintf(txt, "MB 8\"");
      break;
    case 5:
      sprintf(txt, "4x12 ^");
      break;
    case 6:
      sprintf(txt, "4x12 >");
      break;
    }
    break;
  case 1:
    sprintf(txt, "%d S<>H", int(200 * fParam2 - 100));
    break;
  case 2:
    sprintf(txt, "%d", int(200 * fParam3 - 100));
    break;
  case 3:
    sprintf(txt, "%d dB", int(40 * fParam4 - 20));
    break;
  case 4:
    if (fParam5 > 0.5) {
      sprintf(txt, "STEREO");
    } else {
      sprintf(txt, "MONO");
    }
    break;
  case 5:
    sprintf(txt, "%d%%", int(100 * fParam6));
    break;
  case 6:
    sprintf(txt, "%d%%", int(100 * fParam7));
    break;
  default:
    return 0;
  }
  return txt;
}
