// ADSREnveloppe.h: interface for the ADSREnveloppe class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ADSRENVELOPPE_H__96E06570_0AEB_45FF_AC55_6B62D7F397CF__INCLUDED_)
#define AFX_ADSRENVELOPPE_H__96E06570_0AEB_45FF_AC55_6B62D7F397CF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class ADSREnveloppe  
{
public:
	ADSREnveloppe();
	virtual ~ADSREnveloppe();

	void setA(unsigned long a);
	void setD(unsigned long d);
	void setS(unsigned long s);
	void setR(unsigned long r);

	void setSustainGain(float sustain);
	void reset();
	void timeGoesBy(unsigned long timeInSamples);
	float getGain();

private:
	unsigned long a;
	unsigned long d;
	unsigned long s;
	unsigned long r;

	float attackGain;
	float sustainGain;
	float releaseGain;

	unsigned long time;
};

#endif // !defined(AFX_ADSRENVELOPPE_H__96E06570_0AEB_45FF_AC55_6B62D7F397CF__INCLUDED_)
