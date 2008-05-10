
#include "synth.h"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>

#include "svf.h"
#include "adsr.h"

#define MAX_TRACKS 8
#define MAX_WAVES 16
#define TABSIZE 2048
//#define TABSIZE 8192
#define POWTABSIZE 8192

class synth : public lunar::fx<synth> {
public:
	float sawtab[TABSIZE];
	float sqrtab[TABSIZE];
	float sintab[TABSIZE];

	float lattack, ldecay, lsustain, lrelease;
	float scutoff, sreso, scutoff2;
	float amp;

struct lfo {
	float phase;
	float phasestep;
	float amp;

lfo() {
	phase = 0.0f;
	phasestep = 0.0f;
	amp = 0.0f;
	}
};

	struct voice {
		float powtab[POWTABSIZE];
		float start_phasestep;
		float end_phasestep;
		float slide_step;
		float slide_length;
		float phasestep;
		float phase;
		float freq;
		adsr a;
		svf s;
		float cutoff;
		float volume;
		float *curtab;
		lfo pitch;
		
		inline void update_powtab(float scutoff) {
			float c = min(cutoff+freq,20000);
			for (int i = 0; i < POWTABSIZE; ++i) {
				powtab[i] = c * pow(2.0f, 1.0f + ((i/float(POWTABSIZE-1)) * scutoff));
			}
		}
		
		voice() {
			freq = 0.0f;
			start_phasestep = 0.0f;
			end_phasestep = 0.0f;
			slide_step = 0.0f;
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

	void update_waveform_tracks(float *tab) {
		int t;
		for(t = 0; t < MAX_TRACKS; t++) {
			voices[t].curtab = tab;
		}
	}

	void init() {
		int index, size, i, t;
		
		amp = 1.0f;
		update_waveform_tracks(sawtab);
		
		for (i = 0; i < TABSIZE; ++i) {
			sawtab[i] = 1.0f - (float(i) / float(TABSIZE))*2.0f;
			sqrtab[i] = ((i < (TABSIZE / 2)) ? -1.0f : 1.0f );
			sintab[i] = sin(-M_PI + (M_PI * (float(i*2) / float(TABSIZE-1))));
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
		if ( globals->waveform ) {
			switch((int)*globals->waveform) {
			default:
			case 0:
				update_waveform_tracks(sawtab);
				break;
			case 1:
				update_waveform_tracks(sqrtab);
				break;
			case 2:
				update_waveform_tracks(sintab);
				break;
			}
		}
		
		if (update_adsr) {
			update_adsr_tracks();
		}
		if (update_svf) {
			update_svf_tracks();
		}
		
		for (int t = 0; t < track_count; ++t) {
			if (globals->plfospeed) {
				voices[t].pitch.phasestep = ((*globals->plfospeed)*float(TABSIZE)) / float(transport->samples_per_second);
			}
			if (globals->plfodepth) {
				voices[t].pitch.amp = (*globals->plfodepth) * 12.0f / 100.0f;
			}
			if (globals->pitchslide) {
				voices[t].slide_length = (*globals->pitchslide) / 1000.0f;
				voices[t].slide_step = 1 / voices[t].slide_length * 
						(voices[t].end_phasestep - voices[t].start_phasestep) / float(transport->samples_per_second);
			}
			if (tracks[t].note) {
				if (*tracks[t].note == 0.0f) {
					voices[t].a.off();
				} else {
					voices[t].a.on();
					voices[t].volume = 1.0f;
					voices[t].freq = *tracks[t].note;
					voices[t].start_phasestep = voices[t].phasestep;
					voices[t].end_phasestep = ((*tracks[t].note)*float(TABSIZE)) / float(transport->samples_per_second);
					// no sliding at start
					if (voices[t].phasestep == 0.0f) {
						voices[t].start_phasestep = voices[t].end_phasestep;
						voices[t].phasestep = voices[t].end_phasestep;
					}
					voices[t].slide_step = 1 / voices[t].slide_length * 
						(voices[t].end_phasestep - voices[t].start_phasestep) / float(transport->samples_per_second);
					voices[t].phase = 0.0f;
					voices[t].cutoff = scutoff2;
					voices[t].update_powtab(scutoff * 2.0f);          
				}
			}
			if (tracks[t].volume) {
				voices[t].volume = float(*tracks[t].volume) / 128.0f;
				if (voices[t].volume == 1.0f)
					voices[t].pitch.phase = 0.0f;
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
					*b++ += v->s.envprocess(v->curtab[int(v->phase)%TABSIZE],c,1) * vol * adsr;
					// pitch sliding
					if (((v->slide_step > 0) && (v->phasestep < v->end_phasestep)) ||   
					((v->slide_step < 0) && (v->phasestep > v->end_phasestep))) { 
						v->phasestep += v->slide_step;
					} else {
						v->phasestep = v->end_phasestep;
					}
					// pitch lfo
					if (v->pitch.amp) {
						float rp = v->pitch.amp * sintab[int(v->pitch.phase)%TABSIZE];
						float scale = pow(2.0f, rp / 12.0f);
						v->phase += v->phasestep * scale;
					} else {
						v->phase += v->phasestep;
					}
					while (v->phase > TABSIZE)
						v->phase -= TABSIZE;
					v->pitch.phase += v->pitch.phasestep;
					while (v->pitch.phase > TABSIZE)
						v->pitch.phase -= TABSIZE;
				}
			} else {
				v->pitch.phase += v->pitch.phasestep * (float)n;
				while (v->pitch.phase > TABSIZE)
					v->pitch.phase -= TABSIZE;
			}
		}
		dsp_amp(outL, n, amp); 
		dsp_clip(outL, n, 1);
		dsp_copy(outL, outR, n);
	}
};

lunar_fx *new_fx() { return new synth(); }
