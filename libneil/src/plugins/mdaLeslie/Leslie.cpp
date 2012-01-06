#include <cstdio>
#include <cstring>

#include "Leslie.hpp"

Leslie::Leslie() {
  global_values = &gval;
  track_values = 0;
  attributes = 0;
  size = 256;
  hpos = 0;
  hbuf = new float[size];
  fbuf1 = fbuf2 = 0.0f;
  twopi = 6.2831853f;
  speed = output = xover = hiwidth = hidepth = hithrob = lowidth = finespeed = lothrob = 0.0;
}

Leslie::~Leslie() {
  if (hbuf) {
    delete [] hbuf;
  }
}

void Leslie::init(zzub::archive *pi) {
  memset(hbuf, 0, size * sizeof(float));
}

void Leslie::process_events() {
  if (gval.speed != 65535) {
    //0
    speed = (float) gval.speed / 1000.0;
  }
  if (gval.output != 65535) {
    //1
    output = (float) gval.output / 1000.0;
  }
  if (gval.xover != 65535) {
    //2
    xover = (float) gval.xover / 1000.0;
  }
  if (gval.hiwidth != 65535) {
    //3
    hiwidth = (float) gval.hiwidth / 1000.0;
  }
  if (gval.hidepth != 65535) {
    //4
    hidepth = (float) gval.hidepth / 1000.0;
  }
  if (gval.hithrob != 65535) {
    //5
    hithrob = (float) gval.hithrob / 1000.0;
  }
  if (gval.lowidth != 65535) {
    //6
    lowidth = (float) gval.lowidth / 1000.0;
  }
  if (gval.finespeed != 65535) {
    //7
    finespeed = (float) gval.finespeed / 1000.0;
  }
  if (gval.lothrob != 65535) {
    //8
    lothrob = (float) gval.lothrob / 1000.0;
  }
  float ifs = 1.0f / _master_info->samples_per_second;
  float spd = twopi * ifs * 2.0f * finespeed;

  //calcs here!
  filo = 1.f - (float)pow(10.0f, xover * (2.27f - 0.54f * xover) - 1.92f);

  if(speed<0.50f)
    {  
      if(speed<0.1f) //stop
	{ 
	  lset = 0.00f; hset = 0.00f;
	  lmom = 0.12f; hmom = 0.10f; 
	}
      else //low speed
	{ 
	  lset = 0.49f; hset = 0.66f;
	  lmom = 0.27f; hmom = 0.18f;
	}
    }
  else //high speed
    {  
      lset = 5.31f; hset = 6.40f;
      lmom = 0.14f; hmom = 0.09f;
    }
  hmom = (float)pow(10.0f, -ifs / hmom);
  lmom = (float)pow(10.0f, -ifs / lmom); 
  hset *= spd;
  lset *= spd;

  gain = 0.4f * (float)pow(10.0f, 2.0f * output - 1.0f);
  lwid = lowidth * lowidth;
  llev = gain * 0.9f * lothrob * lothrob;
  hwid = hiwidth * hiwidth;
  hdep = hidepth * hidepth * _master_info->samples_per_second / 760.0f;
  hlev = gain * 0.9f * hithrob * hithrob;
}

