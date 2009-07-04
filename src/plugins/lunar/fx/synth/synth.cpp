#include "synth.h"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>

#include "svf.h"
#include "adsr.h"

#define MAX_TRACKS 8
#define MAX_WAVES 16
#define TABSIZE 4096
#define POWTABSIZE 16384
#define OCTAVES 10

class BLOsc {
private:
  float oct[OCTAVES];
  float saw[OCTAVES][TABSIZE];
  float sqr[OCTAVES][TABSIZE];
  float srate;
  float phi;
  float mu;
  int wave;

  inline float cubic(float y0, float y1, float y2, float y3, float mu) {
    // Cubic interpolation.
    float a0, a1, a2, a3, mu2;
    mu2 = mu * mu;
    a0 = y3 - y2 - y0 + y1;
    a1 = y0 - y1 - a0;
    a2 = y2 - y0;
    a3 = y1;
    return (a0 * mu * mu2 + a1 * mu2 + a2 * mu + a3);
  }

public:
  BLOsc(float srate) {
    phi = 0.0;
    mu = 0.0;
    wave = 0;
    float cfreq = 32.703;
    for (int i = 0; i < OCTAVES; i++) {
      this->oct[i] = cfreq;
      cfreq *= 2;
    }
    // Sampling rate.
    this->srate = srate;
    // Each octave get's a bandlimited table.
    for (int i = 0; i < OCTAVES; i++) {
      // Frequency of the C note for the octave.
      float curfreq = oct[i];
      // Index of harmonic partial.
      int partial = 1;

      // These will be later used to reduce the amplitude of the table.
      float saw_divisor = 0.0;
      float sqr_divisor = 0.0;
      // Partial weight.
      float mult = 1.0 / (float)partial;
      // While the frequency is less then the nyquist frequency.
      float nyquist = this->srate * 0.5 * 0.80;
      while (curfreq < nyquist) {
	// Add a sine wave for the corresponding partial to the table.
	for (int j = 0; j < TABSIZE; j++) {
	  // Sine phase.
	  float phase = (float)j / (float)TABSIZE;
	  // Saw gets every harmonic,
	  saw[i][j] += sin(2.0 * M_PI * (float)partial * phase) * mult;
	  // while square gets only odd ones.
	  if (partial % 2 != 0) {
	    sqr[i][j] += sin(2.0 * M_PI * (float)partial * phase) * mult;
	  }
	}
	if (partial % 2 != 0)
	  sqr_divisor += mult;
	saw_divisor += mult;
	// Increment frequency to the next harmonic.
	curfreq += oct[i];
	// Increment partial index.
	partial += 1;
      }
      // Scale table to (-1.0, 1.0) range.
      for (int j = 0; j < TABSIZE; j++) {
	saw[i][j] /= saw_divisor;
	sqr[i][j] /= sqr_divisor;
      }
    }
  }

  inline float cubic_lookup(float *tab, int tabsize, float phi) {
    float indexf = (tabsize * phi);
    int index = (int)floor(indexf);
    float mu = indexf - floor(indexf);
    int i0 = (index - 1) < 0 ? tabsize - 1 : index - 1;
    float result = 
      cubic(tab[i0],  tab[index % tabsize], tab[(index + 1) % tabsize], 
	    tab[(index + 2) % tabsize], mu);
    return result;
  }

  void set_wave(int wave) {
    if (wave == 0) {
      this->wave = 0;
    } else {
      this->wave = 1;
    }
  }

  void play(float *out, float *freq, int n) {
    // For each sample of output ...
    for (int i = 0; i < n; i++) {
      // This is the increment by which the oscillator phase is changing.
      float dphi = freq[i] / this->srate;

      // First find the table just below the frequency we need.
      int wave = 0;
      for (int j = 1; j < OCTAVES; j++) {
	if (oct[j] > freq[i]) {
	  wave = j - 1;
	  break;
	}
      }

      // Where between the two tables are we frequency-wise?
      float diff = oct[wave + 1] - oct[wave];
      // Amount of the lower frequency table influence.
      float bal1 = 1.0 - (freq[i] - oct[wave]) / diff;
      // Amount of the higher frequency table influence.
      float bal2 = 1.0 - bal1;

      // Update the samples in cubic interpolation array.
      if (this->wave == 0) {
	out[i] = 
	  bal1 * cubic_lookup(saw[wave], TABSIZE, phi) +
	  bal2 * cubic_lookup(saw[wave + 1], TABSIZE, phi);
      } else {
	out[i] = 
	  bal1 * cubic_lookup(sqr[wave], TABSIZE, phi) +
	  bal2 * cubic_lookup(sqr[wave + 1], TABSIZE, phi);
      }
      // Increment oscillator phase, advancing the sound wave.
      this->phi += dphi;
      while (phi > 1.0)
	phi -= 1.0;
    }
  }
};

class Inertia {
private:
  float y, d;
  int delay;
public:
  Inertia() {
    y = 0.0;
    d = 0.0;
    delay = 0;
  }

  void set_target(float target, int delay) {
    this->delay = delay;
    this->d = (target - this->y) / (float)delay;
  }

