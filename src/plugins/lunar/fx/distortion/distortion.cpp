
#include "distortion.h"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>

const float ATAN_FACTOR = 1.0f/1.5707963167948966f;
#define TABSIZE 2048


class distortion : public lunar::fx<distortion> {
public:
	float lfo_phase;
	float lfo_amp;
	float lfo_speed;

	float preamp, postamp;
	int fxtype;

	void init() {
		lfo_phase = 0.0f;
		preamp = 1.0f;
		postamp = 1.0f;
		fxtype = 0;
	}

	void exit() {
		delete this;
	}

	void process_events() {
		if (globals->pregain) {
			preamp = dbtoamp(*globals->pregain, -48.0f);
		}
		if (globals->postgain) {
			postamp = dbtoamp(*globals->postgain, -48.0f);
		}
		if (globals->fxtype) {
			fxtype = (int)(*globals->fxtype);
		}
		if (globals->lfospeed) {
			lfo_speed = (*globals->lfospeed) / float(transport->samples_per_second);
		}
		if (globals->lfodepth) {
			if (*globals->lfodepth == -48.0f)
				lfo_amp = 0.0f;
			else
				lfo_amp = dbtoamp(*globals->lfodepth, -48.0f);
		}
		if (globals->lfophase) {
			lfo_phase = *globals->lfophase;
		}
	}

	void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {
		dsp_copy(inL, outL, n);
		dsp_copy(inR, outR, n);
		dsp_amp(outL, n, preamp);
		dsp_amp(outR, n, preamp);
		switch (fxtype)
		{
			case 0: // digital clipping
			{
				if (lfo_amp > 0.0f) {
					float *l = outL;
					float *r = outR;
					float phase = lfo_phase;
					for (int i = 0; i < n; ++i) {
						float dc = (sin(phase * 2 * M_PI)*0.5f + 0.5f) * lfo_amp;
						float minv = -1.0f + (lfo_amp - dc);
						float maxv = 1.0f - dc;
						if (minv > 0.0f)
							minv = 0.0f;
						if (maxv < 0.0f)
							maxv = 0.0f;
						*l = (*l < minv)?minv:((*l > maxv)?maxv:*l);
						*r = (*r < minv)?minv:((*r > maxv)?maxv:*r);
						phase += lfo_speed;
						l++; r++;
					}
					lfo_phase = phase;
					while (lfo_phase > 1.0f)
						lfo_phase -= 1.0f;
				} else {
					dsp_clip(outL, n, 1);
					dsp_clip(outR, n, 1);
				}
			} break;
			case 1: // atan clipping
			{
				float *l = outL;
				float *r = outR;
				int ns = n;
				while (ns--) {
					*l++ = atan(*l) * ATAN_FACTOR;
					*r++ = atan(*r) * ATAN_FACTOR;
				}
			} break;
			default:
				break;
		}
		dsp_amp(outL, n, postamp);
		dsp_amp(outR, n, postamp);
		dsp_clip(outL, n, 1); // signal may never exceed -1..1
		dsp_clip(outR, n, 1);
		//~ float oldiamp;

		//~ dsp_set(outL,n,famp);
		//~ dsp_set(outR,n,famp);
		//~ int sn = (int)min(abs((famp - iamp)/step),n);
		//~ if (sn > 1) {
			//~ oldiamp = iamp;
			//~ if (famp > iamp) {
				//~ iamp = dsp_slope(outL,sn,oldiamp,step);
				//~ iamp = dsp_slope(outR,sn,oldiamp,step);
			//~ } else {
				//~ iamp = dsp_slope(outL,sn,oldiamp,-step);
				//~ iamp = dsp_slope(outR,sn,oldiamp,-step);
			//~ }
		//~ }
		//~ dsp_mul(inL, outL, n);
		//~ dsp_mul(inR, outR, n);
		//~ dsp_clip(outL, n, 1); // signal may never exceed -1..1
		//~ dsp_clip(outR, n, 1);
	}

};

lunar_fx *new_fx() { return new distortion(); }
