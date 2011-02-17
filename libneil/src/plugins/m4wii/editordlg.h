//{{AFX_INCLUDES()
//}}AFX_INCLUDES
#if !defined(AFX_EDITORDLG_H__ED0212E0_E319_11D3_94CF_00C0F030989B__INCLUDED_)
#define AFX_EDITORDLG_H__ED0212E0_E319_11D3_94CF_00C0F030989B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// EditorDlg.h : header file
//

class mi;

/////////////////////////////////////////////////////////////////////////////
// EditorDlg dialog

class EditorDlg : public CDialog
{
// Construction
public:
	EditorDlg(CWnd* pParent = NULL);   // standard constructor
	mi *p_mi;

// Dialog Data
	//{{AFX_DATA(EditorDlg)
	enum { IDD = IDD_DIALOG1 };
	CButton	m_Mono;
	CSpinButtonCtrl	m_PhaseDiff2;
	CSliderCtrl	m_Note;
	CStatic	m_InfoBar;
	CButton	m_SubOscPic;
	CButton	m_Osc2Pic;
	CButton	m_Osc1Pic;
	CButton	m_LFO2Pic;
	CButton	m_LFO1Pic;
	CSliderCtrl	m_ModDest2;
	CSliderCtrl	m_ModDest1;
	CSliderCtrl	m_ModAmount2;
	CSliderCtrl	m_ModAmount1;
	CSpinButtonCtrl	m_LFO2Phase;
	CSliderCtrl	m_LFO2Freq;
	CSliderCtrl	m_LFO2Dest;
	CSliderCtrl	m_LFO2Amount;
	CSpinButtonCtrl	m_LFO1Phase;
	CSliderCtrl	m_LFO1Freq;
	CSliderCtrl	m_LFO1Dest;
	CSliderCtrl	m_LFO1Amount;
	CSliderCtrl	m_AmpGain;
	CSliderCtrl	m_Dist;
	CButton	m_InitLFO;
	CButton	m_InitFilt;
	CButton	m_InitAmp;
	CSliderCtrl	m_UEnvMod;
	CSliderCtrl	m_UEGDest;
	CSliderCtrl	m_UEGSustainLevel;
	CScrollBar	m_UEGSustainTime;
	CScrollBar	m_UEGReleaseTime;
	CScrollBar	m_UEGDecayTime;
	CScrollBar	m_UEGAttackTime;
	CSliderCtrl	m_FEGSustainLevel;
	CScrollBar	m_FEGSustainTime;
	CScrollBar	m_FEGReleaseTime;
	CScrollBar	m_FEGDecayTime;
	CScrollBar	m_FEGAttackTime;
	CScrollBar	m_AEGSustainTime;
	CScrollBar	m_AEGReleaseTime;
	CScrollBar	m_AEGDecayTime;
	CScrollBar	m_AEGAttackTime;
	CSliderCtrl	m_Resonance;
	CSliderCtrl	m_Cutoff;
	CSliderCtrl	m_FEnvMod;
	CSliderCtrl	m_AEGSustainLevel;
	CSliderCtrl	m_Mix;
	CSliderCtrl	m_MixType;
	CButton	m_FixedPitch;
	CButton	m_Sync;
	CSliderCtrl	m_Glide;
	CSliderCtrl	m_WavetableOsc;
	CSliderCtrl	m_Wave2;
	CSliderCtrl	m_Wave1;
	CSliderCtrl	m_SubOscWave;
	CSliderCtrl	m_SubOscVol;
	CSliderCtrl	m_WaveDetuneSemi;
	CSliderCtrl	m_DetuneSemi;
	CSliderCtrl	m_DetuneFine;
	CSliderCtrl	m_FilterType;
	CSliderCtrl	m_PitchBendAmt;
	CSliderCtrl	m_PulseWidth2;
	CSliderCtrl	m_PulseWidth1;
	//}}AFX_DATA

	int LFO1Wave;
	int LFO2Wave;
	void SendBuzzParm(int parm, int value);
	void InitializeValues();		// set control extents and current values for paramters

	void UpdateLFOPic(CButton *pic, int wave, int phase=0);
	void UpdateOscPic(CButton *pic, int wave, int phase=0);
	void SetPlayMode();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(EditorDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(EditorDlg)
	afx_msg void OnButtonInit();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnSynccheck();
	afx_msg void OnFixedpitch();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnInitamp();
	afx_msg void OnInitlfo();
	afx_msg void OnInitfilter();
	afx_msg void OnLfo1pic();
	afx_msg void OnLfo2pic();
	afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnPlaybutton();
	afx_msg void OnStopbutton();
	afx_msg void OnMono();
	DECLARE_EVENTSINK_MAP()
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EDITORDLG_H__ED0212E0_E319_11D3_94CF_00C0F030989B__INCLUDED_)
