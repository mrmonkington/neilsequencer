#include <string.h>
#include "Track.h"
#include "zzub/plugin.h"
#include "Tracker.h"
#include "Channel.h"
#include <math.h>
#include <float.h>
#include <stdlib.h>

using namespace SurfDSPLib;

#define	NONOTE(v) (((v)==zzub::note_value_none)||((v)==zzub::note_value_off))

typedef	struct {
  float	f[512];
} crapkludge;

CTrack::CTrack() {
  m_pChannel = NULL;
}

CTrack::~CTrack() {

}

void CTrack::Reset() {
  if (m_pChannel) {
    m_pChannel->Free();
    m_pChannel = NULL;
  }

  m_pSample = NULL;

  m_iInstrument = 0;
  m_iLastCommand = 0;

  m_iSubDivide = 6;
  m_iLastSample = 0;
  m_iLastTick = 0;
  m_fVolume = 0;
  m_fFreq = 0;
  m_fWantedFreq = 0;
  m_fBaseFreq = 0;
  m_fToneportSpeed = 0.0f;
  m_iBaseNote = 0;
  m_iLastCommand = 1 << 0xF;
  m_iAutoShuffleSteps = 2;
  m_iAutoShuffleAmount = 0;
  m_iAutoShuffleCount = 0;

  m_fVibratoSpeed = 0;
  m_fVibratoDepth = 0;
  m_fVibratoPos = 0;
  m_iVibratoType = 0;

  m_fPanSpeed = 0;
  m_fPanDepth = 0;
  m_fPanPos = 0;
  m_iPanType = 0;

  m_fTremoloSpeed = 0;
  m_fTremoloDepth = 0;
  m_fTremoloPos = 0;
  m_iTremoloType = 0;

  m_fBaseVolume = 0;
  m_iSlice = 0;
  m_iSampleOffset = 0;
  m_oMuted = 0;
  m_fPan = 0.0f;
  m_fBasePan = 0.0f;

  m_fCutoffFreq = 11050.0f;
  m_fBaseCutoffFreq = 11050.0f;
  m_iCutoffLFOType = 0;
  m_fCutoffLFOSpeed = 0;
  m_fCutoffLFODepth = 0;

  m_fResonance = 1.0f;
  m_fBaseResonance = 1.0f;
  m_iResonanceLFOType = 0;
  m_fResonanceLFOSpeed = 0;
  m_fResonanceLFODepth = 0;

  m_iFilterType = 1;

  m_oReverse = false;

  m_fFinetune = 1.0f;

  m_oAvailableForMIDI = true;
  m_oSustained = false;
}

#define	RETRIG_FREQ 0x01
#define	RETRIG_VOLUME 0x02
#define	RETRIG_INS 0x04
#define	RETRIG_INSLOOP 0x08
#define	RETRIG_CUTOFF 0x10
#define	RETRIG_REZ 0x20

#define	FREQSCALE 512.0f

#define	CONVNOTE(x) ((!NONOTE(x)) ? ((x) >> 4) * 12 + ((x) & 0xF) - 1 : (x))

void CTrack::Release() {
  if (m_pChannel) {
    if (m_pChannel->Release() == false && m_pMachine->m_oVirtualChannels) {
      m_pChannel->m_pOwner = NULL;
      m_pChannel = NULL;
    }
  }
}

int CTrack::NewNote(bool oRetrig) {
  int r = 0;

  if (m_pMachine->m_oVirtualChannels)
    Release();
  if (m_pChannel == NULL) {
    m_pChannel = m_pMachine->AllocChannel();
    m_pChannel->m_pOwner = this;
    m_pChannel->m_pInstrument = 
      m_pMachine->m_Wavetable.GetInstrument(m_iInstrument);
    m_pChannel->m_PanningEnvelope.ReadEnvelope(m_pMachine->_host, 
					       m_iInstrument, 1);
    m_pChannel->m_PitchEnvelope.ReadEnvelope(m_pMachine->_host, 
					     m_iInstrument, 2);
  }
  if (m_pChannel->m_pInstrument) {
    if (m_pSample && !m_pMachine->m_oVirtualChannels)
      m_pSample->Free();
    m_pSample = m_pChannel->m_pInstrument->GetSample(m_Vals.note);
  } else {
    m_pSample = NULL;
  }
  if (m_pSample) {
    r |= RETRIG_FREQ | RETRIG_INS;
    if (!oRetrig) {
      m_fFreq = (float)pow(2.0f, (CONVNOTE(m_iBaseNote) - 
				  CONVNOTE(m_pSample->GetRootNote())) / 
			   12.0f) * m_pSample->GetRootFrequency() / 
	m_pMachine->_master_info->samples_per_second;
      m_fBaseFreq = m_fFreq;
      m_fWantedFreq = m_fFreq;
    }
  }
  return r;
}

