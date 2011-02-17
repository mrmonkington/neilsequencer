
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
	int custom_quantize[12];
	
	void init() {
		otrans = 0.0f;
		ntrans = 0.0f;
		quantize = quantizes[0];
	}

	void exit() {
		delete this;
	}
	
	void read_custom_qvalue(int index, float *v) {
		if (v) {
			custom_quantize[index] = (12 + int(*v + 0.5f) - index) % 12;
		}
	}

	void process_events() {
		if (globals->otrans)
			otrans = *globals->otrans - 5.0f;
		if (globals->ntrans)
			ntrans = *globals->ntrans - 6.0f;
		if (globals->hquantize) {
			int q = int(*globals->hquantize + 0.5f);
			if (q == 0)
				quantize = custom_quantize;
			else
				quantize = quantizes[q];
		}
		read_custom_qvalue(0, globals->q0);
		read_custom_qvalue(1, globals->q1);
		read_custom_qvalue(2, globals->q2);
		read_custom_qvalue(3, globals->q3);
		read_custom_qvalue(4, globals->q4);
		read_custom_qvalue(5, globals->q5);
		read_custom_qvalue(6, globals->q6);
		read_custom_qvalue(7, globals->q7);
		read_custom_qvalue(8, globals->q8);
		read_custom_qvalue(9, globals->q9);
		read_custom_qvalue(10, globals->q10);
		read_custom_qvalue(11, globals->q11);
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
