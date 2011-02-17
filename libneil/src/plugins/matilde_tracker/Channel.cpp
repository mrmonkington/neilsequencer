// Channel.cpp: implementation of the CChannel class.
//
//////////////////////////////////////////////////////////////////////

#include "../SurfsDSPLib/SRF_DSP.h"
#include "Envelope.h"
#include "zzub/plugin.h"
#include "Channel.h"
#include "Tracker.h"
#include <math.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CChannel::CChannel() {
  m_pOwner = 0;
  Reset();
  m_oFree = true;
}

CChannel::~CChannel() {

}

void CChannel::Free() {
  if (m_pOwner)
    m_pOwner->m_pChannel = 0;
  m_oFree = true;
  m_pOwner = 0;
}

bool CChannel::Release() {
  bool r = false;
  if (!m_VolumeEnvelope.IsValid()) {
    if ((!m_pMachine->m_oVirtualChannels) || 
	(m_pSample && (m_pSample->IsLoop()|| m_pSample->IsPingPongLoop()))) {
      if (m_pSample) {
	m_pSample->Free();
	m_pSample = 0;
      }
      Reset();
      r = true;
    }
  } else {
    m_VolumeEnvelope.Release();
  }
  m_PanningEnvelope.Release();
  m_PitchEnvelope.Release();
  return r;
}

void CChannel::Reset() {
  m_Resampler.Reset();
  m_Amp.Reset();
  m_VolumeEnvelope.Reset();
  m_PanningEnvelope.Reset();
  m_PitchEnvelope.Reset();
  m_pSample=0;
  // Reset the filters.
  for (int i = 0; i < 3; i++) {
    filters[i].reset();
  }
  Free();
}

void CChannel::SetRampTime(int iRamp) {
  m_Resampler.m_iRampTime = iRamp;
  m_Amp.m_iRampTime = iRamp;
  // Set declick time for the filters the same as for the amp.
  for (int i = 0; i < 3; i++) {
    filters[i].set_inertia(iRamp);
  }
}

bool CChannel::Generate_Add(float **psamples, int numsamples) {
  if ((!m_VolumeEnvelope.HasEnded()) && 
      m_Resampler.Active() && m_pSample && m_pSample->IsStillValid()) {
    if (m_PitchEnvelope.IsValid())
      m_fPitchEnvFreq = float(pow(float(pow(2.0f, float(m_pMachine->m_Attributes.iPitchEnvelopeDepth) * (1.0f / 12.0f))), m_PitchEnvelope.GetCurrentLevel(numsamples) * 2.0f - 1.0f));
    else
      m_fPitchEnvFreq = 1.0f;

      float l = m_VolumeEnvelope.GetCurrentLevel(numsamples);
      float pan;
	       
      if (m_PanningEnvelope.IsValid()) {
	pan = (m_PanningEnvelope.GetCurrentLevel(numsamples) * 2.0f - 1.0f) + 
	  m_fPan;
	if (pan < -1.0f)
	  pan =- 1.0f;
	else if (pan > 1.0f)
	  pan = 1.0f;
      }
      else
	pan = m_fPan;

      if (m_pSample)
	m_Amp.SetVolume(l * m_fVolume * m_pSample->GetVolume() * (1.0f - pan), 
			l * m_fVolume * m_pSample->GetVolume() * (1.0f + pan));
      else
	m_Amp.SetVolume(l * m_fVolume * (1.0f - pan), 
			l * m_fVolume * (1.0f + pan));
      if (m_Amp.Active()) {
	float **paux = m_pMachine->_host->get_auxiliary_buffer();
	// Just a small optimization, 
	// the resampler and amp handle mono->stereo just fine
	if (m_Resampler.m_Location.m_eFormat >= 4 || 
	    m_Resampler.m_Loop.m_eFormat >= 4) {
	  m_Resampler.ResampleToStereoFloatBuffer(paux, numsamples);
	  filters[1].process(paux[0], paux[0], numsamples);
	  filters[2].process(paux[1], paux[1], numsamples);
	  m_Amp.AmpAndAdd_StereoToStereo(psamples, paux, numsamples, 1.0f);
	}
	else {
	  m_Resampler.ResampleToFloatBuffer(paux[0], numsamples);
	  filters[0].process(paux[0], paux[0], numsamples);
	  m_Amp.AmpAndAdd_ToStereo(psamples, paux[0], numsamples, 1.0f);
	}
	return true;
      } else {
	m_Resampler.Skip(numsamples);
	return false;
      }
    } else if (m_pSample) {
    m_pSample->Free();
    m_pSample = 0;
  }

  if (m_pOwner == 0)
    Free();
  return false;
}

