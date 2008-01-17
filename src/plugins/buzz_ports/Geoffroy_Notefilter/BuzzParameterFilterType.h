// BuzzParameterFilterType.h: interface for the BuzzParameterFilterType class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BuzzParameterFilterType_H__19CF9763_6FC9_47CC_914B_71D208757F04__INCLUDED_)
#define AFX_BuzzParameterFilterType_H__19CF9763_6FC9_47CC_914B_71D208757F04__INCLUDED_

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

#define BuzzParameterFilterType_BPF			1
#define BuzzParameterFilterType_NOTCH		2
#define BuzzParameterFilterType_PEAK		3

class BuzzParameterFilterType : public BuzzParameter<byte,byte>
{
public:
	BuzzParameterFilterType();
	virtual ~BuzzParameterFilterType();

	static byte MIN_SLIDER_VALUE;
	static byte MAX_SLIDER_VALUE;
	static byte UNCHANGED_SLIDER_VALUE;
	static byte INIT_SLIDER_VALUE;

	virtual void compute();
	virtual char const * toString(word const value);
};

#endif // !defined(AFX_BuzzParameterFilterType_H__19CF9763_6FC9_47CC_914B_71D208757F04__INCLUDED_)
