/*
  kibibu Green Milk
  Filter classes
  Uses adapted State Variable Filter and Decimateur5 classes
  from music-dsp

  4x oversampled filters with pre- and post-distortion.

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

#pragma once

#include <string>

enum FilterSequence
  {
    Single,
    Serial,
    Parallel,
    Separate,
    Wide,
    None
  };


enum FilterMode
  {
    Lowpass,
    Highpass,
    Bandpass,
    Notch,

    Double_LP,
    Double_HP,
    Double_BP,
    Double_Notch,

    Parallel_LP_HP,
    Parallel_LP_BP,
    Parallel_LP_Notch,
    Parallel_HP_BP,
    Parallel_HP_Notch,
    Parallel_BP_Notch,

    Separate_LP_LP,
    Separate_LP_HP,
    Separate_LP_BP,
    Separate_LP_Notch,

    Separate_HP_LP,
    Separate_HP_HP,
    Separate_HP_BP,
    Separate_HP_Notch,

    Separate_BP_LP,
    Separate_BP_HP,
    Separate_BP_BP,
    Separate_BP_Notch,

    Wide_LP_LP,
    Wide_LP_HP,
    Wide_LP_BP,
    Wide_LP_Notch,

    Wide_HP_LP,
    Wide_HP_HP,
    Wide_HP_BP,
    Wide_HP_Notch,

    Wide_BP_LP,
    Wide_BP_HP,
    Wide_BP_BP,
    Wide_BP_Notch,

    NoFilter

  };

class Halfband
{
public:
  static const int num_taps = 6;

  void Init()
  {
    // init taps
    for(int i = 0; i < Halfband::num_taps; ++i)
      {
	taps[i] = 0.0f;
      }
  }
	
  Halfband()
  {
    Init();
  }

private:
  float taps[num_taps];
	
public:
  inline float filter(const float value)
  {
    // basically windowed sinc
    float out = 
      (value + taps[5] ) * -0.110433221f +
      (taps[1] + taps[3]) *  0.331299664f +
      taps[2] * 0.520404296f;

    // and shuffle
    for(int i = Halfband::num_taps - 1; i > 0; i--)
      {
	taps[i] = taps[i - 1];
      }
    taps[0] = value;

    return out;
  }
};

class QuarterBand
{
public:
  static const int num_taps = 10;

  void Init()
  {
    // init taps
    for(int i = 0; i < QuarterBand::num_taps; ++i)
      {
	taps[i] = 0.0f;
      }
  }

	
  QuarterBand()
  {
    Init();		
  }

private:
  float taps[num_taps];
	
public:
  inline float filter(const float value)
  {
    // coeffs from Scope-FIR
    float out = 
      (value + taps[9]) * -0.049572657f +
      (taps[1] + taps[7]) * -0.082621095f +
      (taps[2] + taps[6]) * 0.17526581f +
      (taps[3] + taps[5]) * 0.247863285f + 
      (taps[4] * 0.27530689f);

    // and shuffle
    for(int i = QuarterBand::num_taps - 1; i > 0; i--)
      {
	taps[i] = taps[i - 1];
      }
    taps[0] = value;

    return out;
    // return value;
  }
};

// gives 4 samples when you provide 1
// uses straight up lerp
class Upsampler4
{
public:
  inline void feed(const float& in) {inp = in;};
  inline float next() { float nx = qb.filter(inp); inp = 0.0f; return nx; };
  // inline float next() { return inp; };

  void Init()
  {
    qb.Init();
    inp = 0.0f;
  }
  Upsampler4() {Init();};

private:
  float inp;
  QuarterBand qb;

};


class Decimateur5
{
private:
  float R0,R1,R2,R3,R4,R5;
public:

  void Init()
  {
    R0=R1=R2=R3=R4=R5=0.0f;
  }

  Decimateur5()
  {
    Init();
  }

  float Calc(const float x0,const float x1)
  {
    float out;
		
    out = 
      (x1 + R5 ) * -0.110433221f +
      (R1 + R3) *  0.331299664f +
      R2 * 0.520404296f;

    R5 = R3;
    // R4 = R2; R4 is never used again
    R3 = R1;
    R2 = R0;
    R1 = x0;
    R0 = x1;

    return (out);
  }
};

// State Variable Filter
class SVFilter
{
public:

  void setResonance(float f);
  void setFrequency(float f, int sampRate);
	
  void filter(float * pSamples, int numSamples);
	
  inline float filterSample(const float& in);
	
  void setOutput(int which);
	
  float lowpass, highpass, bandpass, notch;
  float * out;

private:
  float cutoff;
  float resonance;

  float internal_cutoff;
  float damp;

  inline float svmin(const float f1, const float f2)
  {
    return (f1>f2?f2:f1);
  }

public:
  void Init()
  {
    lowpass = highpass = bandpass = notch = 0.0f;
		
  }

  SVFilter()
  {
    Init();

    out = &lowpass;

    setResonance(0.1f);
    setFrequency(1.0f, 2);
  }

  void copy(SVFilter & other)
  {
    cutoff = other.cutoff;
    resonance = other.resonance;
    internal_cutoff = other.internal_cutoff;
    damp = other.damp;
  }

};


class OversampledDistortionFilter
{
public:
	
  float preDist;
  float postDist;

  void setResonance(float f) {filt.setResonance(f); filt2.setResonance(f);};
  void setFrequency(float f, int sampRate);
	
  void filter(float * pSamples, int numSamples);	
	
  bool high;

  void setOutput(int which);

  static const int numOutputs = 39;
  static std::string describeOutput(int which);

  void Init()
  {	
    decimator.Init();
    decimator2.Init();
    upper.Init();

    filt.Init();
    filt2.Init();

  }

  OversampledDistortionFilter()
  {
    preDist = 0.1f;
    postDist = 0.5f;
		
    high = true;

    Init();
  }

private:
  SVFilter filt;
  SVFilter filt2;

  FilterSequence sequence_mode;

  Decimateur5 decimator;
  Decimateur5 decimator2;
  Upsampler4 upper;

  static std::string describeFilterMode(FilterMode fm);

  static void decodeMode(int which, FilterSequence * seq, FilterMode * pM1, FilterMode * pM2);

  inline float filterSample(const float& in);
  inline float filterSampleNoDist(const float& in);

  inline float filterSampleSerial(const float& in);
  inline float filterSampleParallel(const float& in);

  inline float distortOnly(const float & in);

  inline float distortSample(const float& dist, const float& in);

};
