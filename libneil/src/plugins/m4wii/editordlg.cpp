// EditorDlg.cpp : implementation file
//

// UEG Dest, UEnvMod, sustain levels, FENV mod
// tick marks
// Parameter Describe box
//#include "stdafx.h"
//#include "m4.h"


#include <afxwin.h>
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>
#include <afxmt.h>
#include <afxext.h> 		// MFC extensions
#include <afxcview.h>
#include <afxdisp.h>
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <mmsystem.h>
#include <process.h>


#include "resource.h"
#include "m4.h"

#include "../MachineInterface.h"
#include "EditorDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// EditorDlg dialog


EditorDlg::EditorDlg(CWnd* pParent /*=NULL*/)
	: CDialog(EditorDlg::IDD, pParent)
{

	//{{AFX_DATA_INIT(EditorDlg)
	//}}AFX_DATA_INIT
}


void EditorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(EditorDlg)
	DDX_Control(pDX, IDC_MONO, m_Mono);
	DDX_Control(pDX, IDC_PHASEDIFF2, m_PhaseDiff2);
	DDX_Control(pDX, IDC_NOTE, m_Note);
	DDX_Control(pDX, IDC_INFOBAR, m_InfoBar);
	DDX_Control(pDX, IDC_SUBOSCPIC, m_SubOscPic);
	DDX_Control(pDX, IDC_OSC2PIC, m_Osc2Pic);
	DDX_Control(pDX, IDC_OSC1PIC, m_Osc1Pic);
	DDX_Control(pDX, IDC_LFO2PIC, m_LFO2Pic);
	DDX_Control(pDX, IDC_LFO1PIC, m_LFO1Pic);
	DDX_Control(pDX, IDC_MODDEST2, m_ModDest2);
	DDX_Control(pDX, IDC_MODDEST1, m_ModDest1);
	DDX_Control(pDX, IDC_MODAMOUNT2, m_ModAmount2);
	DDX_Control(pDX, IDC_MODAMOUNT1, m_ModAmount1);
	DDX_Control(pDX, IDC_LFO2PHASE, m_LFO2Phase);
	DDX_Control(pDX, IDC_LFO2FREQ, m_LFO2Freq);
	DDX_Control(pDX, IDC_LFO2DEST, m_LFO2Dest);
	DDX_Control(pDX, IDC_LFO2AMOUNT, m_LFO2Amount);
	DDX_Control(pDX, IDC_LFO1PHASE, m_LFO1Phase);
	DDX_Control(pDX, IDC_LFO1FREQ, m_LFO1Freq);
	DDX_Control(pDX, IDC_LFO1DEST, m_LFO1Dest);
	DDX_Control(pDX, IDC_LFO1AMOUNT, m_LFO1Amount);
	DDX_Control(pDX, IDC_AMPGAIN, m_AmpGain);
	DDX_Control(pDX, IDC_DIST, m_Dist);
	DDX_Control(pDX, IDC_INITLFO, m_InitLFO);
	DDX_Control(pDX, IDC_INITFILTER, m_InitFilt);
	DDX_Control(pDX, IDC_INITAMP, m_InitAmp);
	DDX_Control(pDX, IDC_UENVMOD, m_UEnvMod);
	DDX_Control(pDX, IDC_UEGDEST, m_UEGDest);
	DDX_Control(pDX, IDC_UEG_SL, m_UEGSustainLevel);
	DDX_Control(pDX, IDC_UEG_S, m_UEGSustainTime);
	DDX_Control(pDX, IDC_UEG_R, m_UEGReleaseTime);
	DDX_Control(pDX, IDC_UEG_D, m_UEGDecayTime);
	DDX_Control(pDX, IDC_UEG_A, m_UEGAttackTime);
	DDX_Control(pDX, IDC_FILT_SL, m_FEGSustainLevel);
	DDX_Control(pDX, IDC_FILT_S, m_FEGSustainTime);
	DDX_Control(pDX, IDC_FILT_R, m_FEGReleaseTime);
	DDX_Control(pDX, IDC_FILT_D, m_FEGDecayTime);
	DDX_Control(pDX, IDC_FILT_A, m_FEGAttackTime);
	DDX_Control(pDX, IDC_AMP_S, m_AEGSustainTime);
	DDX_Control(pDX, IDC_AMP_R, m_AEGReleaseTime);
	DDX_Control(pDX, IDC_AMP_D, m_AEGDecayTime);
	DDX_Control(pDX, IDC_AMP_A, m_AEGAttackTime);
	DDX_Control(pDX, IDC_RESONANCE, m_Resonance);
	DDX_Control(pDX, IDC_CUTOFF, m_Cutoff);
	DDX_Control(pDX, IDC_FENVMOD, m_FEnvMod);
	DDX_Control(pDX, IDC_AMP_SL, m_AEGSustainLevel);
	DDX_Control(pDX, IDC_MIX, m_Mix);
	DDX_Control(pDX, IDC_MIXTYPE, m_MixType);
	DDX_Control(pDX, IDC_FIXEDPITCH, m_FixedPitch);
	DDX_Control(pDX, IDC_SYNCCHECK, m_Sync);
	DDX_Control(pDX, IDC_PORTASLIDER, m_Glide);
	DDX_Control(pDX, IDC_WAVETABLESLIDER, m_WavetableOsc);
	DDX_Control(pDX, IDC_WAVESLIDER2, m_Wave2);
	DDX_Control(pDX, IDC_WAVESLIDER1, m_Wave1);
	DDX_Control(pDX, IDC_SUBOSCWAVE, m_SubOscWave);
	DDX_Control(pDX, IDC_SUBOSCVOL, m_SubOscVol);
	DDX_Control(pDX, IDC_SEMISLIDER2, m_WaveDetuneSemi);
	DDX_Control(pDX, IDC_SEMISLIDER, m_DetuneSemi);
	DDX_Control(pDX, IDC_FINESLIDER, m_DetuneFine);
	DDX_Control(pDX, IDC_FILTERTYPE, m_FilterType);
	DDX_Control(pDX, IDC_BENDSEMISLIDER, m_PitchBendAmt);
	DDX_Control(pDX, IDC_PWSLIDER2, m_PulseWidth2);
	DDX_Control(pDX, IDC_PWSLIDER1, m_PulseWidth1);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(EditorDlg, CDialog)
	//{{AFX_MSG_MAP(EditorDlg)
	ON_BN_CLICKED(IDC_BUTTON_INIT, OnButtonInit)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_SYNCCHECK, OnSynccheck)
	ON_BN_CLICKED(IDC_FIXEDPITCH, OnFixedpitch)
	ON_WM_SHOWWINDOW()
	ON_WM_VSCROLL()
	ON_BN_CLICKED(IDC_INITAMP, OnInitamp)
	ON_BN_CLICKED(IDC_INITLFO, OnInitlfo)
	ON_BN_CLICKED(IDC_INITFILTER, OnInitfilter)
	ON_BN_CLICKED(IDC_LFO1PIC, OnLfo1pic)
	ON_BN_CLICKED(IDC_LFO2PIC, OnLfo2pic)
	ON_WM_MEASUREITEM()
	ON_WM_DRAWITEM()
	ON_BN_CLICKED(IDC_PLAYBUTTON, OnPlaybutton)
	ON_BN_CLICKED(IDC_STOPBUTTON, OnStopbutton)
	ON_BN_CLICKED(IDC_MONO, OnMono)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// EditorDlg message handlers

