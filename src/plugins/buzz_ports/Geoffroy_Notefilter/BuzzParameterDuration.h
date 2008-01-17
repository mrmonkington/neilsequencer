// BuzzParameterDuration.h: interface for the BuzzParameterDuration class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BuzzParameterDuration_H__19CF9763_6FC9_47CC_914B_71D208757F04__INCLUDED_)
#define AFX_BuzzParameterDuration_H__19CF9763_6FC9_47CC_914B_71D208757F04__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
//#include <windows.h>
#include "mdk.h"
#include "BuzzParameter.h"
#include "BuzzParameterUnit.h"

class BuzzParameterDuration : public BuzzParameter<word,dword>
{
public:
	BuzzParameterDuration();
	virtual ~BuzzParameterDuration();

	static word MIN_SLIDER_VALUE;
	static word MAX_SLIDER_VALUE;
	static word UNCHANGED_SLIDER_VALUE;
	static word INIT_SLIDER_VALUE;

	virtual void compute();
	virtual char const * toString(word const value);

	virtual void setUnit(byte u);
	virtual void setSamplesPerTick(int n);
	virtual void setSampleRate(int sampleRate);

protected:
	byte unit;
	int samplesPerTick;
	int sampleRate;
};

#endif // !defined(AFX_BuzzParameterDuration_H__19CF9763_6FC9_47CC_914B_71D208757F04__INCLUDED_)