  void play(float *out, int n) {
    for (int i = 0; i < n; i++) {
      out[i] = y;
      if (delay) {
	y += d;
	delay -= 1;
      }
    }
  }
};

#define AMP_SUSTAIN 0.70
#define FLT_SUSTAIN 0.25
#define MAX_RESO 1.4
#define MIN_DIST 0.5
#define MAX_DIST 3.5
#define ACC_VOLUME 0.5
#define ACC_CUTOFF 1.0

class synth : public lunar::fx<synth> {
public:
  float attack, decay;
  float scutoff, sreso, scutoff2, dist;
  float amp, oct;
  float pitchslide;
  float accent, acc_cutoff, acc_reso, acc_attack;
  BLOsc *osc;
  Inertia *freq, *cutoff;
  LPF18 *filter;
  adsr *envelope, *fenv;

  void init() {
    amp = 1.0f;
    this->osc = new BLOsc(transport->samples_per_second);
    this->freq = new Inertia();
    this->cutoff = new Inertia();
    this->filter = new LPF18((float)transport->samples_per_second);
    this->envelope = new adsr(transport->samples_per_second);
    this->fenv = new adsr(transport->samples_per_second);
  }

  void exit() {
    delete this->osc;
    delete this->freq;
    delete this->cutoff;
    delete this->filter;
    delete this->envelope;
    delete this->fenv;
  }

  void process_events() {
    int update_adsr = 0;
    if (globals->attack) {
      attack = *globals->attack / 1000.0;
    }
    if (globals->decay) {
      decay = *globals->decay / 1000.0;
    }
    if (globals->freq) {
      scutoff = *globals->freq;
    }
    if (globals->res) {
      sreso = *globals->res;
      this->filter->set_res(sreso * MAX_RESO);
    }
    if (globals->accent) {
      this->accent = *globals->accent;
    }
    if (globals->cutoff) {
      this->cutoff->set_target(*globals->cutoff, transport->samples_per_tick);
    }
    if (globals->dist) {
      this->dist = *globals->dist;
      this->filter->set_dist(MIN_DIST + *globals->dist * MAX_DIST);
    }
    if (globals->amp) {
      this->amp = dbtoamp(*globals->amp, -48);
    }
    if (globals->waveform) {
      this->osc->set_wave(*globals->waveform);
    }
    if (globals->pitchslide) {
      this->pitchslide = *globals->pitchslide;
    }
    if (globals->oct) {
      this->oct = *globals->oct;
    }
    if (tracks[0].note) {
      if (*tracks[0].note == 0.0 && tracks[0].slide && *tracks[0].slide) {
	  this->envelope->off();
	  this->fenv->off();
      } else {
	if (tracks[0].slide && *tracks[0].slide)
	  ;
	else {
	  // Check if the accent tick was checked in this step,
	  if (tracks[0].accent) {
	    // if it was checked then increase filter and amplitude envelope
	    // multipliers,
	    this->envelope->on(0.001, this->decay,
			       AMP_SUSTAIN, 0.001,
			       1.0 + this->accent * ACC_VOLUME);
	    this->fenv->on(this->attack * (1.0 - this->accent), this->decay,
			   FLT_SUSTAIN, 0.001, 1.0 + this->accent * ACC_CUTOFF);
	  } else {
	    // otherwise set those multipliers to 1.0
	    this->envelope->on(0.001, this->decay, 
			       AMP_SUSTAIN, 0.001, 1.0);
	    this->fenv->on(this->attack, this->decay, 
			   FLT_SUSTAIN, 0.001, 1.0);
	  }
	}
	int delay = (int)(this->pitchslide / 1000.0 * 
			  transport->samples_per_second);
	if (tracks[0].slide && *tracks[0].slide) {
	  this->freq->set_target(*tracks[0].note, delay);
	} else {
	  this->freq->set_target(*tracks[0].note, 8);
	}
      }
    }
  }

  void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {
    float *glide = new float[n];
    float *cutoff = new float[n];
    float *envelope = new float[n];
    float *fenv = new float[n];
    this->freq->play(glide, n);
    for (int i = 0; i < n; i++) {
      if (this->oct > 0.0) {
	glide[i] *= 1.0 + this->oct;
      } else if (this->oct < 0.0) {
	glide[i] *= 1.0 / (1.0 + -this->oct);
      }
    }
    this->cutoff->play(cutoff, n);
    this->osc->play(outL, glide, n);
    this->envelope->play(envelope, n);
    this->fenv->play(fenv, n);
    for (int i = 0; i < n; i++) {
      outL[i] *= envelope[i];
      cutoff[i] *= 
	(1.0 - this->scutoff) + (fenv[i] * this->scutoff);
      outL[i] *= this->amp;
    }
    this->filter->play(outL, cutoff, outL, n);
    dsp_clip(outL, n, 1);
    dsp_copy(outL, outR, n);
    delete glide;
    delete cutoff;
    delete envelope;
    delete fenv;
  }
};

lunar_fx *new_fx() { return new synth(); }
