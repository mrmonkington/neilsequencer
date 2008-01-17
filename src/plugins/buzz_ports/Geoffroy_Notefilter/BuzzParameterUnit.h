// BuzzParameterUnit.h: interface for the BuzzParameterUnit class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BuzzParameterUnit_H__19CF9763_6FC9_47CC_914B_71D208757F04__INCLUDED_)
#define AFX_BuzzParameterUnit_H__19CF9763_6FC9_47CC_914B_71D208757F04__INCLUDED_

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

#define BuzzParameterUnit_SAMPLES	1
#define BuzzParameterUnit_MS		2
#define BuzzParameterUnit_TICKS		3

class BuzzParameterUnit : public BuzzParameter<byte,byte>
{
public:
	BuzzParameterUnit();
	virtual ~BuzzParameterUnit();

	static byte MIN_SLIDER_VALUE;
	static byte MAX_SLIDER_VALUE;
	static byte UNCHANGED_SLIDER_VALUE;
	static byte INIT_SLIDER_VALUE;

	virtual void compute();
	virtual char const * toString(word const value);
};

#endif // !defined(AFX_BuzzParameterUnit_H__19CF9763_6FC9_47CC_914B_71D208757F04__INCLUDED_)
