/*
 * NoteFilter
 *
 * a filter based on notes
 * 
 * feel free to tweak the code but please rename the machine to (YourName)nGeoffroy Notefilter II for instance
 * and send it to me!
 *
 * it's my first machine so be cool, it's not very well coded :-)
 * 
 * (c) Geoffroy Montel 2002
 * email : coder@minizza.com
 */

/*
  Version History:
	1.0 Initial release
  CHANGELOG:
  -
  */

#include "NoteFilter.h"
//#include "../buzz2zzub/dsplib.h"


//DLL_EXPORTS

// Buzz code starts here

// constructor
mi::mi() {
#if (miBETAWARNING)> 0
	firstTick=true;
#endif
#if miNUMGLOBALPARAMETERS>0
	GlobalVals = &gval;
#endif
#if (miNUMATTRIBUTES + miBETAWARNINGATTRIBUTE )>0
	AttrVals = (int *)&aval;
#else
	AttrVals = NULL;
#endif
#if (miMAX_TRACKS>0 && miNUMTRACKPARAMETERS>0)
	TrackVals = tval;
	miNumberOfTracks=0;
#else 
	TrackVals = NULL;
#endif

	// insert your code below

	trackBufferTempSize = 4096;
	trackBufferTemp = (float *)malloc(trackBufferTempSize*sizeof(float));

	mixBufferTempSize = 4096;
	mixBufferTemp = (float *)malloc(mixBufferTempSize*sizeof(float));

	// end of insert
}


// destructor
mi::~mi() {
	// free all allocated buffers here
	free(trackBufferTemp);
	free(mixBufferTemp);
}


// prepare everything, first Tick and AttributesChanged called _after_ this function
void mi::Init(CMachineDataInput * const pi) {
#if miISALWAYSSTEREO > 0
  //SetOutputMode(true);
#endif
#if miNOTIFYONSAMPLERATECHANGES > 0
  samplerate=pz->_master_info->samples_per_second;
#endif

	// insert your own code here which inits the global parameters

	// update with the unit
	unitChanged();

	a.setUnit(BuzzParameterUnit_MS);
	d.setUnit(BuzzParameterUnit_MS);
	s.setUnit(BuzzParameterUnit_MS);
	r.setUnit(BuzzParameterUnit_MS);

	// update samplerate & samplePerTick
	inertia.setSampleRate(samplerate);
	samplesPerTick = pz->_master_info->samples_per_tick;
	inertia.setSamplesPerTick(samplesPerTick);

	inertiaChanged();
	
	// end of insert

#if (miMAX_TRACKS>0 && miNUMTRACKPARAMETERS>0)
	for (int c = 0; c < miMAX_TRACKS; c++)
	{
		mytvals *mytv=mytval+c;
	// insert your own code here which inits the track parameters
		// mytv->xx = paraXX.DefValue

		mytv->noteFilterTrack.setSampleRate(samplerate);
		mytv->noteFilterTrack.setQ(&q);
		mytv->noteFilterTrack.setHarmoVolumes(harmoVolumes);
		mytv->noteFilterTrack.setADSR(&a,&d,&s,&r);
		mytv->volume.setSliderValue(BuzzParameterVolume::MAX_SLIDER_VALUE);
		mytv->noteFilterTrack.setVolume(&(mytv->volume));
	// end of insert
	}
#endif

}

#if (miMAX_TRACKS>0 && miNUMTRACKPARAMETERS>0)
// track number has changed, update your track variables
void mi::SetNumTracks(int const n) {
	miNumberOfTracks=n;
	// insert your own code below, if needed



	// end of insert
}
#endif

// user has selected some menu options
void mi::Command(int const i) {
	switch(i)
	{
	case 0:
	  //pCB->MessageBox(miABOUTTXT);
		break;
	}
}

// user has changed attributes, called after Init()
void mi::AttributesChanged() {
	// check all the attributes in aval
	// aval.time // and so on...
	
	
}

