/*
  Copyright (C) 2007 Marcin Dabrowski

  This library is free software; you can redistribute
 it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#define _USE_MATH_DEFINES
#include <cstdio>
#include <math.h>
#include <cmath>
#include <float.h>
#include <zzub/signature.h>
#include <zzub/plugin.h>
#include "ComplexFloat.h"
#include "FastCosSin.h"
#include "HilbertPair.h"
#include "Allpass2.h"
#include "LinLog.h"
#include "Bigyo_FrequencyShifter.h"

/////////////////////////////////////////////////////////////////////////////////////


/***

    zzub entry points

***/

const char *zzub_get_signature() {
  return ZZUB_SIGNATURE;
}

zzub::plugincollection *zzub_get_plugincollection() {
  return new freqshifterplugincollection();
}

/***

    machine_info

***/

const zzub::parameter *paraDirectionL = 0;
const zzub::parameter *paraDirectionR = 0;
const zzub::parameter *paraRate = 0;
const zzub::parameter *paraFeedBack = 0;
const zzub::parameter *paraLfoRate =0;
const zzub::parameter *paraLfoAmp =0;
const zzub::parameter *paraWet = 0;
const zzub::parameter *paraDry = 0;


machine_info::machine_info() {
  this->flags =
    zzub::plugin_flag_has_audio_input |
    zzub::plugin_flag_has_audio_output;
  this->name = "Bigyo FrequencyShifter";
  this->short_name = "FreqShift";
  this->author = "Marcin Dabrowski";
  this->uri = "@bigyo/frequency+shifter;1";
  paraRate = &add_global_parameter()
    .set_word()
    .set_state_flag()
    .set_name("Frequency")
    .set_description("Frequency")
    .set_value_min(0x0000)
    .set_value_max(0xFFFE)
    .set_value_none(0xFFFF)
    .set_value_default(0x0000);
  paraDirectionL = &add_global_parameter()
    .set_byte()
    .set_state_flag()
    .set_name("Left Direction")
    .set_description("Left Direction")
    .set_value_min(0x00)
    .set_value_max(0x02)
    .set_value_none(0xFF)
    .set_value_default(0x00);
  paraDirectionR = &add_global_parameter()
    .set_byte()
    .set_state_flag()
    .set_name("Right Direction")
    .set_description("Right Direction")
    .set_value_min(0x00)
    .set_value_max(0x02)
    .set_value_none(0xFF)
    .set_value_default(0x00);
  paraFeedBack = &add_global_parameter()
    .set_word()
    .set_state_flag()
    .set_name("Feedback")
    .set_description("Feedback amount")
    .set_value_min(0x0000)
    .set_value_max(0xfffe)
    .set_value_none(0xffff)
    .set_value_default(0x0000);
  paraLfoRate = &add_global_parameter()
    .set_word()
    .set_state_flag()
    .set_name("Lfo Rate")
    .set_description("Lfo Rate")
    .set_value_min(0x0000)
    .set_value_max(0xFFFE)
    .set_value_none(0xFFFF)
    .set_value_default(0x0000);
  paraLfoAmp = &add_global_parameter()
    .set_word()
    .set_state_flag()
    .set_name("Lfo Amp")
    .set_description("Lfo Amp")
    .set_value_min(0x0000)
    .set_value_max(0xFFFE)
    .set_value_none(0xFFFF)
    .set_value_default(0x0000);
  paraWet = &add_global_parameter()
    .set_word()
    .set_state_flag()
    .set_name("Wet")
    .set_description("Wet")
    .set_value_min(0x0001)
    .set_value_max(0xFFFE)
    .set_value_none(0xFFFF)
    .set_value_default(0xFFFE);
  paraDry = &add_global_parameter()
    .set_word()
    .set_state_flag()
    .set_name("Dry")
    .set_description("Dry")
    .set_value_min(0x0001)
    .set_value_max(0xFFFE)
    .set_value_none(0xFFFF)
    .set_value_default(0x0000);
  add_attribute()
    .set_name("Frequency non-linearity")
    .set_value_min(0)
    .set_value_max(10)
    .set_value_default(5);
  add_attribute()
    .set_name("Max. frequency (Hz)")
    .set_value_min(20)
    .set_value_max(20000)
    .set_value_default(5000);
}


#define miMACHINE_NAME "Bigyo FrequencyShifter"
#define miSHORT_NAME "FreqShift"
#define miMACHINE_AUTHOR "Marcin Dabrowski"
#define miVERSION "1.12"
#define miABOUTTXT1 "Marcin Dabrowski\n"
#define miABOUTTXT2	"bigyo@wp.pl\n"
#define miABOUTTXT miMACHINE_NAME" v"miVERSION"\n\nbuild: "__DATE__"\n\n"miABOUTTXT1""miABOUTTXT2

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

freqshifter::freqshifter()
{
  
  
  global_values = &gval;
  attributes = (int*)&aval;
  sine_table = new (float [SINE_TABLE_SIZE]);
  filltable(sine_table);
  
}

freqshifter::~freqshifter()
{
  delete [] sine_table;
}

void freqshifter::init(zzub::archive *const pi)
{
  feedback = 0.0;
  feedL = feedR = 0.0;
  lfo_phase = 0.0;
}

void freqshifter::attributes_changed()
{
  MaxRate = (float)aval.maxfreq;
  slope = powf(0.5f, (float)aval.nonlinearity + 1.0f );
  float freq = (rate / (float)paraRate->value_max) * MaxRate ;
  float omega = freq2omega((float)linlog(freq, 0.0, MaxRate, slope));
  carrier.setOmega(omega);
}

void freqshifter::save(zzub::archive *const po)
{

}