void CTrack::Tick(CTrackVals &tv) {
  bool oDelayNote = false;

  m_oAvailableForMIDI = true;
  m_Vals = tv;
  m_iAutoShuffleCount += 1;
  m_oAutoShuffled = false;
  m_iRandomDelay = 0;

  int thesecommands = 0;
  int retrig = 0;

  int i;
  for (i = 0; i < TOTALEFFECTS; i += 1) {
    unsigned char command, argument;
    
    command = m_Vals.effects[i].command;
    argument = m_Vals.effects[i].argument;

    if (command || argument)
      thesecommands |= 1 << command;
    if (command == 0xED)
      oDelayNote = true;
    if (command == 0x15)
      oDelayNote = true;
    if (command == 0x18 && (argument & 0xF0))
      oDelayNote = true;
  }

  if (m_iAutoShuffleAmount && 
      m_iAutoShuffleSteps && 
      (m_iAutoShuffleCount % m_iAutoShuffleSteps)) {
    // Shuffle!
    // retrig &= ~(RETRIG_INS|RETRIG_INSLOOP);
    oDelayNote = true;
  }

  if ((m_iLastCommand & (1 << 4)) && ((thesecommands & (1 << 4)) == 0)) {
    m_fFreq = m_fBaseFreq;
    retrig |= RETRIG_FREQ;
  }
  if ((m_iLastCommand & (1 << 7)) && ((thesecommands & (1 << 7)) == 0)) {
    m_fVolume = m_fBaseVolume;
    retrig |= RETRIG_VOLUME;
  }
  if ((m_iLastCommand & (1 << 6)) && ((thesecommands & (1 << 6)) == 0)) {
    m_fPan = m_fBasePan;
    retrig |= RETRIG_VOLUME;
  }
  if (m_Vals.instrument >= 1) {
    if (m_pChannel == NULL) {
      m_pChannel = m_pMachine->AllocChannel();
      m_pChannel->m_pOwner = this;
    }

    m_pChannel->m_pInstrument = 
      m_pMachine->m_Wavetable.GetInstrument(m_Vals.instrument);
    // if(m_pSample && !CMachine::s_oVirtualChannels)
    //	 m_pSample->Free();
    // m_pSample=NULL;

    m_fBaseVolume = m_fVolume = 1.0f;

    retrig |= RETRIG_VOLUME;
    m_iSampleOffset = 0;
    m_iSlice = 0;
    if ((m_iInstrument != m_Vals.instrument) && NONOTE(m_Vals.note)) {
      retrig |= RETRIG_INS | RETRIG_INSLOOP;
    }
    m_iInstrument = m_Vals.instrument;
    m_oReverse = false;
    m_pChannel->m_PanningEnvelope.ReadEnvelope(m_pMachine->_host, 
					       m_iInstrument, 1);
    m_pChannel->m_PitchEnvelope.ReadEnvelope(m_pMachine->_host, 
					     m_iInstrument, 2);
  }
  if (m_Vals.note == zzub::note_value_off) {
    if (m_pMachine->m_oSustainAllReleases) {
      m_oSustained = true;
    } else {
      if (!oDelayNote)
	Release();
    }
  }

  if (!NONOTE(m_Vals.note)) {
    m_iBaseNote = m_Vals.note;
    if ((thesecommands & (1 << 3)) == 0) {
      if (!oDelayNote)
	retrig |= NewNote();
    } else if (thesecommands & (1 << 3)) {
      if (m_pSample) {
	m_fWantedFreq = 
	  (float)pow(2.0f, (CONVNOTE(m_Vals.note) - 
			    CONVNOTE(m_pSample->GetRootNote())) / 12.0f) * 
	  m_pSample->GetRootFrequency() / 
	  m_pMachine->_master_info->samples_per_second;
      }
      m_iBaseNote = m_Vals.note;
    }

    if ((m_iVibratoType & 0x04) == 0)
      m_fVibratoPos = 0;
    if ((m_iTremoloType & 0x04) == 0)
      m_fTremoloPos = 0;
    if ((m_iPanType&0x04) == 0)
      m_fPanPos = 0;
    if ((m_iCutoffLFOType & 0x04) == 0)
      m_fCutoffLFOPos = 0;
    if ((m_iResonanceLFOType & 0x04) == 0)
      m_fResonanceLFOPos = 0;

    m_iProbability = 256;
    m_iLoopStretch = -1;
    if (m_pChannel) {
      m_pChannel->m_fPitchEnvFreq = 1.0f;
      m_pChannel->m_PanningEnvelope.Restart(1.0f / float(m_pMachine->_master_info->samples_per_tick * m_pMachine->m_Attributes.iVolumeEnvelopeTicks));
      m_pChannel->m_PitchEnvelope.Restart(1.0f / float(m_pMachine->_master_info->samples_per_tick * m_pMachine->m_Attributes.iVolumeEnvelopeTicks));
      m_pChannel->set_filter_bypass(m_iFilterType > 1 ? false : true);
      m_pChannel->set_filter_mode(m_iFilterType == 2 ? Svf::LOWPASS : Svf::HIGHPASS);
      retrig |= RETRIG_CUTOFF | RETRIG_REZ;
    }

    if (m_pMachine->m_oSustainAllReleases)
      m_oSustained = false;
  }

  if (m_Vals.volume != 0xFF) {
    m_fBaseVolume = m_fVolume = float(m_Vals.volume) / 128.0f;
    retrig |= RETRIG_VOLUME;
  }

  for (i = 0; i < TOTALEFFECTS; i += 1) {
    unsigned char command, argument;
    command = m_Vals.effects[i].command;
    argument = m_Vals.effects[i].argument;

    if (command == 0xFF)
      continue;

    switch (command) {
    case 0x0:
      if (argument == 0)
	thesecommands &= ~(1 << 0);
      break;
    case 0x3:
      if (argument != 0)
	m_fToneportSpeed = (float)pow(2.0f, float(argument) / FREQSCALE);
      break;
    case 0x4:
      if (argument != 0) {
	if ((argument & 0x0F) != 0)
	  m_fVibratoDepth = (argument & 0xF) * (argument & 0xF) / 700.0f;
	if ((argument & 0xF0) != 0)
	  m_fVibratoSpeed = (argument >> 4) * (argument >> 4) * 
	    2 * float(M_PI) / 256.0f;
      }
      break;
    case 0x6:
      if (argument != 0) {
	if ((argument&0x0F) != 0)
	  m_fPanDepth = (argument & 0xF) / 15.0f;
	if ((argument&0xF0) != 0)
	  m_fPanSpeed = (argument >> 4) * (argument >> 4) * 
	    2 * float(M_PI) / 256.0f;
      }
      break;
    case 0x7:
      if (argument != 0) {
	if ((argument & 0x0F) != 0 )
	  m_fTremoloDepth = (argument & 0xF) / 15.0f;
	if ((argument & 0xF0) != 0)
	  m_fTremoloSpeed = (argument >> 4) * (argument >> 4) * 
	    2 * float(M_PI) / 256.0f;
      }
      break;
    case 0x8:
      m_fPan = m_fBasePan = (float(argument) / 255.0f) * 2.0f - 1.0f;
      retrig |= RETRIG_VOLUME;
      break;
    case 0x9:
      m_iSampleOffset = argument == 0 ? 0x100 : argument;
      break;
    case 0x39:
      m_iSlice = argument;
      break;
    case 0x10:
      m_iProbability = argument;
      break;
    case 0x11:
      m_iLoopStretch = argument;
      m_oLoopStretchTrack = false;
      break;
    case 0x12:
      m_iLoopStretch = argument;
      m_oLoopStretchTrack = true;
      break;
    case 0x13:
      if (argument) {
	if (argument >> 4)
	  m_iAutoShuffleSteps = argument >> 4;
	m_iAutoShuffleAmount = argument & 0xF;
      }
      m_iAutoShuffleCount = 0;
      break;
    case 0x14:
      if (argument) {
	m_fBaseVolume += ((rand() % (argument << 1)) - argument) * 
	  (1.0f / 128.0f);
	if (m_fBaseVolume < 0)
	  m_fBaseVolume = 0;
	if (m_fBaseVolume > 2.0f)
	  m_fBaseVolume = 2.0f;
	m_fVolume = m_fBaseVolume;
	retrig |= RETRIG_VOLUME;
      }
      break;
    case 0x15:
      if (argument) {
	m_iRandomDelay = rand() % (argument + 1);
	/*
	  if (m_iRandomDelay)
	  retrig &= ~(RETRIG_INS | RETRIG_INSLOOP);
	*/
      }
      break;
    case 0x16:
      if (argument) {
	m_fFreq *= 
	  (float)pow(2.0f, float((rand() % (argument << 1)) - argument) / 
		     FREQSCALE);
	retrig |= RETRIG_FREQ;
      }
      break;
    case 0x17:
      if (argument) {
	if (m_pSample) {
	  m_fFreq = 
	    (float)pow(2.0f, (CONVNOTE(m_Vals.note) - 
			      CONVNOTE(m_pSample->GetRootNote())) / 12.0f) * 
	    (m_pSample->GetRootFrequency() * argument) / 
	    m_pMachine->_master_info->samples_per_second;
	  m_fBaseFreq = m_fFreq;
	}
      }
      break;
      /*
	case 0x18:
	  if (argument & 0xF0) {
	    retrig &= ~(RETRIG_INS | RETRIG_INSLOOP);
	  }
	  break;
      */
    case 0x20:
      // Cutoff set abs
      if (argument) {
	m_fBaseCutoffFreq = m_fCutoffFreq = 
	  float(70.0f + pow((argument / 256.0f) * sqrt(22000.0f), 2.0f));
	retrig |= RETRIG_CUTOFF;
      }
      break;
    case 0x23:
      // Cutoff set lfo
      m_iCutoffLFOType = argument & 7;
      break;
    case 0x24:
      if (argument != 0) {
	if ((argument & 0x0F) != 0)
	  m_fCutoffLFODepth = (argument & 0xF) * (argument & 0xF) / 100.0f;
	if ((argument & 0xF0) != 0)
	  m_fCutoffLFOSpeed = (argument >>4 ) * (argument >> 4) * 
	    2 * float(M_PI) / 256.0f;
      }
      break;
    case 0x25:
      // Fine cutoff slide up
      m_fCutoffFreq *= (float)pow(2.0f, float(argument) / FREQSCALE);
      m_fBaseCutoffFreq = m_fCutoffFreq;
      retrig |= RETRIG_CUTOFF;
      break;
    case 0x26:
      // Fine cutoff slide down
      m_fCutoffFreq *= (float)pow(2.0f, -float(argument) / FREQSCALE);
      m_fBaseCutoffFreq = m_fCutoffFreq;
      retrig |= RETRIG_CUTOFF;
      break;
    case 0x28:
      // Resonance set abs
      if (argument) {
	m_fBaseResonance = m_fResonance = float(1.0f + argument / 10.0f );
	retrig |= RETRIG_REZ;
      }
      break;
    case 0x2B:
      // Cutoff set lfo
      m_iResonanceLFOType = argument & 7;
      break;
    case 0x2C:
      if (argument != 0) {
	if ((argument & 0x0F) != 0)
	  m_fResonanceLFODepth = (argument & 0xF) * (argument & 0xF) / 20.0f;
	if ((argument & 0xF0) !=0)
	  m_fResonanceLFOSpeed = (argument >> 4) * (argument >> 4) * 
	    2 * float(M_PI) / 256.0f;
      }
      break;
    case 0x2D:
      // Fine resonance slide up
      m_fResonance += argument / 100.0f;
      m_fBaseResonance = m_fResonance;
      retrig |= RETRIG_REZ;
      break;
    case 0x2E:
      // Fine resonance slide down
      m_fResonance -= argument / 100.0f;
      m_fBaseResonance = m_fResonance;
      retrig |= RETRIG_REZ;
      break;
    case 0xE0:
      m_iFilterType = argument;
      break;
    case 0xE1:
      m_fFreq *= (float)pow(2.0f, float(argument) / FREQSCALE);
      retrig |= RETRIG_FREQ;
      break;
    case 0xE2:
      m_fFreq /= (float)pow(2.0f, float(argument) / FREQSCALE);
      retrig |= RETRIG_FREQ;
      break;
    case 0xE4:
      m_iVibratoType = argument & 7;
      break;
    case 0xE5:
      m_fFinetune = float(pow(pow(2.0f, 1.0f / 12.0f), 
			      (argument - 128) / 256.0f));
    case 0xE6:
      m_iPanType = argument & 7;
      break;
    case 0xE7:
      m_iTremoloType = argument & 7;
      break;
    case 0xE8:
      if (argument == 1) {
	m_oReverse = m_oReverse ? false : true;
	retrig |= RETRIG_FREQ;
      }
      break;
    case 0xEA:
      m_fVolume += float(argument) / 128.0f;
      if (m_fVolume > 2.0f)
	m_fVolume = 2.0f;
      m_fBaseVolume = m_fVolume;
      retrig |= RETRIG_VOLUME;
      break;
    case 0xEB:
      m_fVolume -= float(argument) / 128.0f;
      if (m_fVolume < 0.0f)
	m_fVolume = 0.0f;
      m_fBaseVolume = m_fVolume;
      retrig |= RETRIG_VOLUME;
      break;
      /*
	case 0xED:
	  if (argument) {
	    retrig &= ~(RETRIG_INS | RETRIG_INSLOOP);
	  }
	  break;
      */
    case 0xEE:
      m_fBasePan = m_fPan -= float(argument) / 255.0f * 2.0f;
      if (m_fPan < -1.0f)
	m_fPan = -1.0f;
      m_fBasePan = m_fPan;
      retrig |= RETRIG_VOLUME;
      break;
    case 0xEF:
      m_fPan += float(argument) / 255.0f * 2.0f;
      if (m_fPan > 1.0f)
	m_fPan = 1.0f;
      m_fBasePan = m_fPan;
      retrig |= RETRIG_VOLUME;
      break;
    case 0xF:
      if (argument)
	m_iSubDivide = argument;
      break;
    }
  }

  if (m_pChannel) {
    m_pChannel->set_filter_bypass(m_iFilterType > 1 ? false : true);
    m_pChannel->set_filter_mode(m_iFilterType == 2 ? 
				Svf::LOWPASS : 
				Svf::HIGHPASS);
  }

  m_iLastCommand = thesecommands;

  ProcessRetrig(retrig);
  Process(0);
}

