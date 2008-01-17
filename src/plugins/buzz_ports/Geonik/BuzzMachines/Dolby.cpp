/*
 *		Dolby Surround effect for Buzz
 *
 *			Written by George Nicolaidis aka Geonik
 */

//	Includes

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

#include <windows.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <float.h>

#include "Resource.h"
#include "MachineInterface.h"

#include "../DspLib/DspLib.h"

#include "../DspClasses/DspClasses.h"
#include "../DspClasses/Delay.h"
#include "../DspClasses/Filter.h"

#include "../Common/Shared.h"


//	Some typedefs

typedef unsigned long  ulong;
typedef unsigned short uword;
typedef unsigned char  ubyte;

//	Defines

#define c_strName			"Dolby Surround"
#define c_strShortName		"Surround"

#define c_maxAmp			32768.0
#define c_maxDelay			0.050


/*
 *		Declarations and globals
 */

struct				 CMachine;

int					 dspcSampleRate;
CMachineInterface	*dspcMachine;


/*
 *		Parameters
 */

#define mpf_State			MPF_STATE		// Semantics
#define c_ControlNoValue	0xFF

#define c_numControls		2

CMachineParameter const  mpC[c_numControls] = { 
	{ pt_byte,"Mix","Mix",0,0x80,c_ControlNoValue,mpf_State,90 },
	{ pt_byte,"Delay","Delay",10,0x80,c_ControlNoValue,mpf_State,20 } };

CMachineParameter const *mpArray[c_numControls] =
	{ &mpC[0],&mpC[1] };

enum mpValues
	{ mpvControl,mpvMix=0,mpvDelay,mpvC2 };

#pragma pack(1)		

struct CGlobalParameters 
	{ byte C[c_numControls]; };

#pragma pack()


/*
 *		Attributes
 */

#define	c_numAttributes		1

CMachineAttribute const maA[c_numAttributes] = {
	{ "Dummy Attribute",0,1,0 } };

CMachineAttribute const *maArray[c_numAttributes] =
	{ &maA[0] };

enum maValues
	{ mavDummy=0 };

#pragma pack(1)		

struct CAttributes { int A[c_numAttributes]; };

#pragma pack()


/*
 *		CMachineInfo
 */

CMachineInfo const miMachineInfo = { 
	MT_EFFECT,MI_VERSION,MIF_MONO_TO_STEREO,
	0,0,c_numControls,0,mpArray,/*c_numAttributes*/0,maArray,
	"Geonik's " c_strName,c_strShortName,"George Nicolaidis aka Geonik","About..." };

enum miCommands { micAbout };


/*
 *		Custom DSP classes
 */

struct CBwBp : public CBwFilter {

	void Work(float const *pin, float *pout, int ns) {

		double const a  = fA[0];
		double const a3 = fA[3];
		double const a4 = fA[4];
		double y1 = fY[1];
		double y0 = fY[0];

		do {
			double const t = *pin++ - a3 * y0 - a4 * y1;
			double const y = a * (t - y1);
			y1 = y0;
			y0 = t;
			*pout++ = (float)y;
		} while(--ns);

		fY[1] = (float)y1;
		fY[0] = (float)y0; }

};

/*
 *		General purpose functions
 */

double ControlByteToDouble(int const,byte const);	// Convert control byte


/*
 *		Machine class
 */

struct CMachine : public CMachineInterface {
				 CMachine();
				~CMachine();
	void		 Init(CMachineDataInput * const pi);
	void		 SetNumTracks(int const n);
	void		 AttributesChanged();
	char const	*DescribeValue(int const param, int const value);
	void		 Tick();
//	bool		 Work(float *,int,int const);
	bool		 WorkMonoToStereo(float *,float *, int, int const);
	void		 Stop();
	void		 Command(int const);
	void		 About(void);
	void		 MidiNote(int const,int const,int const);
	void		 ControlChange(int const,double const);
	void		 AttributeChange(int const iAttributeNum, int const v);

//	CSharedResource		 cSharedResource;
	CGlobalParameters	 cGlobalParameters;
	CAttributes			 cAttributes;

	int					 iAttribute[c_numAttributes];
	byte				 bControl[c_numControls];
	double				 fControl[c_numControls];

	CDelay				 cDelay;
	CBwBp				 cFilter;

