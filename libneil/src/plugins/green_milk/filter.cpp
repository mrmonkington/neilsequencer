/*
  kibibu Green Milk
  Filter classes

  Copyright (C) 2007  Cameron Foale

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "filter.h"

#define _USE_MATH_DEFINES 
#include <math.h>
#include <stdio.h>

#include "green_milk.h"

// State variable filter
void SVFilter::setResonance(float f)
{
  resonance = f;
}

void SVFilter::setFrequency(float cut, int sampRate)
{
  float fs = float(sampRate);

  cutoff = cut/(fs);
  internal_cutoff = 2.0f*sinf(float(M_PI) * cutoff);
  internal_cutoff = std::min(0.9f, internal_cutoff);
  damp = svmin(2.0f*(1.0f - pow(resonance, 0.5f)), svmin(2.0f, 2.0f/internal_cutoff - internal_cutoff*0.5f));
}


void SVFilter::setOutput(int which)
{
  switch(which)
    {
    case Lowpass: out = &lowpass; break;
    case Highpass: out = &highpass; break;
    case Bandpass: out = &bandpass; break;
    case Notch: out = &notch; break;
    }
  return;
}

float SVFilter::filterSample(const float& in)
{
  notch = in - damp * bandpass;
  lowpass += internal_cutoff * bandpass;
  highpass = notch - lowpass;
  bandpass = internal_cutoff * highpass + bandpass;

  return *out;
}

void SVFilter::filter(float * pSamples, int numSamples)
{
  int i;
  for ( i=0; i < numSamples; ++i ) {
    *pSamples = filterSample(*pSamples);
    ++pSamples;
  }	
}

// DistortionFilter
float OversampledDistortionFilter::distortSample(const float& dist, const float& in)
{
  //	f(x) = (1+k)*x/(1+k*abs(x)
  return ((1 + dist) * in) / (1 + (dist * fabsf(in)));
}

float OversampledDistortionFilter::filterSample(const float &in)
{
  float op1, op2;
  float opm1, opm2;

  upper.feed(in);
	
  op1 = distortSample(postDist, filt2.filterSample(distortSample(preDist,upper.next())));
  op2 = distortSample(postDist, filt2.filterSample(distortSample(preDist,upper.next())));

  opm1 = decimator.Calc(op1, op2);	// first

  op1 = distortSample(postDist, filt2.filterSample(distortSample(preDist,upper.next())));
  op2 = distortSample(postDist, filt2.filterSample(distortSample(preDist,upper.next())));

  opm2 = decimator.Calc(op1, op2);	// second

  return decimator2.Calc(opm1, opm2);
}

float OversampledDistortionFilter::filterSampleSerial(const float &in)
{
  float op1, op2;
  float opm1, opm2;
	
  float distin;

  upper.feed(in);

  distin = distortSample(preDist,upper.next());
  op1 = filt.filterSample(distin);
  op1 = filt2.filterSample(op1); 
	
  distin = distortSample(preDist,upper.next());
  op2 = filt.filterSample(distin);
  op2 = filt2.filterSample(op2);

  opm1 = decimator.Calc(distortSample(postDist,op1), distortSample(postDist,op2));	// first

  distin = distortSample(preDist,upper.next());
  op1 = filt.filterSample(distin);
  op1 = filt2.filterSample(op1); 
	
  distin = distortSample(preDist,upper.next());
  op2 = filt.filterSample(distin);
  op2 = filt2.filterSample(op2);
  opm2 = decimator.Calc(distortSample(postDist,op1), distortSample(postDist,op2));	// second

  return decimator2.Calc(opm1, opm2);
}

float OversampledDistortionFilter::distortOnly(const float &in)
{
  float op1, op2;
  float opm1, opm2;

  upper.feed(in);

  op1 = distortSample(preDist,upper.next());
  op2 = distortSample(preDist,upper.next());
  opm1 = decimator.Calc(distortSample(postDist,op1), distortSample(postDist,op2));
  op1 = distortSample(preDist,upper.next());
  op2 = distortSample(preDist,upper.next());
  opm2 = decimator.Calc(distortSample(postDist,op1), distortSample(postDist,op2));

  return decimator2.Calc(opm1, opm2);

}


float OversampledDistortionFilter::filterSampleParallel(const float &in)
{
  float op1, op2;
  float opm1, opm2;
  float distin;

  upper.feed(in);

  distin = distortSample(preDist,upper.next());
  op1 = filt.filterSample(distin);
  op1 += filt2.filterSample(distin); 

  distin = distortSample(preDist,upper.next());
  op2 = filt.filterSample(distin);
  op2 += filt2.filterSample(distin);

  opm1 = decimator.Calc(distortSample(postDist,op1), distortSample(postDist,op2));	// first

  distin = distortSample(preDist,upper.next());
  op1 = filt.filterSample(distin);
  op1 += filt2.filterSample(distin); 

  distin = distortSample(preDist,upper.next());
  op2 = filt.filterSample(distin);
  op2 += filt2.filterSample(distin);

  opm2 = decimator.Calc(distortSample(postDist,op1), distortSample(postDist,op2));	// second

  return decimator2.Calc(opm1, opm2);

}

float OversampledDistortionFilter::filterSampleNoDist(const float &in)
{
  float op1, op2;
  float opm1, opm2;

  upper.feed(in);
	
  op1 = filt2.filterSample(upper.next());
  op2 = filt2.filterSample(upper.next());

  opm1 = decimator.Calc(op1, op2);	// first

  op1 = filt2.filterSample(upper.next());
  op2 = filt2.filterSample(upper.next());

  opm2 = decimator.Calc(op1, op2);	// second

  return decimator2.Calc(opm1, opm2);
}

void OversampledDistortionFilter::setFrequency(float f, int sr)
{
  // compensate for 4x oversampling
  int comp_sr = sr << 2;
  float fmax;

  switch(sequence_mode)
    {
    case Single: filt2.setFrequency(f, comp_sr); break;
    case Parallel:
    case Serial:
      filt.setFrequency(f, comp_sr);
      filt2.copy(filt);
      break;

    case Separate:
      fmax = comp_sr * (1.0f/6.0f);
      filt.setFrequency(f, comp_sr);
      filt2.setFrequency(std::min(f * 1.5f,fmax), comp_sr);
      break;

    case Wide:			
      fmax = comp_sr * (1.0f/6.0f);
      filt.setFrequency(f, comp_sr);
      filt2.setFrequency(std::min(f * 2.5f,fmax), comp_sr);
      break;
      
    case None:
      break;
    }


}

void OversampledDistortionFilter::filter(float * pSamples, int numSamples)
{	
  switch(sequence_mode)
    {
    case None:	// no filter!
		// still want to upshift and distort
      while(numSamples--)
	{
	  *pSamples = distortOnly(*pSamples);
	  ++pSamples;
	}
      break;
    case Single:
      if((preDist > 0.01f) || (postDist > 0.01f))
	{
	  while(numSamples--)
	    {
	      *pSamples = filterSample(*pSamples);
	      ++pSamples;
	    }
	} else {
	while(numSamples--)
	  {
	    *pSamples = filterSampleNoDist(*pSamples);
	    ++pSamples;
	  }
      }
      break;

    case Parallel:
    case Separate:
    case Wide:
      while(numSamples--)
	{
	  *pSamples = filterSampleParallel(*pSamples);
	  ++pSamples;
	}
      break;

    case Serial:
      while(numSamples--)
	{
	  *pSamples = filterSampleSerial(*pSamples);
	  ++pSamples;
	}
      break;
    }

}

std::string OversampledDistortionFilter::describeFilterMode(enum FilterMode fm)
{
  switch(fm)
    {
    case Lowpass: return "LP";
    case Highpass: return "HP";
    case Bandpass: return "BP";
    case Notch: return "N";
    default:
      break;
    }
  return "?";
}

std::string OversampledDistortionFilter::describeOutput(int which)
{
  FilterMode m1,m2;
  FilterSequence seq;
  decodeMode(which, &seq, &m1, &m2);

  static const int max_desc = 50;
  static char desc[max_desc];	// for descriptions

  if(seq == None)
    {
      return "None";
    }

  if(seq == Single)
    {		
      return describeFilterMode(m2);
    }

  static std::string type = "";

  type = "";
  switch(seq)
    {
    case Serial:
      type = "Serial";
      break;
    case Parallel:
      type = "Para";
      break;
    case Separate:
      type = "Sep";
      break;
    case Wide:
      type = "Wide";
      break;
    default:
      break;
    }

  sprintf(desc, "%s%s%s", type.c_str(), describeFilterMode(m1).c_str(), describeFilterMode(m2).c_str());
  return desc;
}


// set up the two filters
void OversampledDistortionFilter::setOutput(int which)
{
  FilterMode m1,m2;
  decodeMode(which, &sequence_mode, &m1, &m2);

  filt.setOutput(m1);
  filt2.setOutput(m2);

}

void OversampledDistortionFilter::decodeMode(int which, FilterSequence * seq, FilterMode * m1, FilterMode * m2)
{
  switch(which)
    {
    case Lowpass:
    case Highpass:
    case Bandpass:
    case Notch:
      // use filt 2 for these
      *m2 = (FilterMode)(which);		
      *seq = Single;
      break;

    case Double_LP:
    case Double_HP:
    case Double_BP:
    case Double_Notch:
      which -= Double_LP;	// de-offset into lowpass
      *m1 = (FilterMode)(which);
      *m2 = (FilterMode)(which);
      *seq = Serial;
      break;

    case Parallel_LP_HP:
      *m1 = (Lowpass);
      *m2 = (Highpass);
      *seq = Parallel;
      break;
    case Parallel_LP_BP:
      *m1 =(Lowpass);
      *m2 =(Bandpass);
      *seq = Parallel;
      break;
    case Parallel_LP_Notch:
      *m1 =(Lowpass);
      *m2 =(Notch);
      *seq = Parallel;
      break;
    case Parallel_HP_BP:
      *m1 =(Highpass);
      *m2 =(Bandpass);
      *seq = Parallel;
      break;
    case Parallel_HP_Notch:
      *m1 =(Highpass);
      *m2 =(Notch);
      *seq = Parallel;
      break;
    case Parallel_BP_Notch:
      *m1 =(Bandpass);
      *m2 =(Notch);
      *seq = Parallel;
      break;

      // do two passes for separate and wide
      // first pass, set the first filter
    case Separate_LP_LP:
    case Separate_LP_HP:
    case Separate_LP_BP:
    case Separate_LP_Notch:
      *m1 =(Lowpass);
      *seq = Separate;
      break;

    case Separate_HP_LP:
    case Separate_HP_HP:
    case Separate_HP_BP:
    case Separate_HP_Notch:
      *m1 =(Highpass);
      *seq = Separate;
      break;

    case Separate_BP_LP:
    case Separate_BP_HP:
    case Separate_BP_BP:
    case Separate_BP_Notch:
      *m1 =(Bandpass);
      *seq = Separate;
      break;

      // wide
    case Wide_LP_LP:
    case Wide_LP_HP:
    case Wide_LP_BP:
    case Wide_LP_Notch:
      *m1 =(Lowpass);
      *seq = Wide;
      break;

    case Wide_HP_LP:
    case Wide_HP_HP:
    case Wide_HP_BP:
    case Wide_HP_Notch:
      *m1 =(Highpass);
      *seq = Wide;
      break;

    case Wide_BP_LP:
    case Wide_BP_HP:
    case Wide_BP_BP:
    case Wide_BP_Notch:
      *m1 = (Bandpass);
      *seq = Wide;
      break;

    case NoFilter:
      *m1 = (Lowpass);
      *m2 = (Lowpass);
      *seq = None;		
    }

  // second pass for wide and separate
  switch(which)
    {
    case Separate_LP_LP:				
    case Separate_HP_LP:	
    case Separate_BP_LP:	
    case Wide_LP_LP:	
    case Wide_HP_LP:	
    case Wide_BP_LP:
      *m2 =(Lowpass);
      break;

    case Separate_LP_HP:
    case Separate_HP_HP:
    case Separate_BP_HP:
    case Wide_LP_HP:
    case Wide_HP_HP:
    case Wide_BP_HP:
      *m2 =(Highpass);
      break;

    case Separate_LP_BP:
    case Separate_HP_BP:
    case Separate_BP_BP:
    case Wide_LP_BP:
    case Wide_HP_BP:
    case Wide_BP_BP:
      *m2 =(Bandpass);
      break;

    case Separate_LP_Notch:
    case Separate_HP_Notch:
    case Separate_BP_Notch:
    case Wide_LP_Notch:
    case Wide_HP_Notch:
    case Wide_BP_Notch:
      *m2 =(Notch);
      break;
    }

}
