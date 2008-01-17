// BuzzParameterDuration.cpp: implementation of the BuzzParameterDuration class.
//
//////////////////////////////////////////////////////////////////////

#include "BuzzParameterDuration.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

word BuzzParameterDuration::MIN_SLIDER_VALUE = 0;
word BuzzParameterDuration::MAX_SLIDER_VALUE = 0xFFFE;
word BuzzParameterDuration::UNCHANGED_SLIDER_VALUE = 0xFFFF;
word BuzzParameterDuration::INIT_SLIDER_VALUE = 0;

BuzzParameterDuration::BuzzParameterDuration() : BuzzParameter<word,dword>()
{
	unit = BuzzParameterUnit_SAMPLES;
	samplesPerTick = 44100;
	sampleRate = 44100;
	setSliderValue(INIT_SLIDER_VALUE);
}

BuzzParameterDuration::~BuzzParameterDuration()
{
}

void BuzzParameterDuration::compute()
{
	// convert in samples

	switch (unit) {
	case BuzzParameterUnit_SAMPLES:
		currentRealValue = currentSliderValue * 1000;
		break;

	case BuzzParameterUnit_MS:
		currentRealValue = currentSliderValue * sampleRate / 10000;
		break;

	case BuzzParameterUnit_TICKS:
		currentRealValue = currentSliderValue * samplesPerTick;
		break;

	default:
		currentRealValue = 0; // error
		break;
	}
}

char const * BuzzParameterDuration::toString(word const value)
{
	static char txt[50];
	txt[0]=0;

	switch (unit) {
	case BuzzParameterUnit_SAMPLES:
		sprintf(txt,"%d samples",value * 1000);
		break;

	case BuzzParameterUnit_MS:
		sprintf(txt,"%d.%d ms",value/10,value%10);
		break;

	case BuzzParameterUnit_TICKS:
		sprintf(txt,"%d ticks",value);
		break;

	default:
		sprintf(txt,"error, no unity set");
		break;
	}

	return txt;

}

void BuzzParameterDuration::setUnit(byte unit)
{
	this->unit = unit;
	compute();
}

void BuzzParameterDuration::setSamplesPerTick(int n)
{
	this->samplesPerTick = n;
	compute();
}

void BuzzParameterDuration::setSampleRate(int sampleRate) 
{
	this->sampleRate = sampleRate;
	compute();
}
