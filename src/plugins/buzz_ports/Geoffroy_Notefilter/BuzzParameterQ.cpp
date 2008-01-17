// BuzzParameterQ.cpp: implementation of the BuzzParameterQ class.
//
//////////////////////////////////////////////////////////////////////

#include "BuzzParameterQ.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

word BuzzParameterQ::MIN_SLIDER_VALUE = 0;
word BuzzParameterQ::MAX_SLIDER_VALUE = 0xFFFE;
word BuzzParameterQ::UNCHANGED_SLIDER_VALUE = 0xFFFF;
word BuzzParameterQ::INIT_SLIDER_VALUE = 0;

BuzzParameterQ::BuzzParameterQ() : BuzzParameter<word,float>()
{
	MIN_REAL_VALUE = 1.0f;
	MAX_REAL_VALUE = 127.0f;
	setSliderValue(INIT_SLIDER_VALUE);
}

BuzzParameterQ::~BuzzParameterQ()
{
}

void BuzzParameterQ::compute()
{
	float valueToReach = (float)(MIN_REAL_VALUE+(currentSliderValue-MIN_SLIDER_VALUE)*(MAX_REAL_VALUE-MIN_REAL_VALUE)/(MAX_SLIDER_VALUE-MIN_SLIDER_VALUE));

	// stop !
	if (timeSinceLastValueUpdate >= inertia) {
		currentRealValue = valueToReach;
	} 
	// go smooth!
	else {
		currentRealValue = (float) (currentRealValue + ((valueToReach - currentRealValue) * timeBetweenTwoUpdates /  (inertia - timeSinceLastValueUpdate)));
	}
}

char const * BuzzParameterQ::toString(word const value)
{
	static char txt[50];
	txt[0]=0;

	float valueToReach = (float)(MIN_REAL_VALUE+(value-MIN_SLIDER_VALUE)*(MAX_REAL_VALUE-MIN_REAL_VALUE)/(MAX_SLIDER_VALUE-MIN_SLIDER_VALUE));

	sprintf(txt,"%f",valueToReach);

	return txt;

}
