/*
 *		Dsp Classes	: Buzz Oscillators
 *
 *			Written by George Nicolaidis aka Geonik
 */

#ifndef inc_dspcBuzzOsc
#define inc_dspcBuzzOsc

#include "../DspClasses/DspClasses.h"
#include "zzub/plugin.h"

extern int dspcSampleRate;

/*	CBuzzOsc
 *
 *		Bandlimited oscillators built in Buzz
 * No longer built-in to buzz, we have to generate them.
 */

#define OWF_SINE 0
#define OWF_SAWTOOTH 1
#define OWF_PULSE 2
#define OWF_TRIANGLE 3
#define OWF_NOISE 4
#define OWF_303_SAWTOOTH 5

const int OSCTABSIZE = (2048 + 1024 + 512 + 256 + 
			128 + 64 + 32 + 16 + 8 + 4) * sizeof(short);
short oscTables[8][OSCTABSIZE];

int get_oscillator_table_offset(int const level) {
  assert(level >= 0 && level <= 10);
  return (2048 + 1024 + 512 + 256 + 128 + 64 + 32 + 16 + 8 + 4) & 
    ~((2048 + 1024 + 512 + 256 + 128 + 64 + 32 + 16 + 8 + 4) >> level);
}

double square(double v) {
  double sqmod = fmod(v, 2.0f * M_PI);
  return sqmod < M_PI ? -1 : 1;
}
 
double sawtooth(double v) {
  return (fmod(v, 2.0f * M_PI) / M_PI) - 1;
}

double triangle(double v) {
  double sqmod = fmod(v, 2.0f * M_PI);
  if (sqmod < M_PI) {
    return sqmod / M_PI;
  } else
    return (M_PI - (sqmod - M_PI)) / M_PI;
}

void generate_oscillator_tables() {
  int tabSize = 2048;
  srand(static_cast<unsigned int>(time(0)));
  for (int tabLevel = 0; tabLevel < 11; tabLevel++) {
    int tabOfs = get_oscillator_table_offset(tabLevel);
    for (int i = 0; i < tabSize; i++) {
      double dx = (double)i / tabSize;
      oscTables[OWF_SINE][tabOfs + i] = 
	(short)(sin(dx * 2.0f * M_PI) * 32000);
      oscTables[OWF_SAWTOOTH][tabOfs + i] = 
	(short)(sawtooth(dx * 2.0f * M_PI) * 32000);
      oscTables[OWF_PULSE][tabOfs + i] = 
	(short)(square(dx * 2.0f * M_PI) * 32000);
      oscTables[OWF_TRIANGLE][tabOfs + i] = 
	(short)(triangle(dx * 2.0f * M_PI) * 32000);
      oscTables[OWF_NOISE][tabOfs + i] = 
	(short)(((float)rand() / (float)RAND_MAX) * 64000.f - 32000);
      oscTables[OWF_303_SAWTOOTH][tabOfs + i] = 
	(short)(sawtooth(dx * 2.0f * M_PI) * 32000);
      oscTables[6][tabOfs + i] = 
	(short)(sin(dx * 2.0f * M_PI) * 32000);
    }
    tabSize /= 2;
  }
}

short * get_oscillator_table(int i) {
  return oscTables[i];
}


struct CBuzzOsc {
  int iOscTable;
  const short *pOscTable;
  int iLevel;
  const short *pSubTable;
  int iOscMask;
  double fPos;
  double fRate;

  CBuzzOsc() {
    fPos = 0; 
  }

  void SetTable(int i) {
    pOscTable = get_oscillator_table(iOscTable = i); 
  }

  void SetFrequency(double freq) {
    fRate = 2048.0 * freq / dspcSampleRate;
    if ((iOscTable != OWF_SINE) && (fRate >= 0.5))
      iLevel = (int)ceil(log(fRate) * (1.0 / log(2.0)));
    else iLevel = 0;
    pSubTable = pOscTable + get_oscillator_table_offset(iLevel);
    fRate = fRate / (1 << iLevel);
    iOscMask = (2048 >> iLevel) - 1; 
  }

  void Work(float *pout, int ns) {
    //int iOldControlWord = _control87(0, 0);
    //_control87(_RC_DOWN, _MCW_RC);			
    short const	*ptbl = pSubTable;
    int const omask = iOscMask;
    double const d2i = (1.5 * (1 << 26) * (1 << 26));
    double pos = fPos;
    double step	= fRate;
    do {
      double res = pos + d2i;
      int ipos = *(int*)&res;
      double const frac = pos - ipos;
      double const s1 = ptbl[ipos & omask];
      double const s2 = ptbl[(ipos + 1) & omask];
      *pout++ = (float)((s1 + (s2 - s1) * frac));
      pos += step;
    } while(--ns);
    fPos = pos;
    //_control87(iOldControlWord, _MCW_RC); 
  }
};


/*	CPwPulse
 *
 *		Bandlimited pulse with pulse width control
 */

/* // get_oscillator_table returns a pointer to the table */
/* // */
/* // get_oscillator_table_offset returns an offset to */
/* // the table for a specified level */

/* inline int get_oscillator_table_offset(unsigned int level) { */

struct CPwPulse : public CBuzzOsc {
  double fPWidth;
  int iDistance;
  double fDistance;

  CPwPulse() : CBuzzOsc() {
    fPWidth = 0.5; 
  }

  void Init() {
    SetTable(OWF_SAWTOOTH);
  }

  void SetPWidth(double w) {
    fPWidth = w; 
  }

  void Update() {
    double d = fPWidth * (2048 >> iLevel);
    iDistance = DspFastD2I(d);
    fDistance = d - iDistance; 
  }

  void WorkSamples(float *pout, int ns) {
    //int iOldControlWord = _control87(0, 0);
    //_control87(_RC_DOWN, _MCW_RC);			
    short const	*ptbl = pSubTable;
    int const omask = iOscMask;
    int const dist = iDistance;
    double const df = fDistance;
    double const d2i = (1.5 * (1 << 26) * (1 << 26));
    double pos = fPos;
    double step	= fRate;
    do {
      double res = pos + d2i;
      int ipos = *(int*)&res;
      double const frac = pos - ipos;
      double const s1 = ptbl[(ipos) & omask];
      double const s2 = ptbl[(ipos + 1) & omask];
      double const s3 = ptbl[(ipos + dist) & omask];
      double const s4 = ptbl[(ipos + dist + 1) & omask];
      *pout++ = (float)(s1 - s3 + (s2 - s1) * frac - (s4 - s3) * (frac + df));
      pos += step;
    } while(--ns);
    fPos = pos;
    //_control87(iOldControlWord, _MCW_RC); 
  }
};

#endif