BEGIN_EVENTSINK_MAP(EditorDlg, CDialog)
    //{{AFX_EVENTSINK_MAP(EditorDlg)
	//}}AFX_EVENTSINK_MAP
END_EVENTSINK_MAP()

void EditorDlg::OnButtonInit() 
{
	// TODO: Add your control notification handler code here
	InitializeValues();	
}

void EditorDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	// TODO: Add your message handler code here and/or call default
	if(nSBCode == SB_ENDSCROLL)
	{
		CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
		return;
	}

	int id = pScrollBar->GetDlgCtrlID(), Pos;

	switch(id)
	{
	case IDC_BENDSEMISLIDER:
		Pos = m_PitchBendAmt.GetPos();
		SendBuzzParm(3, Pos);
		break;
	case IDC_PORTASLIDER:
		Pos = m_Glide.GetPos();
		SendBuzzParm(4, Pos);
		break;
	case IDC_WAVETABLESLIDER:
		Pos = m_WavetableOsc.GetPos();
		SendBuzzParm(5, Pos);
		break;
	case IDC_SEMISLIDER2:
		Pos = m_WaveDetuneSemi.GetPos();
		SendBuzzParm(7, Pos);
		break;
	case IDC_WAVESLIDER1:
		Pos = m_Wave1.GetPos();
		SendBuzzParm(8, Pos);
		UpdateOscPic(&m_Osc1Pic, Pos);
		break;
	case IDC_PWSLIDER1:
		Pos = m_PulseWidth1.GetPos();
		SendBuzzParm(9, Pos);
		break;
	case IDC_WAVESLIDER2:
		Pos = m_Wave2.GetPos();
		SendBuzzParm(10, Pos);
		UpdateOscPic(&m_Osc2Pic, Pos);
		break;
	case IDC_PWSLIDER2:
		Pos = m_PulseWidth2.GetPos();
		SendBuzzParm(11, Pos);
		break;
	case IDC_SEMISLIDER:
		Pos = m_DetuneSemi.GetPos();
		SendBuzzParm(12, Pos);
		break;
	case IDC_FINESLIDER:
		Pos = m_DetuneFine.GetPos();
		SendBuzzParm(13, Pos);
		break;
	case IDC_MIXTYPE:
		Pos = m_MixType.GetPos();
		SendBuzzParm(15, Pos);
		break;
	case IDC_MIX:
		Pos = m_Mix.GetPos();
		SendBuzzParm(16, Pos);
		break;
	case IDC_SUBOSCWAVE:
		Pos = m_SubOscWave.GetPos();
		SendBuzzParm(17, Pos);
		UpdateOscPic(&m_SubOscPic, Pos);
		break;
	case IDC_SUBOSCVOL:
		Pos = m_SubOscVol.GetPos();
		SendBuzzParm(18, Pos);
		break;
	case IDC_UENVMOD:
		Pos = m_UEnvMod.GetPos();
		SendBuzzParm(24, Pos);
		break;

	case IDC_FILTERTYPE:
		Pos = m_FilterType.GetPos();
		SendBuzzParm(30, Pos);
		break;

	case IDC_FENVMOD:
		Pos = m_FEnvMod.GetPos();
		SendBuzzParm(39, Pos);
		break;

	case IDC_CUTOFF:
		Pos = m_Cutoff.GetPos();
		SendBuzzParm(32, Pos);
		break;

	case IDC_RESONANCE:
		Pos = m_Resonance.GetPos();
		SendBuzzParm(33, Pos);
		break;
	case IDC_UEGDEST:
		Pos = m_UEGDest.GetPos();
		SendBuzzParm(50, Pos);
		break;

	case IDC_LFO1DEST:
		Pos = m_LFO1Dest.GetPos();
		SendBuzzParm(40, Pos);
		break;
	case IDC_LFO1FREQ:
		Pos = m_LFO1Freq.GetPos();
		SendBuzzParm(42, Pos);
		break;
	case IDC_LFO1AMOUNT:
		Pos = m_LFO1Amount.GetPos();
		SendBuzzParm(43, Pos);
		break;
	case IDC_LFO1PHASE:
		Pos = m_LFO1Phase.GetPos();
		SendBuzzParm(44, Pos);
		UpdateLFOPic(&m_LFO1Pic, LFO1Wave, Pos);
		break;

	case IDC_LFO2DEST:
		Pos = m_LFO2Dest.GetPos();
		SendBuzzParm(45, Pos);
		break;
	case IDC_LFO2FREQ:
		Pos = m_LFO2Freq.GetPos();
		SendBuzzParm(47, Pos);
		break;
	case IDC_LFO2AMOUNT:
		Pos = m_LFO2Amount.GetPos();
		SendBuzzParm(48, Pos);
		break;
	case IDC_LFO2PHASE:
		Pos = m_LFO2Phase.GetPos();
		SendBuzzParm(49, Pos);
		UpdateLFOPic(&m_LFO2Pic, LFO2Wave, Pos);
		break;

	case IDC_PHASEDIFF2:
		Pos = m_PhaseDiff2.GetPos();
		SendBuzzParm(51, Pos);
		UpdateOscPic(&m_Osc2Pic, m_Wave2.GetPos(), Pos);
		break;

	case IDC_MODDEST1:
		Pos = m_ModDest1.GetPos();
		SendBuzzParm(52, Pos);
		break;
	case IDC_MODAMOUNT1:
		Pos = m_ModAmount1.GetPos();
		SendBuzzParm(53, Pos);
		break;
	case IDC_MODDEST2:
		Pos = m_ModDest2.GetPos();
		SendBuzzParm(54, Pos);
		break;
	case IDC_MODAMOUNT2:
		Pos = m_ModAmount2.GetPos();
		SendBuzzParm(55, Pos);
		break;

	case IDC_AMPGAIN:
		Pos = m_AmpGain.GetPos();
		SendBuzzParm(56, Pos);
		break;

	}

	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void EditorDlg::OnSynccheck() 
{
	// TODO: Add your control notification handler code here

	SendBuzzParm(14, m_Sync.GetCheck());	
}

