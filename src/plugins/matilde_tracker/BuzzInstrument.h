// BuzzInstrument.h: interface for the CBuzzInstrument class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BUZZINSTRUMENT_H__95C23FB4_32FC_4C5F_AED2_DCE8801DF8C2__INCLUDED_)
#define AFX_BUZZINSTRUMENT_H__95C23FB4_32FC_4C5F_AED2_DCE8801DF8C2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "zzub/plugin.h"
#include "IInstrument.h"
#include "BuzzSample.h"

class	CMatildeTrackerMachine;
class	CBuzzSample;

#define	MAX_BUZZSAMPLES	128

class	CBuzzInstrument : public IInstrument  
{
	friend class			CWavetableManager;
	friend class			CBuzzSample;

public:
							CBuzzInstrument();
	virtual					~CBuzzInstrument();

	virtual	ISample		*	GetSample( int iNote );

protected:
	bool					IsSampleStillValid( CBuzzSample *pSample );

	int						m_iInsNum;
	CMatildeTrackerMachine			*	m_pTracker;
	const zzub::wave_info		*	m_pWaveInfo;
};

#endif // !defined(AFX_BUZZINSTRUMENT_H__95C23FB4_32FC_4C5F_AED2_DCE8801DF8C2__INCLUDED_)
