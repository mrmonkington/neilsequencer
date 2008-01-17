// ADSREnveloppe.cpp: implementation of the ADSREnveloppe class.
//
//////////////////////////////////////////////////////////////////////

#include "ADSREnveloppe.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ADSREnveloppe::ADSREnveloppe()
{
	this->attackGain = 1.0f;
	this->sustainGain = 0.5f;
	this->releaseGain = 0.0f;
	this->a = 0;
	this->d = 0;
	this->s = 0;
	this->r = 0;
	this->time = 0;
}

ADSREnveloppe::~ADSREnveloppe()
{

}

void ADSREnveloppe::setA(unsigned long a)
{
	this->a = a;
}

void ADSREnveloppe::setD(unsigned long d)
{
	this->d = d;
}

void ADSREnveloppe::setS(unsigned long s)
{
	this->s = s;
}

void ADSREnveloppe::setR(unsigned long r)
{
	this->r = r;
}

void ADSREnveloppe::setSustainGain(float sustain) 
{
	this->sustainGain = sustain;
}

float ADSREnveloppe::getGain()
{
	// attack
	if (time<a) {
		return attackGain * ((float)time/(float)a);
	}

	// decay
	if (time>=a && time < a+d) {
		return attackGain + (sustainGain-attackGain)*(((float)time-(float)a)/(float)d);
	}

	// sustain
	if (time>=a+d && time < a+d+s) {
		return sustainGain;
	}

	// release
	if (time>=a+d+s && time < a+d+s+r) {
		return sustainGain + (releaseGain-sustainGain)*(((float)time-((float)a+(float)d+(float)s))/(float)r);
	}

	// after the release
	return releaseGain;
}

void ADSREnveloppe::timeGoesBy(unsigned long timeInSamples) 
{
	this->time += timeInSamples;
}

void ADSREnveloppe::reset() 
{
	this->time = 0;
}
