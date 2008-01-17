// BuzzParameterUnit.cpp: implementation of the BuzzParameterUnit class.
//
//////////////////////////////////////////////////////////////////////

#include "BuzzParameterUnit.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

byte BuzzParameterUnit::MIN_SLIDER_VALUE = BuzzParameterUnit_MS;
byte BuzzParameterUnit::MAX_SLIDER_VALUE = BuzzParameterUnit_TICKS;
byte BuzzParameterUnit::UNCHANGED_SLIDER_VALUE = 0;
byte BuzzParameterUnit::INIT_SLIDER_VALUE = BuzzParameterUnit_MS;

BuzzParameterUnit::BuzzParameterUnit() : BuzzParameter<byte,byte>()
{
	setSliderValue(INIT_SLIDER_VALUE);
}

BuzzParameterUnit::~BuzzParameterUnit()
{
}

void BuzzParameterUnit::compute()
{
	currentRealValue = currentSliderValue;
}

char const * BuzzParameterUnit::toString(word const value)
{
	static char txt[50];
	txt[0]=0;

	switch (value) {
	case BuzzParameterUnit_SAMPLES:
		sprintf(txt,"samples");
		break;

	case BuzzParameterUnit_MS:
		sprintf(txt,"ms");
		break;

	case BuzzParameterUnit_TICKS:
		sprintf(txt,"ticks");
		break;

	default:
		sprintf(txt,"error, no unity set");
		break;
	}

	return txt;
}