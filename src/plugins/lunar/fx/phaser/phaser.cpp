#include "phaser.h"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>
#define WAVESIZE 4096
#define WAVEMASK 4095
#define STAGES 65
// retrieve sample with linear interpolation
inline float get_interpolated_sample(float *samples, float phase, int mask)
{
	int pos = (int)floor(phase);
	float frac = phase - (float) pos;
	float out = samples[pos & mask];
	pos++;
	return out + frac * (samples[pos & mask] - out);
}

inline float float_mask(float phase, int mask)
{
	int pos = (int)floor(phase);
	float frac = phase - (float) pos;
	return (float) (pos & mask) + frac;
}

struct phaser_state {
	float zm0, zm1, zm2, zm3, zm4, zm5, zm6;
	float zm[STAGES];
	float lfo_phase;
	float swirl;
	float init_feedback, feedback, drywet, n_drywet, a;
	int stages;
	phaser_state() {
		reset();
		lfo_phase = 0;
	}
	
	void reset() {
		for (int i = 0; i <= STAGES; i++)
			zm[i] = 0.0f;
	}
	
	inline void set_delay(float value)
	{
		if (value < 0.0f)
			value = 0.0f;
		if (value > 1.0f)
			value = 1.0f;
		a = (1.0f - value) / (1.0f + value);
	}
	
	inline float tick(float in)
	{
		float y;
		float out = in + zm[0] * feedback;
		for (int i = stages; i > 0; i--) {
			y = zm[i] - out*a;
			zm[i] = y * a + out;
			out = y;
		}
		zm[0] = y;
		//  feedback damping to prevent spiking
		if (abs(zm[0]) > 5) {
			feedback = 0.95*feedback;
		}
		return n_drywet * in + y * drywet;
	}
};

class phaser : public lunar::fx<phaser> {
public:
	float wavetable[WAVESIZE];
	float lfo_min;
	float lfo_max;
	float lfo_increment;
	phaser_state phaser_left, phaser_right;
	
	void init() {
		// wavetable for lfo sine
		for (int i = 0; i < WAVESIZE; i++) {
			float t = (float) i / (float) WAVESIZE;
			wavetable[i] = (float) sin(2 * M_PI * t);
		}
	}
	void exit() {
	}

	void process_events() {
		phaser_left.swirl = 0;
		phaser_right.swirl = 20;
		phaser_left.feedback = phaser_left.init_feedback;
		phaser_right.feedback = phaser_right.init_feedback;
		if (globals->feedback) {
			phaser_left.feedback = *globals->feedback;
			phaser_right.feedback = *globals->feedback;
			phaser_left.init_feedback = phaser_left.feedback;
			phaser_right.init_feedback = phaser_right.feedback;
		}
		if (globals->drywet) {
			phaser_left.drywet = *globals->drywet;
			phaser_left.n_drywet = 1 - phaser_left.drywet;
			phaser_right.drywet = *globals->drywet;
			phaser_right.n_drywet = 1 - phaser_right.drywet;	
		}
		if (globals->lfo_min)
			lfo_min = *globals->lfo_min / transport->samples_per_second;
		if (globals->lfo_max)
			lfo_max =  *globals->lfo_max / transport->samples_per_second;
		if (globals->lfo_rate)
			lfo_increment = WAVESIZE * (*globals->lfo_rate / transport->samples_per_second);
		if (globals->stages) {
			phaser_left.stages = (int)*globals->stages;
			phaser_right.stages = (int)*globals->stages;
			phaser_left.reset();
			phaser_right.reset();
		}
		if (globals->lfo_phase) {
			phaser_left.lfo_phase = *globals->lfo_phase*WAVESIZE;
			phaser_right.lfo_phase = *globals->lfo_phase*WAVESIZE;
		}
	}

	void process(float *buffer, int size, phaser_state *phaser) {
		float in, out, delay;
		while (size--) {
			delay = lfo_min + (lfo_max - lfo_min) *  (1.0 + get_interpolated_sample(wavetable, phaser->lfo_phase, WAVEMASK)) / 2;
			phaser->set_delay(delay);
			//phaser->lfo_phase = float_mask(phaser->lfo_phase + lfo_increment * phaser->swirl, WAVEMASK);
			phaser->lfo_phase = float_mask(phaser->lfo_phase + lfo_increment, WAVEMASK);
			in = *buffer;
			out = phaser->tick(in);
			*buffer++ = out;
		}
	}

	void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {
		dsp_copy(inL, outL, n);
		dsp_copy(inR, outR, n);
		process(outL, n, &phaser_left);
		process(outR, n, &phaser_right);
		dsp_clip(outL, n, 1); // signal may never exceed -1..1
		dsp_clip(outR, n, 1);
	}
};

lunar_fx *new_fx() { return new phaser(); }
