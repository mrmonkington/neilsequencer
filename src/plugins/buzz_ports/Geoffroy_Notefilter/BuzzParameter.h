// BuzzParameter.h: interface for the BuzzParameter class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BUZZPARAMETER_H__7FE80804_5C2A_45A4_A003_9B0518A09195__INCLUDED_)
#define AFX_BUZZPARAMETER_H__7FE80804_5C2A_45A4_A003_9B0518A09195__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

template <class SliderValueType, class RealValueType>
class BuzzParameter  
{
public:
	BuzzParameter();
	virtual ~BuzzParameter();

	// your class should contain the following static variables
	/*
	static SliderValueType MIN_SLIDER_VALUE;
	static SliderValueType MAX_SLIDER_VALUE;
	static SliderValueType UNCHANGED_SLIDER_VALUE;
	static SliderValueType INIT_SLIDER_VALUE;

	static RealValueType MIN_REAL_VALUE;
	static RealValueType MAX_REAL_VALUE;
	*/

	void setSliderValue(SliderValueType sliderValue);
	RealValueType getRealValue();
	void timeGoesBy(int timeInSamples);
	void setInertia(int timeInSamples);

	virtual char const * toString(SliderValueType const value);

protected:
	SliderValueType currentSliderValue;
	RealValueType currentRealValue;
	int timeSinceLastValueUpdate;
	int inertia;
	int timeBetweenTwoUpdates;

	virtual void compute();
};

// we are on a template

template <class SliderValueType, class RealValueType> 
BuzzParameter<SliderValueType,RealValueType>::BuzzParameter()
{
	inertia = 0;
	timeBetweenTwoUpdates = 1;
}

template <class SliderValueType, class RealValueType> 
BuzzParameter<SliderValueType,RealValueType>::~BuzzParameter()
{
	// nothing
}

template <class SliderValueType, class RealValueType>
void BuzzParameter<SliderValueType,RealValueType>::setSliderValue(SliderValueType sliderValue)
{
	currentSliderValue = sliderValue;
	timeSinceLastValueUpdate = 0;
	compute();
}

template <class SliderValueType, class RealValueType>
RealValueType BuzzParameter<SliderValueType,RealValueType>::getRealValue()
{
	return currentRealValue;
}

template <class SliderValueType, class RealValueType>
void BuzzParameter<SliderValueType,RealValueType>::timeGoesBy(int timeInSamples)
{
	timeBetweenTwoUpdates = timeInSamples;
	timeSinceLastValueUpdate += timeInSamples;
	compute();
}

template <class SliderValueType, class RealValueType>
void BuzzParameter<SliderValueType,RealValueType>::setInertia(int timeInSamples)
{
	inertia = timeInSamples;
}

template <class SliderValueType, class RealValueType>
void BuzzParameter<SliderValueType,RealValueType>::compute()
{
	// to be defined in child class
}

template <class SliderValueType, class RealValueType>
char const * BuzzParameter<SliderValueType,RealValueType>::toString(SliderValueType const value)
{
	static char txt[50];
	txt[0]=0;

	sprintf(txt,"not defined");

	return txt;

}

#endif // !defined(AFX_BUZZPARAMETER_H__7FE80804_5C2A_45A4_A003_9B0518A09195__INCLUDED_)