void CTrack::ProcessRetrig(int retrig) {
  if (m_pChannel == NULL)
    return;

  if (m_pMachine->m_Attributes.iVolumeRamp) {
    m_pChannel->SetRampTime(int(m_pMachine->m_Attributes.iVolumeRamp * 
				m_pMachine->_master_info->samples_per_second / 
				1000.0f));
  }
  else {
    m_pChannel->SetRampTime(0);
  }

  if (retrig & RETRIG_VOLUME) {
    m_pChannel->SetVolumeAndPan(m_fVolume, m_fPan);
  }

  if (retrig & RETRIG_CUTOFF) {
    m_pChannel->set_filter_sampling_rate(m_pMachine->_master_info->
					 samples_per_second);
    m_pChannel->set_filter_cutoff(m_fCutoffFreq);
  }
  if (retrig & RETRIG_REZ) {
    if (m_fResonance < 1.0f)
      m_fResonance = 1.0f;
    if (m_fResonance > 25.0f)
      m_fResonance = 25.0f;
    m_pChannel->set_filter_sampling_rate(m_pMachine->_master_info->
					 samples_per_second);
    m_pChannel->set_filter_resonance((m_fResonance - 1.0) / 25.0);
  }

  if (retrig & RETRIG_INS) {
    if (m_pSample && m_pSample->IsValid() && m_pSample->IsStillValid()) {
      if ((rand() & 255) < m_iProbability) {
	m_pChannel->m_pSample = m_pSample;
	if (m_pSample->IsStereo())
	  m_pChannel->m_Resampler.m_Location.m_eFormat = SMP_SIGNED16_STEREO;
	else
	  m_pChannel->m_Resampler.m_Location.m_eFormat = SMP_SIGNED16;
	m_pChannel->m_Resampler.m_oPingPongLoop = m_pSample->IsPingPongLoop();
	m_pChannel->m_Resampler.m_oForward = true;
	switch (m_pMachine->m_Attributes.iFilterMode) {
	case 0:
	  m_pChannel->m_Resampler.m_Location.m_eFiltering = FILTER_NEAREST;
	  break;
	case 1:
	  m_pChannel->m_Resampler.m_Location.m_eFiltering = FILTER_LINEAR;
	  break;
	case 2:
	  m_pChannel->m_Resampler.m_Location.m_eFiltering = FILTER_SPLINE;
	  break;
	}
	m_pChannel->m_Resampler.m_Location.m_pStart = 
	  m_pSample->GetSampleStart();
	m_pChannel->m_Resampler.m_Location.m_pEnd = 
	  m_pSample->GetSampleStart();
	if (m_pSample->IsLoop()) {
	  m_pChannel->m_Resampler.m_Loop = m_pChannel->m_Resampler.m_Location;
	  m_pChannel->m_Resampler.m_Loop.m_pEnd = m_pSample->GetSampleStart();
	  m_pChannel->m_Resampler.m_Loop.AdvanceLocation(m_pSample->GetLoopStart());
	  m_pChannel->m_Resampler.m_Loop.AdvanceEnd(m_pSample->GetLoopEnd());
	  m_pChannel->m_Resampler.m_Location.AdvanceEnd(m_pSample->GetLoopEnd());
	}
	else {
	  m_pChannel->m_Resampler.m_Loop.m_pStart = NULL;
	  m_pChannel->m_Resampler.m_Location.AdvanceEnd(m_pSample->GetSampleLength());
	}
	if (m_iLoopStretch != -1 && m_iLoopStretch) {
	  m_fFreq = m_fBaseFreq = float(m_pSample->GetSampleLength()) / 
	    float(m_pMachine->_master_info->samples_per_tick * m_iLoopStretch);
	}
	m_pChannel->m_Resampler.m_iPosition = 0;
	if (m_iSlice > 0)
	  m_pChannel->m_Resampler.m_iPosition += 
	    m_pSample->GetSliceOffset(m_iSlice - 1);
	m_pChannel->m_Resampler.m_iPosition += 
	  (m_iSampleOffset * m_pSample->GetSampleLength()) >> 8;
	if (m_pChannel->m_Resampler.m_iPosition > m_pSample->GetSampleLength())
	  m_pChannel->m_Resampler.m_iPosition = m_pSample->GetSampleLength();
	m_pChannel->m_Resampler.m_iFraction = 0;
	m_pChannel->m_Amp.Retrig();
	m_pChannel->m_VolumeEnvelope.ReadEnvelope(m_pMachine->_host, 
						  m_iInstrument, 0);
	m_pChannel->m_VolumeEnvelope.Restart(1.0f / float(m_pMachine->_master_info->samples_per_tick * m_pMachine->m_Attributes.iVolumeEnvelopeTicks));
      }
      m_iProbability = 256;
    } else {
      m_pChannel->m_Resampler.m_Location.m_pStart = NULL;
    }
  }

  if (m_iLoopStretch != -1 && m_iLoopStretch && m_oLoopStretchTrack) {
    if (m_pChannel->m_pSample && 
	m_pChannel->m_pSample->IsValid() && 
	m_pChannel->m_pSample->IsStillValid()) {
      m_fFreq = m_fBaseFreq = float(m_pChannel->m_pSample->GetSampleLength()) / 
	float(m_pMachine->_master_info->samples_per_tick * m_iLoopStretch);
      retrig |= RETRIG_FREQ;
    }
  }

  if ((retrig & RETRIG_FREQ) || (m_pChannel->m_PitchEnvelope.IsValid())) {
    m_pChannel->m_Resampler.SetFrequency((m_oReverse ? -m_fFreq : m_fFreq) * 
					 m_fFinetune * 
					 m_pChannel->m_fPitchEnvFreq);
  }
}

