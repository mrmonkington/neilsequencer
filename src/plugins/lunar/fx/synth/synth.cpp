
#include "synth.h"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>

#include "svf.h"
#include "adsr.h"

#define MAX_TRACKS 8
#define MAX_WAVES 16
#define TABSIZE 2048
#define POWTABSIZE 8192

class synth : public lunar::fx<synth> {
public:
	float sawtab[TABSIZE];

	float lattack, ldecay, lsustain, lrelease;
	float scutoff, sreso, scutoff2;
	float amp;

	struct voice {
		float powtab[POWTABSIZE];
		float phasestep;
		float phase;
		float freq;
		adsr a;
		svf s;
		float cutoff;
		float volume;
		
		inline void update_powtab(float scutoff) {
			float c = min(cutoff+freq,20000);
			for (int i = 0; i < POWTABSIZE; ++i) {
				powtab[i] = c * pow(2.0f, 1.0f + ((i/float(POWTABSIZE-1)) * scutoff));
			}
		}
		
		voice() {
			freq = 0.0f;
			phasestep = 0.0f;
			phase = 0.0f;
			cutoff = 0.0f;
			volume = 1.0f;
		}
	};

	voice voices[MAX_TRACKS];

	void update_adsr_tracks() {
		int t;
		
		for (t = 0; t < MAX_TRACKS; t++) {
			voices[t].a.setup(transport->samples_per_second, lattack / 1000.0f, ldecay / 1000.0f, lsustain / 100.0f, lrelease / 1000.0f, 9999.0f);
		}
	}

	void update_svf_tracks() {
		int t;
		for (t = 0; t < MAX_TRACKS; t++) {
			voices[t].s.setup(transport->samples_per_second, scutoff, sreso, 0.0);
		}
	}

	void init() {
		int index, size, i, t;
		
		amp = 1.0f;
		
		for (i = 0; i < TABSIZE; ++i) {
			sawtab[i] = 1.0f - (float(i) / float(TABSIZE))*2.0f;
		}
	}

	void exit() {
		int t;
		
	}

	void process_events() {
		int update_adsr = 0;
		int update_svf = 0;
		
		if (globals->attack) {
			lattack = *globals->attack;
			update_adsr = 1;
		}
		if (globals->decay) {
			ldecay = *globals->decay;
			update_adsr = 1;
		}
		if (globals->sustain) {
			lsustain = *globals->sustain;
			update_adsr = 1;
		}
		if (globals->release) {
			lrelease = *globals->release;
			update_adsr = 1;
		}

		if (globals->freq) {
			scutoff = *globals->freq;
			update_svf = 1;
		}
		if (globals->res) {
			sreso = *globals->res;
			update_svf = 1;
		}
		if (globals->cutoff) {
			scutoff2 = *globals->cutoff;
		}
		if (globals->amp) {
			amp = dbtoamp(*globals->amp, -48);
		}
		
		if (update_adsr) {
			update_adsr_tracks();
		}
		if (update_svf) {
			update_svf_tracks();
		}
		
		for (int t = 0; t < track_count; ++t) {
			if (tracks[t].note) {
				if (*tracks[t].note == 0.0f) {
					voices[t].a.off();
				} else {
					voices[t].a.on();
					voices[t].volume = 1.0f;
					voices[t].freq = *tracks[t].note;
					voices[t].phasestep = ((*tracks[t].note)*float(TABSIZE)) / float(transport->samples_per_second);
					voices[t].phase = 0.0f;
					voices[t].cutoff = scutoff2;
					voices[t].update_powtab(scutoff * 2.0f);
				}
			}
			if (tracks[t].volume) {
				voices[t].volume = float(*tracks[t].volume) / 128.0f;
			}
		}
		
	}

	void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {
		voice *v; // active voice
		int slen; 
		
		dsp_zero(outL, n);
		for (int t = 0; t < track_count; ++t) {
			v = &voices[t];
			if (v->a.state != adsr::state_off) {
				int count = n;
				float *b = outL;
				float vol = v->volume;
				while (count--) {
					float adsr = v->a.process();
					float c = v->powtab[int(adsr * float(POWTABSIZE-1))];
					*b++ += v->s.envprocess(sawtab[int(v->phase)%TABSIZE],c,1) * vol * adsr;
					v->phase += v->phasestep;
					while (v->phase > TABSIZE)
						v->phase -= TABSIZE;
				}
			}
		}
		dsp_amp(outL, n, amp); 
		dsp_clip(outL, n, 1);
		dsp_copy(outL, outR, n);		
	}
};

lunar_fx *new_fx() { return new synth(); }