// Describe the parameters
char const *mi::DescribeValue(int const param, int const value) {
	static char txt[50];
	txt[0]=0;

	switch(param) {

	case PARAM_FILTERTYPE:
		return filterType.toString(value);
		break;

	case PARAM_Q:
		return q.toString(value);
		break;

	case PARAM_INERTIA:
		return inertia.toString(value);
		break;

	case PARAM_UNIT:
		return unit.toString(value);
		break;

	case PARAM_VOLUMEHARMO_0:
	case PARAM_VOLUMEHARMO_1:
	case PARAM_VOLUMEHARMO_2:
	case PARAM_VOLUMEHARMO_3:
	case PARAM_VOLUMEHARMO_4:
	case PARAM_VOLUMEHARMO_5:
	case PARAM_VOLUMEHARMO_6:
	case PARAM_VOLUMEHARMO_7:
	case PARAM_VOLUMEHARMO_8:
	case PARAM_VOLUMEHARMO_9:
		
		return harmoVolumes[param-PARAM_VOLUMEHARMO_0].toString(value);
		break;

	case PARAM_SWITCH_ADSR:
		return switchADSR.toString(value);
		break;

	case PARAM_A:
	case PARAM_D:
	case PARAM_S:
	case PARAM_R:
		return a.toString(value);
		break;

	case PARAM_NOTE:
	  return "Note";
	  break;

	case PARAM_TRACK_VOLUME:
		return harmoVolumes[0].toString(value);

	default:
		sprintf(txt,"** ERROR **");
	
	}

	return txt;
}

// machine input has changed mono<->stereo
void mi::OutputModeChanged(bool stereo) {
	
}

void mi::MidiNote(int const channel, int const value, int const velocity) {
	
}

void mi::Stop() {
	
}

// what to update when the inertia changes
void mi::inertiaChanged()
{
	// q has inertia
	this->q.setInertia(inertia.getRealValue());

	// harmonics volumes have inertia
	for (int i=0;i<NoteFilterTrack_NB_HARMONICS;i++) {
		harmoVolumes[i].setInertia(inertia.getRealValue());
	}

}

// what to update when the unit changes
void mi::unitChanged()
{
	inertia.setUnit(unit.getRealValue());
	inertiaChanged();

	/*
	a.setUnit(unit.getRealValue());
	d.setUnit(unit.getRealValue());
	s.setUnit(unit.getRealValue());
	r.setUnit(unit.getRealValue());
	*/
}

void mi::filterTypeChanged()
{
	if (filterType.getRealValue() == BuzzParameterFilterType_BPF) {
		q.MIN_REAL_VALUE = 1.0f;
		q.MAX_REAL_VALUE = 127.0f;
	} else if (filterType.getRealValue() == BuzzParameterFilterType_NOTCH) {
		q.MIN_REAL_VALUE = 0.01f;
		q.MAX_REAL_VALUE = 2.0f;
	} else if (filterType.getRealValue() == BuzzParameterFilterType_PEAK) {
		q.MIN_REAL_VALUE = 0.01f;
		q.MAX_REAL_VALUE = 10.0f;
	}
}


// check for updated parameters, called every tick
void mi::Tick() {

#if miNOTIFYONSAMPLERATECHANGES > 0
	if(pz->_master_info->samples_per_second != samplerate) {
	// insert your code below which handles the change in the samplerate

	  samplerate = pz->_master_info->samples_per_second;

		for (int c = 0; c < miMAX_TRACKS; c++)
		{
			mytvals *mytv=mytval+c;
			mytv->noteFilterTrack.setSampleRate(samplerate);
		}

		inertia.setSampleRate(samplerate);
		inertiaChanged();

	// end of insert
	}
#endif

	if(pz->_master_info->samples_per_tick != samplesPerTick) {
		samplesPerTick = pz->_master_info->samples_per_tick;
		inertia.setSamplesPerTick(samplesPerTick);
		inertiaChanged();
	}
/*
// example
  if (gval.xx != paraXX.NoValue) {
	  =gval.xx;
  }
  */


	// insert your code below

	if (gval.filtertype != zparaFilterType->value_none) {
		this->filterType.setSliderValue(gval.filtertype);
		for (int c = 0; c < miMAX_TRACKS; c++)
		{
			mytvals *mytv=mytval+c;
			mytv->noteFilterTrack.setFilterType(filterType.getRealValue());
		}

		filterTypeChanged();

	}

	if (gval.unit != zparaUnit->value_none) {
		this->unit.setSliderValue(gval.unit);
		/*
		this->inertia.setUnit(unit.getRealValue());
		inertiaChanged();
		*/
		unitChanged();
	}

	if (gval.inertia != zparaInertia->value_none) {
		this->inertia.setSliderValue(gval.inertia);
		this->q.setInertia(inertia.getRealValue());
		inertiaChanged();
	}

	if (gval.q != zparaQ->value_none) {
		this->q.setSliderValue(gval.q);
	}

	for (int i=0;i<NoteFilterTrack_NB_HARMONICS;i++) {
		if (gval.harmovolume[i] != zparaVolumeHarmo[i]->value_none) {
			this->harmoVolumes[i].setSliderValue(gval.harmovolume[i]);
		}
	}

	if (gval.switchADSR != zparaSwitchADSR->value_none) {
		this->switchADSR.setSliderValue(gval.switchADSR);
		for (int c = 0; c < miNumberOfTracks; c++) {
			mytvals *mytv=mytval+c;
			mytv->noteFilterTrack.activateADSR(this->switchADSR.getRealValue());
		}

	}

	if (gval.a != zparaA->value_none) {
		this->a.setSliderValue(gval.a);
	}

	if (gval.d != zparaD->value_none) {
		this->d.setSliderValue(gval.d);
	}

	if (gval.s != zparaS->value_none) {
		this->s.setSliderValue(gval.s);
	}

	if (gval.r != zparaR->value_none) {
		this->r.setSliderValue(gval.r);
	}

	// end of insert


// if you have multiple track support, this part will be compiled in
#if (miMAX_TRACKS>0 && miNUMTRACKPARAMETERS>0)
	for (int c = 0; c < miNumberOfTracks; c++) {
		tvals *tv=tval+c;
		mytvals *mytv=mytval+c;
		/*
		if (tv->xx != paraXX->value_none) {
			=tv->xx;
		}
		*/

		// insert your code for parsing track effects below, tv->value for each parameter
		if (tv->note != zparaNote->value_none) {
			mytv->noteFilterTrack.setNote(tv->note);
		}

		if (tv->volume != zparaTrackVolume->value_none) {
			mytv->volume.setSliderValue(tv->volume);
		}

		// end of insert
	
	}
#endif

#if (miBETAWARNING) > 0
  if(firstTick) {
	firstTick=false;

#if miBETAWARNING > 0
	if(aval.betawarning==0)
	  {}
	  //pCB->MessageBox(miBETAWARNINGTXT);
#endif

  }
#endif

} 