int CTrack::DoVibrato() {
  float	depth = 0.0f;

  switch (m_iVibratoType & 0x3) {
  case 0:
    depth = float(sin(m_fVibratoPos));
    break;
  case 1:
    depth = m_fVibratoPos / float(M_PI) - 1.0f;
    break;
  case 2:
    depth = m_fVibratoPos >= float(M_PI) ? 1.0f : -1.0f;
    break;
  }
  depth *= m_fVibratoDepth;
  m_fFreq = m_fBaseFreq * float(pow(2.0f, depth));
  m_fVibratoPos += m_fVibratoSpeed;
  if (m_fVibratoPos >= float(2 * M_PI))
    m_fVibratoPos -= float(2 * M_PI);
  return RETRIG_FREQ;
}

int CTrack::DoCutoffLFO() {
  float	depth = 0.0f;

  switch (m_iCutoffLFOType & 0x3) {
  case 0:
    depth = float(sin(m_fCutoffLFOPos));
    break;
  case 1:
    depth = m_fCutoffLFOPos / float(M_PI) - 1.0f;
    break;
  case 2:
    depth = m_fCutoffLFOPos >= float(M_PI) ? 1.0f : -1.0f;
    break;
  }

  depth *= m_fCutoffLFODepth;
  m_fCutoffFreq = m_fBaseCutoffFreq * float(pow(2.0f, depth));
  m_fCutoffLFOPos += m_fCutoffLFOSpeed;
  if (m_fCutoffLFOPos >= float(2 * M_PI))
    m_fCutoffLFOPos -= float(2 * M_PI);

  return RETRIG_CUTOFF;
}

