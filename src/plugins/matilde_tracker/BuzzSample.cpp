// BuzzSample.cpp: implementation of the CBuzzSample class.
//
//////////////////////////////////////////////////////////////////////

#include	"BuzzSample.h"
#include	"BuzzInstrument.h"
#include 	"zzub/plugin.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBuzzSample::CBuzzSample() : m_oUsed(false)
{

}

CBuzzSample::~CBuzzSample()
{

}

float		CBuzzSample::GetVolume()
{
	return m_pInstrument->m_pWaveInfo->volume;
}

int			CBuzzSample::GetRootNote()
{
	return m_pWaveLevel->root_note;
}

int			CBuzzSample::GetRootFrequency()
{
	return m_pWaveLevel->samples_per_second;
}

bool		CBuzzSample::IsValid()
{
	return m_iSavedNumSamples && m_pSavedSamples;
}

bool		CBuzzSample::IsStereo()
{
	return (m_pInstrument->m_pWaveInfo->flags&zzub::wave_flag_stereo)?true:false;
}

bool		CBuzzSample::IsPingPongLoop()
{
	return ((m_pInstrument->m_pWaveInfo->flags&zzub::wave_flag_pingpong)?true:false) && m_pWaveLevel->loop_end>m_pWaveLevel->loop_start;
}

bool		CBuzzSample::IsLoop()
{
	return ((m_pInstrument->m_pWaveInfo->flags&zzub::wave_flag_loop)?true:false) && m_pWaveLevel->loop_end>m_pWaveLevel->loop_start;
}

void	*	CBuzzSample::GetSampleStart()
{
	return m_pWaveLevel->samples;
}

long		CBuzzSample::GetSampleLength()
{
	return m_pWaveLevel->sample_count;
}

long		CBuzzSample::GetSliceOffset(int index)
{
	if ((index < 0)||(index >= m_pWaveLevel->slices.size()))
		return 0;
	long offset = m_pWaveLevel->slices[index];
	if ((offset < 0)||(offset >= m_pWaveLevel->sample_count))
		return 0;
	return offset;
}

long		CBuzzSample::GetLoopStart()
{
	return m_pWaveLevel->loop_start;
}

long		CBuzzSample::GetLoopEnd()
{
	return m_pWaveLevel->loop_end;
}

bool		CBuzzSample::IsStillValid()
{
	return m_pInstrument->IsSampleStillValid( this );
}

void		CBuzzSample::Free()
{
	m_iSavedNumSamples=0;
	m_pSavedSamples=0;
	m_oUsed=false;
}
