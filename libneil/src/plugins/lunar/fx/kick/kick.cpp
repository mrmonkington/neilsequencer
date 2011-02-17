
#include "kick.h"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>

#include "adsr.h"

#define MAX_TRACKS 1
#define TABSIZE 2048

class kick : public lunar::fx<kick> {
public:
	float sinetab[TABSIZE];
	float lattack, ldecay, lsustain, lrelease;
	float sfreq, efreq;
	float vdecay;
	float power;
	float amp;

	struct voice {
		float *tab;
		float powtab[TABSIZE];
		float sfreq;
		float efreq;
		float delta;
		float phase;
		float x;
		float speed;
		float sps;
		float power;
		float volume;
		adsr a;
		
		inline void set_powtab(float sfreq, float efreq, float power) {
			if ((this->sfreq == sfreq) && (this->power == power) && (this->efreq == efreq))
				return;
			this->sfreq = sfreq;
			this->efreq = efreq;
			this->power = power;
			float delta = efreq - sfreq;
			for (int i = 0; i < TABSIZE; ++i) {
				powtab[i] = exp(sfreq + delta*pow(float(i)/(float(TABSIZE-1)),power)) * sps;
			}
		}
		
		voice() {
			sfreq = -1.0f;
			efreq = -1.0f;
			power = -1.0f;
			volume = 1.0f;
			speed = 1.0/44100.0;
		}
		
		inline bool process(float *b, int n) {
			if (a.state == adsr::state_off)
				return false;
			int ns = n;
			float *pb = b;
			while (ns--) {
				*pb++ = tab[int(phase*float(TABSIZE))%TABSIZE] * volume;
				phase += powtab[int(x*float(TABSIZE-1))];
				x += speed;
				if (x > 1)
					x = 1;
			}
			a.process_amp(b, n);
			return true;
		}

	};

	voice voices[MAX_TRACKS];
	float trackbuf[256];

	void init() {
		amp = 1.0f;
		for (int i = 0; i < TABSIZE; i++) {
			sinetab[i] = sin(float(i)*2*M_PI/float(TABSIZE));
		}
		for (int v = 0; v < MAX_TRACKS; v++) {
			voices[v].tab = sinetab;
		}
	}

	void process_events() {
		if (globals->attack) {
			lattack = *globals->attack;
		}
		if (globals->decay) {
			ldecay = *globals->decay;
		}
		if (globals->sustain) {
			lsustain = *globals->sustain;
		}
		if (globals->release) {
			lrelease = *globals->release;
		}
		if (globals->startfreq) {
			sfreq = *globals->startfreq;
		}
		if (globals->endfreq) {
			efreq = *globals->endfreq;
		}
		if (globals->vdecay) {
			vdecay = *globals->vdecay;
		}
		if (globals->power) {
			power = *globals->power;
		}
		if (globals->amp) {
			amp = dbtoamp(*globals->amp,-48);
		}

		for (int t = 0; t < track_count; ++t) {
			if (tracks[t].trigger) {
				if (*tracks[t].trigger == 1) {
					voices[t].a.on();
					voices[t].x = 0;
					voices[t].phase = 0;
					voices[t].volume = amp;
					voices[t].speed = 1000.0/(vdecay * transport->samples_per_second); // 1s
					voices[t].sps = 1.0/transport->samples_per_second;
					voices[t].a.setup(transport->samples_per_second, lattack / 1000.0f, ldecay / 1000.0f, lsustain / 100.0f, lrelease / 1000.0f, 9999.0f);
					voices[t].set_powtab(log(sfreq), log(efreq), power);
				} else {
					voices[t].a.off();
				}
			}
			if (tracks[t].volume) {
				voices[t].volume = amp * (float(*tracks[t].volume) / 128.0f);
			}
		}
		
	}

	void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {
		dsp_zero(outL, n);
		for (int t = 0; t < track_count; ++t) {
			if (voices[t].process(trackbuf, n))
				dsp_add(trackbuf,outL,n);
		}
		dsp_copy(outL, outR, n);
	}
};

lunar_fx *new_fx() { return new kick(); }
