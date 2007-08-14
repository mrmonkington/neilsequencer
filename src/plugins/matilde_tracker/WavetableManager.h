// WavetableManager.h: interface for the CWavetableManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WAVETABLEMANAGER_H__6DB4E742_F3EB_4F41_B083_CEF88343DD24__INCLUDED_)
#define AFX_WAVETABLEMANAGER_H__6DB4E742_F3EB_4F41_B083_CEF88343DD24__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class	IInstrument;
class	CMatildeTrackerMachine;

#include	"BuzzInstrument.h"
#include 	"zzub/plugin.h"

class	CWavetableManager  
{
	friend class CBuzzInstrument;
public:
						CWavetableManager();
						~CWavetableManager();

	IInstrument		*	GetInstrument( int iNum );
	void				SetTracker( CMatildeTrackerMachine *pTrk );
	void				Stop();

	int					GetUsedSamples();

protected:
	CBuzzSample		*	AllocBuzzSample();
	
	CMatildeTrackerMachine		*	m_pTracker;

	CBuzzInstrument		m_BuzzInstruments[zzub::wavetable_index_value_max];
	CBuzzSample			m_BuzzSamples[MAX_BUZZSAMPLES];
	int					m_iNextFreeBuzzSample;
};

#endif // !defined(AFX_WAVETABLEMANAGER_H__6DB4E742_F3EB_4F41_B083_CEF88343DD24__INCLUDED_)
