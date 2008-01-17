// CookbookFilter.h: interface for the CookbookFilter class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CookbookFilter_H__CAAC7A47_AD64_4BCC_B48B_DA65E2F4A9FE__INCLUDED_)
#define AFX_CookbookFilter_H__CAAC7A47_AD64_4BCC_B48B_DA65E2F4A9FE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
//#include <windows.h>
#include <float.h>
#include "mdk.h"

class CookbookFilter  
{
public:
	void setBandwidth(float bandwidth);
	void setdBGain(float dBGain);
	void setSlope(float slope);
	typedef enum {LPF,HPF,BPF,NOTCH,PEAKINGEQ,LOWSHELF,HIGHSHELF} filtertype;
	
	CookbookFilter();
	virtual ~CookbookFilter();

	void setSampleRate(int samplesPerSec);
	void setFrequency(float freq);
	void setQ(float q);
	void setType(filtertype type);
	void setGain(float gain);
	bool WorkStereo(float *psamples, int numsamples, int const mode);

private:
	void computeCoeffs();

private:
	float bandwidth;
    float param_cutoff, param_resonance;
    float filtCoefTab[5];
    float lx1, lx2, ly1, ly2; // Left sample history
    float rx1, rx2, ry1, ry2; // Right sample history
	float gain;
	int samplesPerSec;
	filtertype type;
protected:
	float slope;
	float dBGain;
};

#endif // !defined(AFX_CookbookFilter_H__CAAC7A47_AD64_4BCC_B48B_DA65E2F4A9FE__INCLUDED_)
