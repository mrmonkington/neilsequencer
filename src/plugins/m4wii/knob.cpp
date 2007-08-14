// Machine generated IDispatch wrapper class(es) created with ClassWizard

#include "stdafx.h"
#include "knob.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// CKnob properties

short CKnob::GetValue()
{
	short result;
	GetProperty(0x1, VT_I2, (void*)&result);
	return result;
}

void CKnob::SetValue(short propVal)
{
	SetProperty(0x1, VT_I2, propVal);
}

short CKnob::GetMaxValue()
{
	short result;
	GetProperty(0x2, VT_I2, (void*)&result);
	return result;
}

void CKnob::SetMaxValue(short propVal)
{
	SetProperty(0x2, VT_I2, propVal);
}

short CKnob::GetMinValue()
{
	short result;
	GetProperty(0x3, VT_I2, (void*)&result);
	return result;
}

void CKnob::SetMinValue(short propVal)
{
	SetProperty(0x3, VT_I2, propVal);
}

/////////////////////////////////////////////////////////////////////////////
// CKnob operations

void CKnob::AboutBox()
{
	InvokeHelper(0xfffffdd8, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CKnobEvents properties

/////////////////////////////////////////////////////////////////////////////
// CKnobEvents operations

void CKnobEvents::ValueChanged(short val)
{
	static BYTE parms[] =
		VTS_I2;
	InvokeHelper(0x1, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 val);
}
