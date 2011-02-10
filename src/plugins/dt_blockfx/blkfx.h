/******************************************************************************
DT Block FX - classes to perform actual effects on the FFT'd data

BlkFx
	Base class for effects

BlkFx*
	Individual classes to perform a particular effect on the FFT spectrum

History
	Date       Version    Programmer         Comments
	16/2/03    1.0        Darrell Tam		Created
******************************************************************************/

#ifndef _DT_BLKFX_H_
#define _DT_BLKFX_H_
#include <stdio.h>
#include <vector>
#include "cplx.h"
#include "params.h"
#include "dt_blockfx.h"

using namespace std;
class MI_BlockFx;

//------------------------------------------------------------------------------------------
class BlkFx
// interface to FFT processing effects
{
public:
	MI_BlockFx *mi; // parent machine interface

	virtual ~BlkFx() {}

	virtual void process(float* x1_re, float* x1_im, float* x1_re_end);
	virtual void process(void); // called from MI_BlockEffects to perform tbe effect
	virtual void name(char* txt) { strcpy(txt, "None"); } // name of effect
	virtual void desc(char *txt, float v) { strcpy(txt, "-"); } // text description of value "v"
	typedef auto_ptr<BlkFx> AP;
};

//------------------------------------------------------------------------------------------
// wrap effect methods with a BlkFx calling class
//
struct BlkFxContrast : public BlkFx
{
	void process(float* x1_re, float* x1_im, float* x1_re_end, float raise, bool raise_neg);
	virtual void process(float* x1_re, float* x1_im, float* x1_re_end);
	virtual void name(char* txt) { strcpy(txt, "Contrast"); }
	virtual void desc(char *txt, float v) { 
		if(v == 0) strcpy(txt, "0 (off)");
		else sprintf(txt, "%.2f", v*2.0f-1.0f);
	}
};

struct BlkFxSmear : public BlkFx
{
	virtual void process(float* x1_re, float* x1_im, float* x1_re_end);
	virtual void name(char* txt) { strcpy(txt, "Smear"); }
	virtual void desc(char *txt, float v) {
		if(v == 0) strcpy(txt, "0% (off)");
		else sprintf(txt, "%.0f%%", v*100.0f);
	}
};
struct BlkFxPhaseShift : public BlkFx
{
	virtual void process(float* x1_re, float* x1_im, float* x1_re_end);
	virtual void name(char* txt) { strcpy(txt, "PhaseShift"); }
	virtual void desc(char *txt, float v) { sprintf(txt, "%.0fdeg", (v-0.5f)*360.0f);	}
};
struct BlkFxClip : public BlkFx
{
	virtual void process(float* x1_re, float* x1_im, float* x1_re_end);
	virtual void name(char* txt) { strcpy(txt, "Clip"); }
	virtual void desc(char *txt, float v) { sprintf(txt, "%.2f", v); }
};
struct BlkFxWeed : public BlkFx
{
	virtual void process(float* x1_re, float* x1_im, float* x1_re_end);
	virtual void name(char* txt) { strcpy(txt, "Weed"); }
	virtual void desc(char *txt, float v) { sprintf(txt, "%.2f", v*2.0f-1.0f); }
};
struct BlkFxShiftAdd : public BlkFx
{
	// temporary variables used during "process()"
	float curr_amp;

	float/*-0.5..0.5*/ getFreq(float v/*0..1*/);

	inline void apply(cplxf& src, cplxf& dst) { dst = src*curr_amp+dst; }
	virtual void process(void);
	virtual void name(char* txt) { strcpy(txt, "ShiftAdd"); }
	virtual void desc(char *txt, float v);
};
struct BlkFxShiftReplace : public BlkFxShiftAdd
{
	inline void apply(cplxf& src, cplxf& dst) { dst = src*curr_amp; }
	virtual void process(void);
	virtual void name(char* txt) { strcpy(txt, "ShiftReplace"); }
};


#endif
