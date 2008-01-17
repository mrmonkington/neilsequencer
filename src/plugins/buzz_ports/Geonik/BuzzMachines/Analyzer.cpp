/*
 *		Analyzer plug-in for Buzz
 *
 *			Written by George Nicolaidis aka Geonik
 */

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

#include <windows.h>
#include "Analyzer.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <float.h>

#include "MachineInterface.h"

#include "../DspClasses/DspClasses.h"
#include "../DspClasses/Volume.h"

#include "../Common/Prefs.h"
#include "../Common/Shared.h"
#include "../Common/Dialog.h"

typedef unsigned long  ulong;
typedef unsigned short uword;
typedef unsigned char  ubyte;

#define c_strName			"Analyzer"
#define c_strShortName		"Analyzer"

#define c_numControls		1
#define c_numAttributes		1
#define c_maxAmp			32768.0


/*
 *		Declarations and globals
 */

struct				 CMachine;
struct				 CADialog;

int					 dspcSampleRate;
CMachineInterface	*dspcMachine;


/*
 *		Parameters
 */

CMachineParameter const	mpC0		= {	pt_byte,"Dummy","Dummy",0,0x80,0xFF,MPF_STATE,0x00 };
CMachineParameter const	mpC1		= {	pt_byte,"Dummy","Dummy",0,0x80,0xFF,MPF_STATE,0x00 };
CMachineParameter const	mpC2		= {	pt_byte,"Dummy","Dummy",0,0x80,0xFF,MPF_STATE,0x00 };

CMachineParameter const *mpArray[]	= { &mpC0,&mpC1,&mpC2 };
enum					 mpValues	  { mpvC0_ };
int const				 mpvC0		=   mpvC0_;

#pragma pack(1)		
struct CGlobalParameters			  { byte C[c_numControls]; };
struct CParameters					  { };
#pragma pack()


/*
 *		Attributes
 */

CMachineAttribute const  maA0		= { "Dummy",0,100,0 };
CMachineAttribute const *maArray[]	= { &maA0 };

#pragma pack(1)		
struct CAttributes					  { int A[c_numAttributes]; };
#pragma pack()


/*
 *		CMachineInfo
 */

CMachineInfo const miMachineInfo = { 
	MT_GENERATOR,MI_VERSION,0,
	0,0,0/*c_numControls*/,0,mpArray,0/*c_numAttributes*/,maArray,
	"Geonik's " c_strName,c_strShortName,"George Nicolaidis aka Geonik","Analyze...\nInfo...\nAbout..." };

enum miCommands { micOpen,micInfo,micAbout };

enum miTags { mitRefreshRate = 1, mitAveraging };

/*
 *		General purpose functions
 */

double ControlByteToDouble(int const,byte const);	// Convert control byte


/*
 *		Main Dialog declaration
 */

struct CAnalyzerDialog : public CGnDialog {
	int		iUpdateCount;
	int		iRefreshRate;
	int		iAveraging;
	double  fRmsAvg;
	double  fAmpAvg;
	double	fSumAvg;

		 CAnalyzerDialog();
	void Init(CPrefsLoad * const);
	void Save(CPrefsSave * const);
	void Update();

	void wm_InitDialog(void * const);
	int	 wm_Command(int iID, int iNotifyCode, void *hCtrl); };


/*
 *		Machine class
 */

struct CMachine : public CMachineInterface {
				 CMachine();
				~CMachine();
	void		 Init(CMachineDataInput * const pi);
	void		 Save(CMachineDataOutput * const po);
	void		 SetNumTracks(int const n);
	void		 AttributesChanged();
	char const	*DescribeValue(int const param, int const value);
	void		 Tick();
	bool		 Work(float *psamples, int numsamples, int const Mode);
	void		 Stop();
	void		 Command(int const);
	void		 ControlChange(int const,double const);
	void		 About();

	CSharedResource		 cSharedResource;
	CGlobalParameters	 cGlobalParameters;
	CAttributes			 cAttributes;

	byte				 bControl[c_numControls];
	double				 fControl[c_numControls];

	// Add members here

	CRms				 cRms;
	CGnDialog			 cInfoDialog;
	CAnalyzerDialog		 cAnalyzerDialog;
	float				 fMinimum;
	float				 fMaximum;
	double				 fSum;
 };


