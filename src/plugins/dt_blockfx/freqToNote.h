#ifndef _FREQ_TO_NOTE_H_
#define _FREQ_TO_NOTE_H_

const float A4_FREQ = 440.000031f;

// return note name & fine tune corresponding to frequency 
int FreqToNote(char* out_txt, float freq);


#endif
