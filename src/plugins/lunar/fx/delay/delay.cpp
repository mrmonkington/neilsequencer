
#include "delay.h"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>

#define MAX_DELAY_LENGTH 192000 // in samples

class delay : public lunar::fx<delay> {
public:
	struct ringbuffer_t {
		float buffer[MAX_DELAY_LENGTH]; // ringbuffer
		float *eob; // end of buffer
		float *pos; // buffer position
	};

	ringbuffer_t rb[2];
	float wet;
	float dry;
	float fb;

	void rb_init(ringbuffer_t *rb) {
		dsp_zero(rb->buffer, MAX_DELAY_LENGTH);
		rb->eob = rb->buffer + 1;
		rb->pos = rb->buffer;
	}

	void rb_setup(ringbuffer_t *rb, int size) {
		rb->eob = rb->buffer + size;
		while (rb->pos >= rb->eob)
			rb->pos -= size;
	}

	void rb_mix(ringbuffer_t *rb, float *out, int n) {
		float s;
		while (n--) {
			s = *out;
			*out = (*out * dry) + (*rb->pos * wet);
			*rb->pos = min(max((*rb->pos + s) * fb, -1.0f),1.0f);
			out++;
			rb->pos++;
			if (rb->pos == rb->eob) // wrap
				rb->pos = rb->buffer;
		}
	}

	void init() {
		rb_init(&rb[0]);
		rb_init(&rb[1]);
		wet = 0.0f;
		dry = 0.0f;
		fb = 0.0f;
	}

	void exit() {
	}

	void process_events() {
		float delay_in_beats;
		int rbsize;
		
		if (globals->delay) {
			delay_in_beats = *globals->delay / 1024.0f;
			rbsize = (int)min(transport->samples_per_tick * transport->ticks_per_beat * delay_in_beats + 0.5f, MAX_DELAY_LENGTH);
			printf("delay = %f, rbsize = %i\n", delay_in_beats, rbsize);
			rb_setup(&rb[0], rbsize);
			rb_setup(&rb[1], rbsize);
		}
		if (globals->wet) {
			wet = dbtoamp(*globals->wet, -48.0f);
		}
		if (globals->dry) {
			dry = dbtoamp(*globals->dry, -48.0f);
		}
		if (globals->fb) {
			fb = dbtoamp(*globals->fb, -48.0f);
		}
	}

	void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {
		dsp_copy(inL, outL, n);
		dsp_copy(inR, outR, n);
		rb_mix(&rb[0], outL, n);
		rb_mix(&rb[1], outR, n);
		dsp_clip(outL, n, 1); // signal may never exceed -1..1
		dsp_clip(outR, n, 1);
	}

};

lunar_fx *new_fx() { return new delay(); }
