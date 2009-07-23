#include "LunarGranulizer.hpp"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>

#define MAX_LENGTH 8192
#define MAX_GRAINS 128

#define STATUS_SAMPLING 1
#define STATUS_PLAYING 2
#define STATUS_READY 3

class Grain {
private:
  int status, sampling_length, delay, temp_length;
  float samplesL[MAX_LENGTH];
  float samplesR[MAX_LENGTH];
  float envelope[MAX_LENGTH];
  float freq_env[MAX_LENGTH];
  float mul, counter, amp;

  void calc_envelope(float off, float sus, 
		     float pstart, float pend) {
    for (int i = 0; i < MAX_LENGTH; i++) {
      envelope[i] = 0.0;
      freq_env[i] = 1.0;
    }

    int attack = (int)(sampling_length * off);
    int decay = (int)(sampling_length * (1.0 - sus - off));
    int sustain = (int)(sampling_length * sus);

    float attack_incr, decay_incr, env;
    attack_incr = 1.0 / attack; // Attack increment step.
    decay_incr = 1.0 / decay; // Decay increment step;
    env = 0.0; // Starting envelope value.
    float penv = pstart;
    float pincr = (pend - pstart) / (float)sampling_length;

    for (int i = 0; i < sampling_length; i++) {
      if (i < attack) {
	// If envelope is in attack stage increment by attack_incr.
	envelope[i] = env;
	env += attack_incr;
      } else if (i > attack + sustain) {
	// If envelope is in decay stage decrement by decay_incr.
	envelope[i] = env;
	env -= decay_incr;
      } else {
	// Otherwise it means we are in sustain stage assign 1.0.
	envelope[i] = 1.0;
      }
      freq_env[i] = penv;
      penv += pincr;
    }
  }

public:
  Grain() {
    counter = 0.0;
    sampling_length = 1024;
    for (int i = 0; i < MAX_LENGTH; i++) {
      samplesL[i] = 0.0;
      samplesR[i] = 0.0;
    }
    mul = 1.0;
    status = STATUS_READY;
  }

  void setMul(float mul) {
    this->mul = mul;
  }

  int ready() {
    return (status == STATUS_READY);
  }

  void startSampling(int length) {
    if (status != STATUS_READY) {
      return;
    }
    else {
      status = STATUS_SAMPLING;
      this->sampling_length = length;
    }
  }

  void startPlaying(int delay, float attack, float sustain, 
		    float pstart, float pend, float amp, int length) {
    if (status != STATUS_READY) {
      return;
    }
    else {
      calc_envelope(attack, sustain, pstart, pend);
      status = STATUS_PLAYING;
      this->delay = delay;
      this->amp = amp;
      this->temp_length = length;
    }
  }

  void process(float *inL, float *inR, float *outL, float *outR, int n) {
    for (int i = 0; i < n; i++) {
      switch (status) {
      case STATUS_SAMPLING:
	if (counter >= (float)sampling_length) {
	  status = STATUS_READY;
	  counter = 0.0;
	} else {
	  samplesL[(int)counter] = inL[i];
	  samplesR[(int)counter] = inR[i];
	  counter += 1.0;
	}
	break;
      case STATUS_PLAYING:
	if (delay-- <= 0) {
	  if (counter >= (float)sampling_length) {
	    status = STATUS_SAMPLING;
	    this->sampling_length = temp_length;
	    counter = 0.0;
	  } else {
	    outL[i] += (amp * 
			samplesL[(int)counter] * 
			envelope[(int)counter]);
	    outR[i] += (amp * 
			samplesR[(int)counter] *
			envelope[(int)counter]);
	    counter += freq_env[i];
	  }
	  break;
	}
      case STATUS_READY:
	break;
      }
    }
  }
};

class LunarGranulizer : public lunar::fx<LunarGranulizer> {
private:
  int seed, number_of_grains, grain_length;
  float min_delay, max_delay, min_amp, max_amp,
    attack, sustain, pstart, pend, feedback, dry_wet;
  float *fleft, *fright;
  Grain grains[MAX_GRAINS];

  int rand1(void) {
    seed = seed * 1103515245 + 12345;
    return (unsigned int)(seed / 65536) % 32768;
  }
  
  float rnd() {
    return (float)rand1() / 32767.0;
  }

  float rnd_range(float min, float max) {
    if (min >= max) return min;
    float rmax = rnd() * (max - min);
    return rmax + min;
  }

  int random_event(float chance) {
    return (chance > rnd());
  }
  
public:
  void init() {
    seed = 42;
    fleft = new float[65536];
    fright = new float[65536];
    dsp_zero(fleft, 65536);
    dsp_zero(fright, 65536);
  }

  void exit() {
    delete fleft;
    delete fright;
  }

  void process_events() {
    if (globals->dry_wet) {
      this->dry_wet = *globals->dry_wet;
    }
    if (globals->number_of_grains) {
      this->number_of_grains = *globals->number_of_grains;
      for (int i = 0; i < this->number_of_grains; i++) {
	grains[i].setMul(1.0 / (float)number_of_grains);
      }
    }
    if (globals->grain_length) {
      this->grain_length = *globals->grain_length;
    }
    if (globals->attack) {
      this->attack = *globals->attack;
    }
    if (globals->sustain) {
      this->sustain = *globals->sustain;
    }
    if (globals->min_amp) {
      this->min_amp = *globals->min_amp;
    }
    if (globals->max_amp) {
      this->max_amp = *globals->max_amp;
    }
    if (globals->pstart) {
      this->pstart = *globals->pstart;
    }
    if (globals->pend) {
      this->pend = *globals->pend;
    }
    if (globals->min_delay) {
      this->min_delay = *globals->min_delay;
    }
    if (globals->max_delay) {
      this->max_delay = *globals->max_delay;
    }
    if (globals->feedback) {
      this->feedback = *globals->feedback;
    }
  }

  void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {
    float *dryL = new float[n];
    float *dryR = new float[n];

    dsp_zero(outL, n);
    dsp_zero(outR, n);

    // Add feedback to the signal.
    for (int i = 0; i < n; i++) {
      dryL[i] = inL[i];
      dryR[i] = inR[i];
      inL[i] += this->feedback * fleft[i];
      inR[i] += this->feedback * fright[i];
    }

    for (int i = 0; i < number_of_grains; i++) {
      if (grains[i].ready()) {
	int delay = rnd_range(min_delay, max_delay) *
	  transport->samples_per_second;
	float amp = rnd_range(min_amp, max_amp);
	grains[i].startPlaying(delay, attack, sustain, pstart, pend, amp, grain_length);
      }
      grains[i].process(inL, inR, outL, outR, n);
    }

    for (int i = 0; i < n; i++) {
      // Capture signal for feedback.
      fleft[i] = outL[i];
      fright[i] = outR[i];
      // Adjust for dry/wet.
      outL[i] = dry_wet * outL[i] + (1.0 - dry_wet) * dryL[i];
      outR[i] = dry_wet * outR[i] + (1.0 - dry_wet) * dryR[i];
    }

    dsp_clip(outL, n, 1.0);
    dsp_clip(outR, n, 1.0);

    delete dryL;
    delete dryR;
  }
};

lunar_fx *new_fx() {return new LunarGranulizer();}