	double				 fDryOut;
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
 *		Machine members
 */

CMachine::CMachine() {
	GlobalVals	= &cGlobalParameters;
	AttrVals	= (int *)&cAttributes;
 }


CMachine::~CMachine() {
 }


void CMachine::Init(CMachineDataInput * const pi) { int i;

	dspcSampleRate	= pMasterInfo->SamplesPerSec;		// Dsp Classes
	dspcMachine		= this;

	for(i=0; i < c_numAttributes; i++) {				// Attributes
		iAttribute[i] = 0x80000000; }

/*	for(i=0; i<c_numControls; i++) {					// Controls
		CMachineParameter const *p = mpArray[i + mpvControl];
		bControl[i] = p->DefValue;
		fControl[i] = ControlByteToDouble(i,bControl[i]);
		ControlChange(i,bControl[i]); } */

	//	Custom machine initialization

	cDelay.Alloc((int)((double)dspcSampleRate * c_maxDelay + 16));
	cFilter.SetBandpass(1000.0,3860.0);
 }


void CMachine::Command(int const i) {
	switch(i) {
	case micAbout:
		#include "Banner.h" // remove this !
//		About();
		break; }
 }


// #include "../Common/About.h"


void CMachine::SetNumTracks(int const n) {
 }


char const *CMachine::DescribeValue(int const pc, int const iv) {
	static char b[16];
	double v = ControlByteToDouble(pc,iv);
	switch(pc) {
	case mpvControl+mpvMix:
		//	Add descriptions for controls
		sprintf(b,"%.0f%%", (float)(100.0*v));
		break;
	case mpvControl+mpvDelay:
		sprintf(b,"%.1f%ms", (float)(1000.0*v));
		break;
	default:
		sprintf(b,"%.3f", (float)(v)); }
	return b;
 }


void CMachine::Tick() {
	for(int i=0; i<c_numControls; i++) {
		if(cGlobalParameters.C[i] != c_ControlNoValue) {
			bControl[i] = cGlobalParameters.C[i];
			fControl[i] = ControlByteToDouble(i,bControl[i]);
			ControlChange(i,fControl[i]); } }
 }


void CMachine::AttributesChanged() {
	for(int i=0; i<c_numAttributes; i++) {
		if(cAttributes.A[i] != iAttribute[i]) {
			AttributeChange(i,cAttributes.A[i]);
			iAttribute[i] = cAttributes.A[i]; } }
 }


void CMachine::AttributeChange(int const an, int const v) {
	switch(an) {
	case mavDummy:
		break; }
 }


void CMachine::ControlChange(int const cc, double const v) {
	switch(cc) {
	case mpvMix:
		fDryOut = sqrt(1.0 - v*v);
		break;
	case mpvDelay:
		cDelay.SetDelay((int)(v * (double)dspcSampleRate));
		break; }
 }


void CMachine::Stop() {
 }


void CMachine::MidiNote(int const channel, int const value, int const velocity) {
 }


/*
 *		General purpose functions
 */

double ControlByteToDouble(int const cc, byte const b) {
	switch(cc) {
	case mpvMix:
		return ((double)b)*(1.0/128.0);
	case mpvDelay:
		return ((double)b)*(c_maxDelay/128.0);
	default:
		return ((double)b)*(1.0/128.0); }
 }


/*
 *		Worker functions
 */

#pragma optimize ("a", on)

bool CMachine::WorkMonoToStereo(float *pin, float *pout, int ns, int const mode) {

	if (mode == WM_WRITE || mode == WM_NOIO)
		return false;

	if (mode == WM_READ) {
		do {
			float const v = *pin++;
			*pout++ = v;
			*pout++ = v;
		} while(--ns);
		return true; }

	float *paux = pCB->GetAuxBuffer();
	cFilter.Work(pin,paux,ns);

	double const wa = fControl[mpvMix];
	double const da = fDryOut;

	float *pdel = cDelay.pBuffer + cDelay.iPos;
	while(ns > 0) {
		int cnt = __min(ns,(cDelay.pBuffer + cDelay.iLength) - pdel);
		ns -= cnt;
		do {
			double const c = *pin++;
			double const s = *pdel;
			*pdel++ = *paux++;
			*pout++ = (float)(da * c + wa * s);
			*pout++ = (float)(da * c - wa * s);
		} while(--cnt);
		if(pdel == cDelay.pBuffer + cDelay.iLength) pdel = cDelay.pBuffer; }
	cDelay.iPos = pdel - cDelay.pBuffer;

	return true; }