bool Leslie::process_stereo(float **pin, float **pout, int n, int mode) {
  if (mode != zzub::process_mode_read_write) {
    bool buffers_empty = true;
    for (unsigned int i = 0; i < size; i++) {
      if (hbuf[i] > 0.0001f) {
	buffers_empty = false;
      }
      if (buffers_empty) {
	return false;
      }
    }
  }
  float a; // input accumulator
  float c, d; // left and right accumulators
  float g=gain;
  float h, l; // high and low separations
  float fo=filo, fb1=fbuf1, fb2=fbuf2;
  float hl=hlev, hs=hspd, ht, hm=hmom, hp=hphi, hw=hwid, hd=hdep;
  float ll=llev, ls=lspd, lt, lm=lmom, lp=lphi, lw=lwid;
  float hint, k0=0.03125f, k1=32.f; //k0 = 1/k1
  long  hdd, hdd2, k=0, hps=hpos;

  ht=hset*(1.f-hm); //target speeds
  lt=lset*(1.f-lm);

  chp = (float)cos(hp); chp *= chp * chp; //set LFO values
  clp = (float)cos(lp);
  shp = (float)sin(hp);
  slp = (float)sin(lp);

  int s = 0;
  while(s < n)
    {
      a = pin[0][s] + pin[1][s]; //mono input aha!

      if(k) k--; else //linear piecewise approx to LFO waveforms
        {
	  ls = (lm * ls) + lt; //tend to required speed
	  hs = (hm * hs) + ht;
	  lp += k1 * ls;
	  hp += k1 * hs;
                           
	  dchp = (float)cos(hp + k1*hs);
	  dchp = k0 * (dchp * dchp * dchp - chp); //sin^3 level mod
	  dclp = k0 * ((float)cos(lp + k1*ls) - clp);
	  dshp = k0 * ((float)sin(hp + k1*hs) - shp);
	  dslp = k0 * ((float)sin(lp + k1*ls) - slp);

	  k=(long)k1;
        }

      // crossover filter
      fb1 = fo * (fb1 - a) + a;
      fb2 = fo * (fb2 - fb1) + fb1;  
      h = (g - hl * chp) * (a - fb2);
      l = (g - ll * clp) * fb2;

      if(hps>0)
	hps--;
      else
	hps=200;  //delay input pos

      hint = hps + hd * (1.0f + chp); //delay output pos 
      hdd = (int)hint; 
      hint = hint - hdd; //linear intrpolation
      hdd2 = hdd + 1;
      if(hdd>199) {
	if(hdd>200) hdd -= 201; hdd2 -= 201;
      }

      *(hbuf + hps) = h; //delay input
      a = *(hbuf + hdd);
      h += a + hint * ( *(hbuf + hdd2) - a); //delay output

      // both channels set to low + high mix
      c = l + h; 
      d = l + h;

      // 
      h *= hw * shp;
      l *= lw * slp;
      d += l - h;
      c += h - l;

      pout[0][s] = c; //output
      pout[1][s] = d;

      chp += dchp;
      clp += dclp;
      shp += dshp;
      slp += dslp;

      s++;
    }
  lspd = ls;
  hspd = hs;
  hpos = hps;
  lphi = (float)fmod(lp+(k1-k)*ls,twopi);
  hphi = (float)fmod(hp+(k1-k)*hs,twopi);
  if(fabs(fb1)>1.0e-10)
    fbuf1=fb1;
  else
    fbuf1=0.0f; //catch denormals
  if(fabs(fb2)>1.0e-10)
    fbuf2=fb2;
  else
    fbuf2=0.0f; 
  return true;
}

const char *Leslie::describe_value(int param, int value) {
  static char txt[20];
  float fval = (float)value / 1000.0;
  switch (param) {
  case 0:
    if(fval<0.5f) { 
      if(fval < 0.1f) {
	strcpy(txt, "STOP");
      } else {
	strcpy(txt, "SLOW");
      }
    } else {
      strcpy(txt, "FAST");
    }   
    break;
  case 1:
    sprintf(txt, "%0.1fdB", 40 * fval - 20);
    break;
  case 2:
    sprintf(txt, "%0.1fHz", 10 * (float)pow( 10.0f, 1.179f + fval ) );
    break;
  case 3:
    sprintf(txt, "%0.1f%%", fval * 100);
    break;
  case 4:
    sprintf(txt, "%0.1f%%", fval * 100);
    break;
  case 5:
    sprintf(txt, "%0.1f%%", fval * 100);
    break;
  case 6:
    sprintf(txt, "%0.1f%%", fval * 100);
    break;
  case 7:
    sprintf(txt, "%0.1f%%", fval * 200);
    break;
  case 8:
    sprintf(txt, "%0.1f%%", fval * 100);
    break;
  default:
    return 0;
  }
  return txt;
}