/*
 *		Dll Exports
 */

extern "C" {
__declspec(dllexport) CMachineInfo const * __cdecl GetInfo() {
	return &miMachineInfo; }
__declspec(dllexport) CMachineInterface * __cdecl CreateMachine() {
	return new CMachine; } }

void	*hDllModule;

bool __stdcall DllMain(void *hModule,unsigned long ul_reason_for_call,void *lpReserved) {
	hDllModule = hModule;
    return true; }


/*
 *		Main Dialog implementation
 */

CAnalyzerDialog::CAnalyzerDialog(): CGnDialog() {
	fRmsAvg			= 0;
	fAmpAvg			= 0;
	fSumAvg			= 0;
	iUpdateCount	=-1; }

void CAnalyzerDialog::Init(CPrefsLoad * const pl) {
	CGnDialog::Init(pl);
	iRefreshRate	= pl->LoadInt(mitRefreshRate,0);
	iAveraging		= pl->LoadInt(mitAveraging,1); }

void CAnalyzerDialog::Save(CPrefsSave * const ps) {
	CGnDialog::Save(ps);
	ps->SaveInt(mitRefreshRate,iRefreshRate);
	ps->SaveInt(mitAveraging,iAveraging); }

void CAnalyzerDialog::wm_InitDialog(void * const hDlg) {
	CheckMenuRadioItem(GetMenu(hDlg),ID_REFRESHRATE,ID_REFRESHRATE+2,ID_REFRESHRATE + iRefreshRate,MF_BYCOMMAND);
	CheckMenuRadioItem(GetMenu(hDlg),ID_AVERAGING,ID_AVERAGING+2,ID_AVERAGING + iAveraging,MF_BYCOMMAND); }

int	 CAnalyzerDialog::wm_Command(int iId, int iNotifyCode, void *hCtrl)	{
	switch(iId) {
	case ID_REFRESHRATE:
	case ID_REFRESHRATE+1:
	case ID_REFRESHRATE+2:
		CheckMenuRadioItem(GetMenu(hDialog),ID_REFRESHRATE,ID_REFRESHRATE+2,iId,MF_BYCOMMAND);
		iRefreshRate = iId - ID_REFRESHRATE;
		return true;
	case ID_AVERAGING:
	case ID_AVERAGING+1:
	case ID_AVERAGING+2:
		CheckMenuRadioItem(GetMenu(hDialog),ID_AVERAGING,ID_AVERAGING+2,iId,MF_BYCOMMAND);
		iAveraging = iId - ID_AVERAGING;
		return true;
	case IDABOUT:
		((CMachine *)dspcMachine)->About();
		return true; }
	return false; }

#define _Machine	((CMachine *)dspcMachine)

void CAnalyzerDialog::Update() {
	static double const tAveraging[3] = { 0.85,0.92,0.97 };
	static char b[64];
	double const avg   = tAveraging[iAveraging];
	double const avg1m = 1.0 - avg;
	double t;

	iUpdateCount++;

	double rv = (1.281/c_maxAmp)*sqrt(_Machine->cRms.fQ);
	fRmsAvg = fRmsAvg * avg + rv * avg1m;

	double amp = _Machine->fMaximum - _Machine->fMinimum;
	fAmpAvg = fAmpAvg * avg + amp * avg1m;
	_Machine->fMaximum = _Machine->fMinimum = 0;

	double dc = fabs(_Machine->fSum / (double)_Machine->pMasterInfo->SamplesPerTick);
	fSumAvg = fSumAvg * avg + dc * avg1m;
	_Machine->fSum = 0;

	if(!(iUpdateCount & ((1 << iRefreshRate) - 1)))  {
		t = 10.0 * log(rv);
		if(t <= -100) SetWindowText(GetDlgItem(hDialog,IDC_RMS),"-inf dB");
		else {
			sprintf(b,"%.2f dB",t);
			SetWindowText(GetDlgItem(hDialog,IDC_RMS),b); }
		sprintf(b,"%.1f %%",(100.0/65536.0)*amp);
		SetWindowText(GetDlgItem(hDialog,IDC_AMP),b);
		sprintf(b,"%.1f",dc);
		SetWindowText(GetDlgItem(hDialog,IDC_DC),b); }

	if(!(iUpdateCount & 7)) {
		t = 10.0 * log(fRmsAvg);
		if(t <= -100) SetWindowText(GetDlgItem(hDialog,IDC_RMS10),"-inf dB");
		else {
			sprintf(b,"%.2f dB",t);
			SetWindowText(GetDlgItem(hDialog,IDC_RMS10),b); }
		sprintf(b,"%.1f %%",(100.0/65536.0)*fAmpAvg);
		SetWindowText(GetDlgItem(hDialog,IDC_AMP10),b);
		sprintf(b,"%.1f",fSumAvg);
		SetWindowText(GetDlgItem(hDialog,IDC_DC10),b); } }