int CTrack::DoResonanceLFO() {
  float	depth = 0.0f;

  switch (m_iResonanceLFOType & 0x3) {
  case 0:
    depth = float(sin(m_fResonanceLFOPos));
    break;
  case 1:
    depth = m_fResonanceLFOPos / float(M_PI) - 1.0f;
    break;
  case 2:
    depth = m_fResonanceLFOPos >= float(M_PI) ? 1.0f : -1.0f;
    break;
  }

  depth *= m_fResonanceLFODepth;
  m_fResonance = m_fBaseResonance + depth;
  m_fResonanceLFOPos += m_fResonanceLFOSpeed;
  if (m_fResonanceLFOPos >= float(2 * M_PI))
    m_fResonanceLFOPos -= float(2 * M_PI);
  return RETRIG_REZ;
}

int CTrack::DoAutopan() {
  float	depth = 0.0f;

  switch (m_iPanType & 0x3) {
  case 0:
    depth = float(sin(m_fPanPos));
    break;
  case 1:
    depth = m_fPanPos / float(M_PI) - 1.0f;
    break;
  case 2:
    depth = m_fPanPos >= float(M_PI) ? 1.0f : -1.0f;
    break;
  }

  depth *= m_fPanDepth;
  m_fPan = m_fBasePan - depth;
  m_fPanPos += m_fPanSpeed;
  if (m_fPanPos >= float(2 * M_PI))
    m_fPanPos -= float(2 * M_PI);
  if (m_fPan < -1.0f)
    m_fPan = -1.0f;
  if (m_fPan > 1.0f)
    m_fPan = 1.0f;
  return RETRIG_VOLUME;
}

