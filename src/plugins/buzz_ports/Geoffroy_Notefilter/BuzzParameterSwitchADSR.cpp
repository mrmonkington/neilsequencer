// BuzzParameterSwitchADSR.cpp: implementation of the BuzzParameterSwitchADSR class.
//
//////////////////////////////////////////////////////////////////////

#include "BuzzParameterSwitchADSR.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

byte BuzzParameterSwitchADSR::MIN_SLIDER_VALUE = BuzzParameterSwitchADSR_OFF;
byte BuzzParameterSwitchADSR::MAX_SLIDER_VALUE = BuzzParameterSwitchADSR_ON;
byte BuzzParameterSwitchADSR::UNCHANGED_SLIDER_VALUE = 0;
byte BuzzParameterSwitchADSR::INIT_SLIDER_VALUE = BuzzParameterSwitchADSR_OFF;

BuzzParameterSwitchADSR::BuzzParameterSwitchADSR() : BuzzParameter<byte,bool>()
{
	setSliderValue(INIT_SLIDER_VALUE);
}

BuzzParameterSwitchADSR::~BuzzParameterSwitchADSR()
{
}

void BuzzParameterSwitchADSR::compute()
{
	currentRealValue = (currentSliderValue == BuzzParameterSwitchADSR_ON);
}

char const * BuzzParameterSwitchADSR::toString(word const value)
{
	static char txt[50];
	txt[0]=0;

	switch (value) {
	case BuzzParameterSwitchADSR_OFF:
		sprintf(txt,"OFF");
		break;

	case BuzzParameterSwitchADSR_ON:
		sprintf(txt,"ON");
		break;

	default:
		sprintf(txt,"error, no value set");
		break;
	}

	return txt;
}