bool mi::WorkStereo(float *psamples, int numsamples, int const mode) {
	/*
	 * update parameters with inertia
	 */
	
	q.timeGoesBy(numsamples);
	for (int i=0;i<NoteFilterTrack_NB_HARMONICS;i++) {
		this->harmoVolumes[i].timeGoesBy(numsamples);
	}

	if (mode==WM_NOIO) {
		// machine is not connected to anything
		return false;
	} else

	if (mode==WM_WRITE) {
		// no sound coming in
		// generators make the sound here
		// - return true when sound has been produced
		// - return false for silence
		// effects (like delays) can produce sound here too
		// - return true if sound has been made, false otherwise
		return false;
	} else
	
	if (mode == WM_READ) {
		// <thru> set in the sequence editor
		// machine can read the stream but not write
		// return true always
		return true;
	}

	// effects handle sound here usually
	// return true always, except false when you want to silence the output
	
	// see if the temporary buffer is large enough -- is realloc allowed in the work function? -- jmmcd
	if (numsamples * 2 > trackBufferTempSize) {
	  int *dummy = (int *) realloc(trackBufferTemp,numsamples * 2 * sizeof(float));
	  dummy = (int *) realloc(mixBufferTemp,numsamples * 2 * sizeof(float));
		trackBufferTempSize = numsamples;
		mixBufferTempSize = numsamples;
	}

	// zero the mixBuffer
	dsp_zero(mixBufferTemp,numsamples*2);

	// process each track
	for (int c = 0; c < miNumberOfTracks; c++) {
		tvals *tv=tval+c;
		mytvals *mytv=mytval+c;

		// copy the samples in the temporary buffer
		dsp_copy(psamples, trackBufferTemp, numsamples*2);

		// process it
		mytv->noteFilterTrack.WorkStereo(trackBufferTemp,numsamples,mode);
		
		// mix it with the output
		dsp_add(trackBufferTemp, mixBufferTemp, numsamples*2);
	}

	// copy the output to psamples
	dsp_copy(mixBufferTemp, psamples, numsamples*2);

/*
	do {
		psamples[0]=psamples[0];
		psamples[1]=psamples[1];
		psamples+=2;
	} while(--numsamples);
*/
	return true;
}

bool mi::Work(float *psamples, int numsamples, int const mode) {
	
	if (mode==WM_NOIO) {
		// machine is not connected to anything
		return false;
	} else

	if (mode==WM_WRITE) {
		// no sound coming in
		// generators make the sound here
		// - return true when sound has been produced
		// - return false for silence
		// effects (like delays) can produce sound here too
		// - return true if sound has been made, false otherwise
		return false;
	} else

	if (mode == WM_READ) {
		// <thru> set in the sequence editor
		// machine can read the stream but not write
		// return true always
		return true;
	}

	// effects handle sound here usually
	// return true always, except false when you want to silence the output

/*
	do {
		psamples[0]=psamples[0];
		psamples++;
	} while(--numsamples);
*/
	return true;
}