int CTrack::DoTremolo() {
  float	depth = 0;

  switch (m_iTremoloType & 0x3) {
  case 0:
    depth = float(sin(m_fTremoloPos));
    break;
  case 1:
    depth = m_fTremoloPos / float(2 * M_PI);
    break;
  case 2:
    depth = m_fTremoloPos >= float(M_PI) ? 1.0f : 0.0f;
    break;
  }

  depth = m_fBaseVolume - m_fBaseVolume * (1.0 - depth) * m_fTremoloDepth;

  if (depth > 2.0f)
    depth = 2.0f; 
  else if (depth < 0.0f)
    depth = 0.0f;

  m_fVolume = depth;
  m_fTremoloPos += m_fTremoloSpeed;
  if (m_fTremoloPos >= float(2 * M_PI))
    m_fTremoloPos -= float(2 * M_PI);

  return RETRIG_VOLUME;
}

int CTrack::DoToneport() {
  if (m_fWantedFreq > m_fFreq) {
    m_fFreq *= m_fToneportSpeed;
    if (m_fFreq > m_fWantedFreq)
      m_fFreq = m_fWantedFreq;
  } else if (m_fWantedFreq < m_fFreq) {
    if (m_fToneportSpeed)
      m_fFreq /= m_fToneportSpeed;

    if (m_fFreq < m_fWantedFreq)
      m_fFreq = m_fWantedFreq;
  }

  m_fBaseFreq = m_fFreq;

  return RETRIG_FREQ;
}

