// BuzzParameterFilterType.cpp: implementation of the BuzzParameterFilterType class.
//
//////////////////////////////////////////////////////////////////////

#include "BuzzParameterFilterType.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

byte BuzzParameterFilterType::MIN_SLIDER_VALUE = BuzzParameterFilterType_BPF;
byte BuzzParameterFilterType::MAX_SLIDER_VALUE = BuzzParameterFilterType_PEAK;
byte BuzzParameterFilterType::UNCHANGED_SLIDER_VALUE = 0;
byte BuzzParameterFilterType::INIT_SLIDER_VALUE = BuzzParameterFilterType_BPF;

BuzzParameterFilterType::BuzzParameterFilterType() : BuzzParameter<byte,byte>()
{
	setSliderValue(INIT_SLIDER_VALUE);
}

BuzzParameterFilterType::~BuzzParameterFilterType()
{
}

void BuzzParameterFilterType::compute()
{
	currentRealValue = currentSliderValue;
}

char const * BuzzParameterFilterType::toString(word const value)
{
	static char txt[50];
	txt[0]=0;

	switch (value) {
	case BuzzParameterFilterType_BPF:
		sprintf(txt,"BandPass");
		break;

	case BuzzParameterFilterType_NOTCH:
		sprintf(txt,"Notch");
		break;

	case BuzzParameterFilterType_PEAK:
		sprintf(txt,"Peak");
		break;

	default:
		sprintf(txt,"error, no filter set");
		break;
	}

	return txt;
}