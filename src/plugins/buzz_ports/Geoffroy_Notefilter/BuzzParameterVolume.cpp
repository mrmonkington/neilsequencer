// BuzzParameterVolume.cpp: implementation of the BuzzParameterVolume class.
//
//////////////////////////////////////////////////////////////////////

#include "BuzzParameterVolume.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

word BuzzParameterVolume::MIN_SLIDER_VALUE = 0;
word BuzzParameterVolume::MAX_SLIDER_VALUE = 0xFFFE;
word BuzzParameterVolume::UNCHANGED_SLIDER_VALUE = 0xFFFF;
word BuzzParameterVolume::INIT_SLIDER_VALUE = 0;

float BuzzParameterVolume::MIN_REAL_VALUE = 0.00f;
float BuzzParameterVolume::MAX_REAL_VALUE = 1.0f;

BuzzParameterVolume::BuzzParameterVolume() : BuzzParameter<word,float>()
{
	setSliderValue(INIT_SLIDER_VALUE);
}

BuzzParameterVolume::~BuzzParameterVolume()
{
}

void BuzzParameterVolume::compute()
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

char const * BuzzParameterVolume::toString(word const value)
{
	static char txt[50];
	txt[0]=0;

	float valueToReach = (float)(MIN_REAL_VALUE+(value-MIN_SLIDER_VALUE)*(MAX_REAL_VALUE-MIN_REAL_VALUE)/(MAX_SLIDER_VALUE-MIN_SLIDER_VALUE));

	sprintf(txt,"%d %%",(int)(valueToReach*100.0f));

	return txt;

}
