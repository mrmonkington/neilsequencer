// BuzzParameterVolume.h: interface for the BuzzParameterVolume class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BuzzParameterVolume_H__19CF9763_6FC9_47CC_914B_71D208757F04__INCLUDED_)
#define AFX_BuzzParameterVolume_H__19CF9763_6FC9_47CC_914B_71D208757F04__INCLUDED_

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

class BuzzParameterVolume : public BuzzParameter<word,float>  
{
public:
	BuzzParameterVolume();
	virtual ~BuzzParameterVolume();

	static word MIN_SLIDER_VALUE;
	static word MAX_SLIDER_VALUE;
	static word UNCHANGED_SLIDER_VALUE;
	static word INIT_SLIDER_VALUE;

	static float MIN_REAL_VALUE;
	static float MAX_REAL_VALUE;

	virtual void compute();
	virtual char const * toString(word const value);
};

#endif // !defined(AFX_BuzzParameterVolume_H__19CF9763_6FC9_47CC_914B_71D208757F04__INCLUDED_)
