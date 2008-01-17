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
 */

struct CBuzzOsc {

	int			 iOscTable;
	const short	*pOscTable;
	int			 iLevel;
	const short	*pSubTable;
	int			 iOscMask;
	double		 fPos;
	double		 fRate;

	CBuzzOsc() {

		fPos = 0; }


  void SetTable(zzub::plugin *p, int i) {

          pOscTable = p->_host->get_oscillator_table(iOscTable = i); }


	void SetFrequency(double freq) {

          fRate = 2048.0 * freq / dspcSampleRate;

		if((iOscTable != OWF_SINE) && (fRate >= 0.5))
			iLevel = (int)ceil(log(fRate) * (1.0 / log(2.0)));
			else iLevel = 0;

		pSubTable	= pOscTable + zzub::get_oscillator_table_offset(iLevel);
		fRate		= fRate / (1 << iLevel);
		iOscMask	= (2048 >> iLevel) - 1; }


	void Work(float *pout, int ns) {

          //int iOldControlWord = _control87(0, 0);
          //_control87(_RC_DOWN, _MCW_RC);			
		short const	*ptbl	= pSubTable;
		int const	 omask	= iOscMask;
		double const d2i	= (1.5 * (1 << 26) * (1 << 26));
		double		 pos	= fPos;
		double		 step	= fRate;
		do {
			double		 res  = pos + d2i;
			int			 ipos = *(int *)&res;
			double const frac = pos - ipos;
			double const s1	  = ptbl[ipos&omask];
			double const s2   = ptbl[(ipos+1)&omask];
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
	double	 fPWidth;
	int		 iDistance;
	double	 fDistance;

	CPwPulse() : CBuzzOsc() {
		fPWidth = 0.5; }

	void Init(zzub::plugin *p) {
          SetTable(p, zzub::oscillator_type_sawtooth);
        }

	void SetPWidth(double w) {
		fPWidth = w; }

	void Update() {
		double d = fPWidth * (2048 >> iLevel);
		iDistance = DspFastD2I(d);
		fDistance = d - iDistance; }

#pragma optimize ("a", on)

	void WorkSamples(float *pout, int ns) {
          //int iOldControlWord = _control87(0, 0);
          //_control87(_RC_DOWN, _MCW_RC);			
		short const	*ptbl	= pSubTable;
		int const	 omask	= iOscMask;
		int const	 dist	= iDistance;
		double const df		= fDistance;
		double const d2i	= (1.5 * (1 << 26) * (1 << 26));
		double		 pos	= fPos;
		double		 step	= fRate;
		do {
			double		 res  = pos + d2i;
			int			 ipos = *(int *)&res;
			double const frac = pos - ipos;
			double const s1	  = ptbl[(ipos)&omask];
			double const s2   = ptbl[(ipos+1)&omask];
			double const s3	  = ptbl[(ipos+dist)&omask];
			double const s4   = ptbl[(ipos+dist+1)&omask];
			*pout++ = (float)(s1 - s3 + (s2-s1)*frac - (s4-s3)*(frac+df));
			pos += step;
		} while(--ns);
		fPos = pos;
		//_control87(iOldControlWord, _MCW_RC); 
        }

#pragma optimize ("a", off)

 };

#endif