bool CChannel::Generate_Move(float **psamples, int numsamples) {
  if ((!m_VolumeEnvelope.HasEnded()) && m_Resampler.Active() && 
      m_pSample && m_pSample->IsStillValid()) {
    if (m_PitchEnvelope.IsValid())
      m_fPitchEnvFreq = float(pow(float(pow(2.0f, float(m_pMachine->m_Attributes.iPitchEnvelopeDepth) * (1.0f / 12.0f))), m_PitchEnvelope.GetCurrentLevel(numsamples) * 2.0f - 1.0f));
    else
      m_fPitchEnvFreq = 1.0f;
    
    float l = m_VolumeEnvelope.GetCurrentLevel(numsamples);
    float pan;
		
    if (m_PanningEnvelope.IsValid()) {
      pan = (m_PanningEnvelope.GetCurrentLevel(numsamples) * 2.0f - 1.0f) + 
	m_fPan;
      if (pan < -1.0f)
	pan = -1.0f;
      else if (pan > 1.0f)
	pan = 1.0f;
    } else {
      pan = m_fPan;
    }

    if (m_pSample)
      m_Amp.SetVolume(l * m_fVolume * m_pSample->GetVolume() * (1.0f - pan), 
		      l * m_fVolume * m_pSample->GetVolume() * (1.0f + pan));
    else
      m_Amp.SetVolume(l * m_fVolume * (1.0f - pan), 
		      l * m_fVolume * (1.0f + pan));
      /*
	if (m_pWaveInfo)
	m_Amp.SetVolume( l*m_fVolume*m_pWaveInfo->Volume*(1.0f-pan), l*m_fVolume*m_pWaveInfo->Volume*(1.0f+pan) );
	else
	m_Amp.SetVolume( l*m_fVolume*(1.0f-pan), l*m_fVolume*(1.0f+pan) );
      */

    if (m_Amp.Active()) {
      float **paux = m_pMachine->_host->get_auxiliary_buffer();
      // Just a small optimization, 
      // the resampler and amp handle mono->stereo just fine
      if(m_Resampler.m_Location.m_eFormat >= 4 || 
	 m_Resampler.m_Loop.m_eFormat >= 4) {
	m_Resampler.ResampleToStereoFloatBuffer(paux, numsamples);
	filters[1].process(paux[0], paux[0], numsamples);
	filters[2].process(paux[1], paux[1], numsamples);
	m_Amp.AmpAndMove_StereoToStereo(psamples, paux, numsamples, 1.0f);
      }
      else {
	m_Resampler.ResampleToFloatBuffer(paux[0], numsamples);
	filters[0].process(paux[0], paux[0], numsamples);
	m_Amp.AmpAndMove_ToStereo( psamples, paux[0], numsamples, 1.0f );
      }
      return true;
    }
    else {
      m_Resampler.Skip(numsamples);
      return false;
    }
  }
  else if (m_pSample) {
      m_pSample->Free();
      m_pSample = 0;
  }

  if (m_pOwner == 0)
    Free();
  return false;
}

int CChannel::GetWaveEnvPlayPos(const int env) {
  switch (env) {
  case 0:
    return m_VolumeEnvelope.GetPlayPos();
  case 1:
    return m_PanningEnvelope.GetPlayPos();
  case 2:
    return m_PitchEnvelope.GetPlayPos();
  default:
    return 0;
  }
}