void freqshifter::process_events()
{
  if (gval.Rate != paraRate->value_none) {
    rate = gval.Rate;
    float freq = (rate / (float) paraRate->value_max) * MaxRate;
    float omega = freq2omega((float)linlog(freq, 0.0, MaxRate, slope));
    carrier.setOmega(omega);
  }
  if (gval.FeedBack != paraFeedBack->value_none) {
    feedback = gval.FeedBack / float(paraFeedBack->value_max);
  }
  if (gval.Wet != paraWet->value_none) {
    wet = (float)gval.Wet / paraWet->value_max;
  }

  if (gval.Dry != paraDry->value_none) {
    dry = (float)gval.Dry / paraWet->value_max;
  }

  if (gval.DirectionL != paraDirectionL->value_none) {
    dirL = gval.DirectionL;
  }

  if (gval.DirectionR != paraDirectionR->value_none) {
    dirR = gval.DirectionR;
  }

  if (gval.LfoRate != paraLfoRate->value_none){
    lfo_rate = 0.01 +
      pow(gval.LfoRate / float(paraLfoRate->value_max), 4.0) * 1000.0;
  }

  if (gval.LfoAmp != paraLfoRate->value_none){
    lfo_amp =  (gval.LfoAmp / float(paraLfoAmp->value_max)) * 2.0;
  }
}

bool freqshifter::process_stereo(float** pin, float** pout,
				 int numsamples, int mode)
{
  if (mode == zzub::process_mode_write)
    return false;
  if (mode == zzub::process_mode_no_io)
    return false;
  if (mode == zzub::process_mode_read) // <thru>
    return true;
  float *psamples[2] = {
    pin[0],
    pin[1]
  };
  float *rsamples[2] = {
    pout[0],
    pout[1]
  };
  float dval; // dry value
  float wval; // wet value
  for (int i = 0; i < numsamples; i++) {
    float frate = (rate / paraRate->value_max) + 
      lsinus(lfo_phase) * lfo_amp * 0.5;
    if (frate < 0.0) {
      frate = 0.0;
    }
    if (frate > 1.0) {
      frate = 1.0;
    }
    float freq = frate * MaxRate;
    float omega = freq2omega((float)linlog(freq, 0.0, MaxRate, slope));
    carrier.setOmega(omega);
    complex<float> c = carrier.process();
    if (dirL) {
      dval = psamples[0][i];
      complex<float> l = hL.process(psamples[0][i] + feedL * feedback);
      wval = (dirL == 1) ?
	(c.re * l.re - c.im * l.im) :
	(c.re * l.re + c.im * l.im);
      feedL = wval;
      rsamples[0][i] = dval * dry + wval * wet;
    }
    if (dirR) {
      dval = psamples[1][i];
      complex<float> r = hR.process(psamples[1][i] + feedR * feedback);
      wval = (dirR == 1) ?
	(c.re * r.re - c.im * r.im) :
	(c.re * r.re + c.im * r.im);
      feedR = wval;
      rsamples[1][i] = dval * dry + wval * wet;
    }
  }
  return true;
}

void freqshifter::command(int const i)
{
  switch (i) {
  case 0:
    _host->message(miABOUTTXT);
    break;
  default:
    break;
  }
}

char const *freqshifter::describe_value(int const param, int const value) {
  int n;
  float v, v1;
  static char txt[16];
  switch(param) {
  case 0: //  Rate
    v = (float)linlog(MaxRate * value / paraRate->value_max, 0.0f,
		      MaxRate, slope);
    v1 = (float)linlog(MaxRate * (value + 1) / paraRate->value_max, 0.0f,
		       MaxRate, slope);
    n = (int)(1.0f - log10f(v1 - v));
    if (n < 0)
      n = 0;
    sprintf(txt, "%.*f Hz", n, v);
    break;
  case 1: // Dir L
  case 2: // Dir R
    switch(value) {
    case 0:
      return("Off");
    case 1:
      return("Down");
    case 2:
      return("Up");
    }
  case 3: // Feedback
    {
      sprintf(txt, "%.3f", float(value) / 0xfffe);
      break;
    }
  case 4:  //
    {
      sprintf(txt, "%.3f Hz", lfo_rate);
      break;
    }
  case 5:  //
    {
      sprintf(txt, "%.3f", lfo_amp * 0.5f);
      break;
    }
  case 6: //  Wet
    {
      if (wet == 0.0f) {
	sprintf(txt, "-inf dB");
      } else {
	sprintf(txt, "%.3f dB", 10.0 * log10(wet));
      }
      break;
    }
  case 7: // Dry
    {
      if (dry == 0.0f) {
	sprintf(txt, "-inf dB");
      } else {
	sprintf(txt, "%.3f dB", 10.0 * log10(dry));
      }
      break;
    }

  default:
    sprintf(txt, "%.2f %%", (float)value / 65534.0f * 100.0f);
  }
  return txt;
}


float freqshifter::sinus(float &phase)
{
  float rate = lfo_rate / _master_info->samples_per_second;
  float result = sin(2.0 * M_PI * phase);
  phase += rate;
  while (phase >= 1.0)
    phase -= 1.0;
  return result;
}

float freqshifter::lsinus(float &phase)
{
  
  float rate = lfo_rate / _master_info->samples_per_second;
  float result = lut_sin(TWOPI * phase);
  phase += rate;
  while (phase > 1.0)
    phase -= 1.0;
  return result;
  
}

void freqshifter::filltable(float *table)
{
  
  for(int  i = 0; i < SINE_TABLE_SIZE; i++){
    table[i] = sin( i * TWOPI_OVER_LEN );
  }

}