/*
 *		Machine members
 */

CMachine::CMachine() {
	GlobalVals	= &cGlobalParameters;
	AttrVals	= (int *)&cAttributes;
 }

CMachine::~CMachine() {
 }

void CMachine::Init(CMachineDataInput * const pi) {
	dspcSampleRate	= pMasterInfo->SamplesPerSec;		// Dsp Classes
	dspcMachine		= this;
	for(int i=0; i<c_numControls; i++) {				// Controls
		CMachineParameter const *p = mpArray[i + mpvC0];
		bControl[i] = p->DefValue;
		fControl[i] = ControlByteToDouble(i,bControl[i]); }
	// Add custom initialization here

	cRms.Configure();
	cAnalyzerDialog.Setup(hDllModule,IDD_MAIN,NULL);
	cInfoDialog.Setup(hDllModule,IDD_INFO,NULL);

{	CPrefsLoad pl(pi);
	cAnalyzerDialog.Init(&pl); }

	fMaximum = 0;
	fMinimum = 0;
	fSum	 = 0;
 }

void CMachine::Save(CMachineDataOutput * const po) {
	CPrefsSave ps(po);
	cAnalyzerDialog.Save(&ps);
 }

void CMachine::Command(int const i) {
	switch(i) {
	case micOpen:
		cAnalyzerDialog.Open();
		break;
	case micInfo:
		cInfoDialog.Open();
		break;
	case micAbout:
		About();
		break; }
 }

#include "../Common/About.h"

void CMachine::SetNumTracks(int const n) {
 }

char const *CMachine::DescribeValue(int const pc, int const iv) {
	static char b[16];
	switch(pc) {
	case 0:
		// Add case statements
	default:
		return NULL; }
	return b;
 }

void CMachine::Tick() {

	cAnalyzerDialog.Update();
	cInfoDialog.Update();

/*	for(int i=0; i<c_numControls; i++) {
		if(cGlobalParameters.C[i] != 0xff) {				// Waring: Hardcoded novalue
			bControl[i] = cGlobalParameters.C[i];
			fControl[i] = ControlByteToDouble(i,bControl[i]);
			ControlChange(i,fControl[i]); } } */
 }

void CMachine::AttributesChanged() {
 }

void CMachine::ControlChange(int const cc, double const v) {
	switch(cc) {
	case 0:
//		cRms.Configure(v*100.0+1.0);
		break;
		// Add case statements
	default:
		break; }
 }

void CMachine::Stop() {
 }


/*
 *		General purpose functions
 */

double ControlByteToDouble(int const cc, byte const b) {
	switch(cc) {
	case 0:
		// Add case statements
	default:
		return (double)b * (1.0 / (double)(mpArray[mpvC0+cc]->MaxValue)); }
 }


/*
 *		Worker functions
 */

#pragma optimize ("a", on)

bool CMachine::Work(float *pb, int ns, int const mode) {

	if(!(mode & WM_READ)) {
		cRms.fQ = 0;
		if(mode == WM_NOIO) return true;
		else return false; }

	cRms.WorkSamples(pb,ns);

	double t1 = fMaximum;
	double t2 = fMinimum;
	double sum= fSum;

	do {
		float t = *pb++;
		sum += t;
		if(t > t1) t1 = t;
		else
		if(t < t2) t2 = t;
	} while(--ns);

	fMaximum = (float)t1;
	fMinimum = (float)t2;
	fSum	 = sum;

	return true;
 }

