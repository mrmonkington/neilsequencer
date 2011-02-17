#include <math.h>
#include <stdio.h>
#include "FreqToNote.h"

static char *note_names[] = {"c","c#","d","d#","e","f","f#","g","g#","a","a#","b" };
static float log_2 = logf(2);
static float log_c0_hz = logf(A4_FREQ)-(4.0f+9.0f/12.0f)*logf(2);


int FreqToNote(char* out_txt, float freq)
{
	float log_freq = logf(freq);

	// don't print freqs below c0
	if(log_freq < log_c0_hz) return sprintf(out_txt, "-"); 

	float note = 12.0f*(log_freq-log_c0_hz)/log_2;
	int close = (int)(note+0.5f);
	int cents = (int)((note-close)*100.0f);
	int octave = close/12;
	int note12 = close-12*octave;
	return sprintf(out_txt, "%s%d:%d", note_names[note12], octave, cents);
}