void EditorDlg::OnFixedpitch() 
{
						
	SendBuzzParm(6, m_FixedPitch.GetCheck());	
}

void EditorDlg::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CDialog::OnShowWindow(bShow, nStatus);
	
	// TODO: Add your message handler code here
	InitializeValues();	
}

void EditorDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	// TODO: Add your message handler code here and/or call default
	if(nSBCode == SB_ENDSCROLL)
	{
		CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
		return;
	}

	int id = pScrollBar->GetDlgCtrlID(), Pos;

	if(nSBCode == SB_THUMBTRACK || nSBCode == SB_THUMBPOSITION)
	{
		Pos = nPos;
	}
	else if(id != IDC_UEG_SL && id != IDC_FILT_SL && id != IDC_AMP_SL)
	{
		if(nSBCode == SB_PAGEUP)
		{
			Pos = pScrollBar->GetScrollPos() - 16;
		}
		else if(nSBCode == SB_PAGEDOWN)
		{
			Pos = pScrollBar->GetScrollPos() + 16;
		}
		else if(nSBCode == SB_LINEUP)
		{
			Pos = pScrollBar->GetScrollPos() - 1;
		}
		else if(nSBCode == SB_LINEDOWN)
		{
			Pos = pScrollBar->GetScrollPos() + 1;
		}
	}
		

	switch(id)
	{

	case IDC_UEG_A:
		SendBuzzParm(19, 127-Pos);
		m_UEGAttackTime.SetScrollPos(Pos);
		break;
	case IDC_UEG_D:
		SendBuzzParm(20, 127-Pos);
		m_UEGDecayTime.SetScrollPos(Pos);
		break;
	case IDC_UEG_S:
		SendBuzzParm(21, 128-Pos);
		m_UEGSustainTime.SetScrollPos(Pos);
		break;
	case IDC_UEG_R:
		SendBuzzParm(23, 127-Pos);
		m_UEGReleaseTime.SetScrollPos(Pos);
		break;
	case IDC_AMP_A:
		SendBuzzParm(25, 127-Pos);
		m_AEGAttackTime.SetScrollPos(Pos);
		break;
	case IDC_AMP_D:
		SendBuzzParm(26, 127-Pos);
		m_AEGDecayTime.SetScrollPos(Pos);
		break;
	case IDC_AMP_S:
		SendBuzzParm(27, 128-Pos);
		m_AEGSustainTime.SetScrollPos(Pos);
		break;
	case IDC_AMP_R:
		SendBuzzParm(29, 127-Pos);
		m_AEGReleaseTime.SetScrollPos(Pos);
		break;
	case IDC_FILT_A:		
		SendBuzzParm(34, 127-Pos);
		m_FEGAttackTime.SetScrollPos(Pos);
		break;
	case IDC_FILT_D:
		SendBuzzParm(35, 127-Pos);
		m_FEGDecayTime.SetScrollPos(Pos);
		break;
	case IDC_FILT_S:
		SendBuzzParm(36, 128-Pos);
		m_FEGSustainTime.SetScrollPos(Pos);
		break;
	case IDC_FILT_R:
		SendBuzzParm(38, 127-Pos);
		m_FEGReleaseTime.SetScrollPos(Pos);
		break;
	case IDC_UEG_SL:
		Pos = m_UEGSustainLevel.GetPos();
		SendBuzzParm(22, 127-Pos);
		break;
	case IDC_AMP_SL:
		Pos = m_AEGSustainLevel.GetPos();
		SendBuzzParm(28, 127-Pos);
		break;
	case IDC_FILT_SL:
		Pos = m_FEGSustainLevel.GetPos();
		SendBuzzParm(37, 127-Pos);
		break;
	}


	
	CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
}

