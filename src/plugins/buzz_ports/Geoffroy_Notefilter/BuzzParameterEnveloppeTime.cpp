// BuzzParameterEnveloppeTime.cpp: implementation of the BuzzParameterEnveloppeTime class.
//
//////////////////////////////////////////////////////////////////////

#include "BuzzParameterEnveloppeTime.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

word BuzzParameterEnveloppeTime::MIN_SLIDER_VALUE = 0;
word BuzzParameterEnveloppeTime::MAX_SLIDER_VALUE = 0xFFFE;
word BuzzParameterEnveloppeTime::UNCHANGED_SLIDER_VALUE = 0xFFFF;
word BuzzParameterEnveloppeTime::INIT_SLIDER_VALUE = 0;

float BuzzParameterEnveloppeTime::MIN_REAL_VALUE = 0.01f;
float BuzzParameterEnveloppeTime::MAX_REAL_VALUE = 127.0f;

BuzzParameterEnveloppeTime::BuzzParameterEnveloppeTime() : BuzzParameter<word,float>()
{
}

BuzzParameterEnveloppeTime::~BuzzParameterEnveloppeTime()
{
}

void BuzzParameterEnveloppeTime::compute()
{
	float valueToReach = (float)(MIN_REAL_VALUE+(currentSliderValue-MIN_SLIDER_VALUE)*(MAX_REAL_VALUE-MIN_REAL_VALUE)/(MAX_SLIDER_VALUE-MIN_SLIDER_VALUE));

	// stop !
	if (timeSinceLastValueUpdate >= inertia) {
		currentRealValue = valueToReach;
	} 
	// go smooth!
	else {
		currentRealValue = (float) (currentRealValue + ((valueToReach - currentRealValue) / (float) (inertia - timeSinceLastValueUpdate)));
	}
}

char const * BuzzParameterEnveloppeTime::toString(word const value)
{
	static char txt[50];
	txt[0]=0;

	float valueToReach = (float)(MIN_REAL_VALUE+(value-MIN_SLIDER_VALUE)*(MAX_REAL_VALUE-MIN_REAL_VALUE)/(MAX_SLIDER_VALUE-MIN_SLIDER_VALUE));

	sprintf(txt,"%f",valueToReach);

	return txt;

}