int CTrack::DoVolslide(int argument) {
  if (argument & 0xF0) {
    m_fVolume += float((argument & 0xF0) >> 4) / 128.0f;
    if (m_fVolume > 2.0f)
      m_fVolume = 2.0f;
    return RETRIG_VOLUME;
  } else if (argument & 0x0F) {
    m_fVolume -= float(argument & 0x0F) / 128.0f;
    if (m_fVolume < 0.0f)
      m_fVolume = 0.0f;
    return RETRIG_VOLUME;
  }
  return 0;
}

int CTrack::DoPanslide(int argument) {
  if (argument & 0xF0) {
    m_fPan -= float((argument & 0xF0) >> 4) / 255.0f * 2.0f;
    if (m_fPan < -1.0f)
      m_fPan = -1.0f;
    return RETRIG_VOLUME;
  } else if (argument & 0x0F) {
    m_fPan += float(argument & 0x0F) / 255.0f * 2.0f;
    if (m_fPan > 1.0f)
      m_fPan = 1.0f;
    return RETRIG_VOLUME;
  }
  return 0;
}

void CTrack::Process(int iStep) {
  if (m_pChannel == NULL)
    return;

  int retrig = 0;

  int i;
  for(i = 0; i < TOTALEFFECTS; i += 1) {
    int	command, argument;
    CEnvelope minenv;
    
    command = m_Vals.effects[i].command;
    argument = m_Vals.effects[i].argument;

    if (command == 0xFF)
      continue;

    switch (command) {
    case 0x0:
      if (argument != 0) {
	if (m_pChannel->m_pSample && m_pChannel->m_pSample->IsStillValid()) {
	  switch(iStep % 3) {
	  case 0:
	    m_fFreq = 
	      (float)pow(2.0f, 
			 (CONVNOTE(m_iBaseNote) - 
			  CONVNOTE(m_pChannel->m_pSample->GetRootNote())) / 
			 12.0f) * m_pChannel->m_pSample->GetRootFrequency() / 
	      m_pMachine->_master_info->samples_per_second;
	    break;
	  case 1:
	    m_fFreq = 
	      (float)pow(2.0f,
			 (CONVNOTE(m_iBaseNote) - 
			  CONVNOTE(m_pChannel->m_pSample->GetRootNote()) + 
			  (argument >> 4)) / 12.0f) * 
	      m_pChannel->m_pSample->GetRootFrequency() / 
	      m_pMachine->_master_info->samples_per_second;
	    break;
	  case 2:
	    m_fFreq = 
	      (float)pow(2.0f, (CONVNOTE(m_iBaseNote) - 
				CONVNOTE(m_pChannel->m_pSample->GetRootNote()) +
				(argument & 0x0F)) / 12.0f) * 
	      m_pChannel->m_pSample->GetRootFrequency() / 
	      m_pMachine->_master_info->samples_per_second;
	    break;
	  }
	  retrig |= RETRIG_FREQ;
	}
      }
      break;
    case 0x1:
      m_fFreq *= (float)pow(2.0f, float(argument) / FREQSCALE);
      m_fBaseFreq = m_fFreq;
      retrig |= RETRIG_FREQ;
      break;
    case 0x2:
      m_fFreq /= (float)pow(2.0f, float(argument) / FREQSCALE);
      m_fBaseFreq = m_fFreq;
      retrig |= RETRIG_FREQ;
      break;
    case 0x3:
      retrig |= DoToneport();
      break;
    case 0x4:
      retrig |= DoVibrato();
      break;
    case 0x5:
      retrig |= DoPanslide(argument);
      break;
    case 0x6:
      retrig |= DoAutopan();
      break;
    case 0x7:
      retrig |= DoTremolo();
      break;
    case 0xA:
      retrig |= DoVolslide(argument);
      break;
    case 0x15:
      if (m_iRandomDelay && (m_iRandomDelay == iStep)) {
	retrig = RETRIG_VOLUME|RETRIG_FREQ;
	retrig |= NewNote();
      }
      break;
    case 0x18:
      if (argument & 0xF0) {
	if ((((argument >> 4) <= (m_iSubDivide - 1)) ? 
	     (argument >> 4) : 
	     (m_iSubDivide - 1)) == iStep) {
	  retrig = RETRIG_VOLUME | RETRIG_FREQ;
	  retrig |= NewNote();
	}
      }
      if ((argument & 0xF) == iStep) {
	Release();
      }
      break;
    case 0x19:
      if (argument == 0x10 && ((argument & 0xF) == iStep)) {
	m_pMachine->m_oSustainAllReleases = true;
      }
      else if (((argument&0xF0) == 0x20) && 
	       m_pMachine->m_oSustainAllReleases && 
	       ((argument & 0xF) == iStep)) {
	m_pMachine->m_oSustainAllReleases = false;
	int i;
	for (i = 0; i < m_pMachine->numTracks; i += 1) {
	  if (m_pMachine->m_Tracks[i].m_oSustained) {
	    m_pMachine->m_Tracks[i].Release();
	    m_pMachine->m_Tracks[i].m_oSustained = false;
	  }
	}
      }
      break;
    case 0x21:
      // Cutoff slide up
      m_fCutoffFreq*=(float)pow(2.0f,float(argument)/FREQSCALE);
      m_fBaseCutoffFreq=m_fCutoffFreq;
      retrig|=RETRIG_CUTOFF;
      break;
    case 0x22:
      // Cutoff slide down
      m_fCutoffFreq *= (float)pow(2.0f, -float(argument) / FREQSCALE);
      m_fBaseCutoffFreq = m_fCutoffFreq;
      retrig |= RETRIG_CUTOFF;
      break;
    case 0x24:
      // Cutoff do lfo
      retrig |= DoCutoffLFO();
      break;
    case 0x29:
      // Resonance slide up
      m_fResonance += argument / 100.0f;
      m_fBaseResonance = m_fResonance;
      retrig |= RETRIG_REZ;
      break;
    case 0x2A:
      // Resonance slide down
      m_fResonance -= argument / 100.0f;
      m_fBaseResonance = m_fResonance;
      retrig |= RETRIG_REZ;
      break;
    case 0x2C:
      // Resonance do lfo
      retrig |= DoResonanceLFO();
      break;
    case 0xE9:
      if (argument && ((iStep % argument) == 0)) {
	retrig |= NewNote(true);
      }
      break;
    case 0xDC:
      if (argument == iStep) {
	Release();
      }
      break;
    case 0xEC:
      if (argument == iStep) {
	m_fVolume = 0;
	retrig |= RETRIG_VOLUME;
      }
      break;
    case 0xED:
      if (argument) {
	if (((argument <= (m_iSubDivide - 1)) ? 
	     argument : 
	     (m_iSubDivide - 1)) == iStep) {
	  if (m_Vals.note == zzub::note_value_off)
	    Release();
	  else
	    retrig |= NewNote();
	}
      }
      break;
    }
  }

  if ((!NONOTE(m_Vals.note)) && m_iAutoShuffleAmount && 
      (!m_oAutoShuffled) && m_iAutoShuffleSteps && 
      (m_iAutoShuffleCount % m_iAutoShuffleSteps)) {
    if (m_iAutoShuffleAmount * (m_iAutoShuffleCount % m_iAutoShuffleSteps) / 
	m_iAutoShuffleSteps * m_iSubDivide / 15 < iStep) {
      retrig |= NewNote();
      retrig |= RETRIG_VOLUME | RETRIG_FREQ;
      m_oAutoShuffled = true;
      m_pChannel->m_pSample = m_pSample;
    }
  }
  ProcessRetrig(retrig);
}

void CTrack::Stop() {
  if (m_pChannel) {
    m_pChannel->Free();
    m_pChannel = NULL;
  }
  m_iAutoShuffleAmount = 0;
  m_oAvailableForMIDI = true;
}

int CTrack::GetWaveEnvPlayPos(const int env) {
  if (m_pChannel)
    return m_pChannel->GetWaveEnvPlayPos(env);
  return -1;
}

