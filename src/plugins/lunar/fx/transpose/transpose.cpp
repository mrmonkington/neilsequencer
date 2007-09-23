
#include "transpose.h"
#include <lunar/fx.hpp>

int quantizes[3][12] = {
//    C C#  D D#  E  F F#  G G#  A A#  B
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // none
	{ 0,-1, 0, 1, 0, 0,-1, 0,-1, 0,-1, 0}, // major
	{ 0,-1, 0, 0,-1, 0,-1, 0, 0,-1, 0,-1}, // minor
};

class transpose : public lunar::fx<transpose> {
public:
	float otrans;
	float ntrans;
	int *quantize;
	
	void init() {
		otrans = 0.0f;
		ntrans = 0.0f;
		quantize = quantizes[0];
	}

	void exit() {
		delete this;
	}

	void process_events() {
		if (globals->otrans)
			otrans = *globals->otrans - 5.0f;
		if (globals->ntrans)
			ntrans = *globals->ntrans - 6.0f;
		if (globals->hquantize)
			quantize = quantizes[int(*globals->hquantize + 0.5f)];
	}
	
	float quantize_note(float fnote) {
		int note = int(fnote + 0.5f);
		int octave = note / 12;
		return float((((note % 12) + quantize[note % 12]) % 12) + octave*12);
	}
	
	void process_note_regular(float *note) {
		if (note && (*note != 0)) {
			*note = quantize_note(*note) + otrans*12.0f + ntrans;
		}
	}
	
	void process_note_locked(float *lnote) {
		if (lnote && (*lnote != 0)) {
			*lnote = quantize_note(*lnote);
			int note = int(*lnote);
			int octave = note / 12;
			*lnote = float(octave*12 + (int(*lnote + ntrans + 0.5f) % 12));
		}
	}
	
	void process_controller_events() {
		process_note_regular(controllers->note1);
		process_note_regular(controllers->note2);
		process_note_regular(controllers->note3);
		process_note_regular(controllers->note4);
		process_note_regular(controllers->note5);
		process_note_regular(controllers->note6);
		process_note_regular(controllers->note7);
		process_note_regular(controllers->note8);
		
		process_note_locked(controllers->lnote1);
		process_note_locked(controllers->lnote2);
		process_note_locked(controllers->lnote3);
		process_note_locked(controllers->lnote4);
		process_note_locked(controllers->lnote5);
		process_note_locked(controllers->lnote6);
		process_note_locked(controllers->lnote7);
		process_note_locked(controllers->lnote8);
	}
	
	void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {
	}

};

lunar_fx *new_fx() { return new transpose(); }