void EditorDlg::SetPlayMode()
{
	int n = 0;

	if(m_Mono.GetCheck())
	{
		n = 8;
		m_InitAmp.SetCheck(0);
		m_InitLFO.SetCheck(0);
		m_InitFilt.SetCheck(0);
	}
	else
	{
		if(m_InitAmp.GetCheck())
			n += 1;
		if(m_InitLFO.GetCheck())
			n += 2;
		if(m_InitFilt.GetCheck())
			n += 4;
	}


	SendBuzzParm(0, n);
}

void EditorDlg::OnInitamp() 
{
	// TODO: Add your control notification handler code here
	SetPlayMode();
}

void EditorDlg::OnInitlfo() 
{
	// TODO: Add your control notification handler code here
	SetPlayMode();	
}

void EditorDlg::OnInitfilter() 
{
	// TODO: Add your control notification handler code here
	SetPlayMode();	
}


void EditorDlg::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct) 
{
	// TODO: Add your message handler code here and/or call default
	
	CDialog::OnMeasureItem(nIDCtl, lpMeasureItemStruct);
}

void EditorDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	// TODO: Add your message handler code here and/or call default
	
	
	if(nIDCtl == IDC_LFO1PIC)
		UpdateLFOPic(&m_LFO1Pic, LFO1Wave, m_LFO1Phase.GetPos());
	else if(nIDCtl == IDC_LFO2PIC)
		UpdateLFOPic(&m_LFO2Pic, LFO2Wave, m_LFO2Phase.GetPos());
	else if(nIDCtl == IDC_OSC1PIC)
		UpdateOscPic(&m_Osc1Pic, m_Wave1.GetPos());
	else if(nIDCtl == IDC_OSC2PIC)
		UpdateOscPic(&m_Osc2Pic, m_Wave2.GetPos(), m_PhaseDiff2.GetPos());
	else if(nIDCtl == IDC_SUBOSCPIC)
		UpdateOscPic(&m_SubOscPic, m_SubOscWave.GetPos());	
  
	CDialog::OnDrawItem(nIDCtl, lpDrawItemStruct);
}


void EditorDlg::OnMono() 
{
	// TODO: Add your control notification handler code here
	SetPlayMode();	
}
