// NoteFilterTrack.h: interface for the NoteFilterTrack class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_NOTEFILTERTRACK_H__883FC995_5615_48EA_BBCE_1FD052522E0F__INCLUDED_)
#define AFX_NOTEFILTERTRACK_H__883FC995_5615_48EA_BBCE_1FD052522E0F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
//#include <windows.h>

#include "mdk.h"

#include "ADSREnveloppe.h"
#include "CookbookFilter.h"
#include "BuzzParameterQ.h"
#include "BuzzParameterVolume.h"
#include "BuzzParameterDuration.h"




#define NoteFilterTrack_NB_HARMONICS 10

class NoteFilterTrack  
{
public:
	NoteFilterTrack();
	virtual ~NoteFilterTrack();

	void setSampleRate(int sampleRate);
	void setNote(byte note);
	void setQ(BuzzParameterQ * q);
	void setHarmoVolumes(BuzzParameterVolume * harmoVolumes);
	void setFilterType(byte type);
	void setADSR(BuzzParameterDuration * a, BuzzParameterDuration * d, BuzzParameterDuration * s, BuzzParameterDuration * r);
	void activateADSR(bool activate);
	void setVolume(BuzzParameterVolume * trackVolume);

	bool WorkStereo(float *psamples, int numsamples, int const mode);

protected:
	bool adsrActivated;
	float buzzNote2Freq(byte note);

	BuzzParameterQ * q;
	BuzzParameterDuration *a, *d, *s, *r;
	ADSREnveloppe adsrEnveloppe;
	BuzzParameterVolume * volume;

	CookbookFilter bandPassFilter[NoteFilterTrack_NB_HARMONICS];
	BuzzParameterVolume * harmoVolumes;

	float oldValueForQ;
	float oldValueForHarmoVolumes[NoteFilterTrack_NB_HARMONICS];
	unsigned long oldValueForA,oldValueForD,oldValueForS,oldValueForR;

	float * trackBufferTemp;
	int trackBufferTempSize;
	float * mixBufferTemp;
	int mixBufferTempSize;
};

#endif // !defined(AFX_NOTEFILTERTRACK_H__883FC995_5615_48EA_BBCE_1FD052522E0F__INCLUDED_)
