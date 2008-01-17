// NoteFilterTrack.cpp: implementation of the NoteFilterTrack class.
//
//////////////////////////////////////////////////////////////////////

#include "NoteFilterTrack.h"
//#include "../buzz2zzub/dsplib.h"
#include "BuzzParameterFilterType.h"


// Copied from lunar/dspbb.h

void dsp_zero(float *b, int numsamples) {
	memset(b, 0, sizeof(float) * numsamples);
}
void dsp_copy(float *i, float *o, int numsamples) {
	memcpy(o, i, sizeof(float) * numsamples);
}
void dsp_add(float *i, float *o, int numsamples) {
	while (numsamples--) {
		*o++ += *i++;
	}
}



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

NoteFilterTrack::NoteFilterTrack()
{
	oldValueForQ = 0;
	oldValueForA = oldValueForD = oldValueForS = oldValueForR = 0;
	a = d = s = r = NULL;
	q = NULL;
	volume = NULL;
	adsrActivated = false;

	for (int i = 0; i<NoteFilterTrack_NB_HARMONICS; i++) {
	  bandPassFilter[i].setType(CookbookFilter::BPF);
		bandPassFilter[i].setGain(0.0f);
		oldValueForHarmoVolumes[i] = 0.0f;
	}

	trackBufferTempSize = 4096;
	trackBufferTemp = (float *)malloc(trackBufferTempSize*sizeof(float));

	mixBufferTempSize = 4096;
	mixBufferTemp = (float *)malloc(mixBufferTempSize*sizeof(float));
}

NoteFilterTrack::~NoteFilterTrack()
{
	free(trackBufferTemp);
	free(mixBufferTemp);
}

void NoteFilterTrack::setNote(byte note) 
{
	// change the frequency
	float fundamental = buzzNote2Freq(note);

	for (int i = 0; i<NoteFilterTrack_NB_HARMONICS; i++) {
		bandPassFilter[i].setFrequency(fundamental*(i+1));
	}

	// reset ADSR
	adsrEnveloppe.reset();
}

void NoteFilterTrack::setQ(BuzzParameterQ * q) 
{
	this->q = q;
}

bool NoteFilterTrack::WorkStereo(float *psamples, int numsamples, int const mode) 
{
	/*
	 * update inertia for track parameters 
	 */

	adsrEnveloppe.timeGoesBy(numsamples);

	// wonder if Q has changed since last time
	if (q != NULL) if (q->getRealValue() != oldValueForQ) {
		oldValueForQ = q->getRealValue();
		for (int i = 0; i<NoteFilterTrack_NB_HARMONICS; i++) {
			bandPassFilter[i].setQ(oldValueForQ);
		}
	}

	// gain
	
	// wonder if a,d,s or r has changed since last time
	if (a!=NULL && d!=NULL && s!=NULL && r!=NULL && 
		(a->getRealValue() != oldValueForA ||
		 d->getRealValue() != oldValueForD ||
		 s->getRealValue() != oldValueForS ||
		 r->getRealValue() != oldValueForR)) {
		// change ADSR
		oldValueForA = a->getRealValue();
		oldValueForD = d->getRealValue();
		oldValueForS = s->getRealValue();
		oldValueForR = r->getRealValue();

		adsrEnveloppe.setA(oldValueForA);
		adsrEnveloppe.setD(oldValueForD);
		adsrEnveloppe.setS(oldValueForS);
		adsrEnveloppe.setR(oldValueForR);
	}

	// get the gain from the ADSR enveloppe
	float adsrGain = volume->getRealValue();
	if (adsrActivated) {
		adsrGain *= adsrEnveloppe.getGain();
	}

	// wonder if the harmonics volume has changed since last time too
	BuzzParameterVolume * ptBuzzParameterVolume;
	for (int j = 0; j < NoteFilterTrack_NB_HARMONICS; j++) {
		ptBuzzParameterVolume = &(harmoVolumes[j]);

		// volume of harmonics has changed
		if (ptBuzzParameterVolume != NULL) if (ptBuzzParameterVolume->getRealValue() != oldValueForHarmoVolumes[j]) {
			oldValueForHarmoVolumes[j] = ptBuzzParameterVolume->getRealValue();
		}
		
		bandPassFilter[j].setGain(oldValueForHarmoVolumes[j] * adsrGain);
	}

	// see if the temporary buffer is large enough
	if (numsamples * 2 > trackBufferTempSize) {
	  int *dummy = (int *) realloc(trackBufferTemp,numsamples * 2 * sizeof(float));
	  dummy = (int *) realloc(mixBufferTemp,numsamples * 2 * sizeof(float));
		trackBufferTempSize = numsamples;
		mixBufferTempSize = numsamples;
	}

	// zero the mixBuffer
	dsp_zero(mixBufferTemp,numsamples*2);

	// go through all the harmonics
	for (int i = 0; i<NoteFilterTrack_NB_HARMONICS; i++) {
		// copy the samples in the temporary buffer
                dsp_copy(psamples, trackBufferTemp, numsamples*2);

		// filter the thing
		if (bandPassFilter[i].WorkStereo(trackBufferTemp,numsamples,mode))
		{
			// mix it with the output
                        dsp_add(trackBufferTemp, mixBufferTemp, numsamples*2);
		}
	}

	// copy the output to psamples
	dsp_copy(mixBufferTemp, psamples, numsamples*2);
	
	return true;
}

void NoteFilterTrack::setSampleRate(int sampleRate)
{
	for (int i = 0; i<NoteFilterTrack_NB_HARMONICS; i++) {
		bandPassFilter[i].setSampleRate(sampleRate);
	}
}

float NoteFilterTrack::buzzNote2Freq(byte buzzNote)
{
	static float base_freq = 16.3516f;

	int Note = (buzzNote>>4)*12+(buzzNote&0x0f) -1;
	return (float)(base_freq* pow(2.0, Note/12.0));
}

void NoteFilterTrack::setHarmoVolumes(BuzzParameterVolume * harmoVolumes)
{
	this->harmoVolumes = harmoVolumes;
}

void NoteFilterTrack::setFilterType(byte filterType)
{
	CookbookFilter::filtertype cookbookType;

	switch (filterType) {
	case BuzzParameterFilterType_BPF:
		cookbookType = CookbookFilter::BPF;
		break;
	case BuzzParameterFilterType_NOTCH:
		cookbookType = CookbookFilter::NOTCH;
		break;
	case BuzzParameterFilterType_PEAK:
		cookbookType = CookbookFilter::PEAKINGEQ;
		break;
	default:
		cookbookType = CookbookFilter::LPF;
		break;
	}

	for (int i = 0; i<NoteFilterTrack_NB_HARMONICS; i++) {
		bandPassFilter[i].setType(cookbookType);
	}
}

void NoteFilterTrack::setADSR(BuzzParameterDuration *a, BuzzParameterDuration *d, BuzzParameterDuration *s, BuzzParameterDuration *r)
{
	this->a = a;
	this->d = d;
	this->s = s;
	this->r = r;
}

void NoteFilterTrack::activateADSR(bool activate)
{
	this->adsrActivated = activate;
}

void NoteFilterTrack::setVolume(BuzzParameterVolume *trackVolume)
{
	this->volume = trackVolume;
}
