/*
 *		2p Filter plug-in for Buzz
 *
 *			Written by George Nicolaidis aka Geonik
 */

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

#include <windows.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <float.h>

#include "MachineInterface.h"

#include "../DspClasses/DspClasses.h"
#include "../DspClasses/Filter.h"

#include "../Common/Shared.h"

typedef unsigned long  ulong;
typedef unsigned short uword;
typedef unsigned char  ubyte;

#define c_strName			"2p Filter"
#define c_strShortName		"2p Filter"

#define c_numControls		5


/*
 *		Declarations and globals
 */

struct				 CMachine;

int					 dspcSampleRate;
CMachineInterface	*dspcMachine;


/*
 *		Parameters
 */

CMachineParameter const mpType		= { pt_byte,"Type","Filter type (0=LP. 1=HP)",0,1,0xFF,MPF_STATE,0 };
CMachineParameter const	mpCutOff	= {	pt_byte,"CutOff","Filter cutoff",0,0xF0,0xFF,MPF_STATE,0xF0 };
CMachineParameter const	mpBpRes		= {	pt_byte,"Hi Resonance","Stopband resonance",0,0x80,0xFF,MPF_STATE,0x40 };
CMachineParameter const	mpLpRes		= {	pt_byte,"Lo Resonance","Passband resonance (n/a for highpass)",0,0x80,0xFF,MPF_STATE,0x08 };
CMachineParameter const	mpInertia	= { pt_byte,"Inertia","Inertia of the cutoff parameter",0,0xFE,0xFF,MPF_STATE,1 };

CMachineParameter const *mpArray[]	= { &mpType,&mpCutOff,&mpBpRes,&mpLpRes,&mpInertia };
enum					 mpValues	  { mpvType,mpvCutOff,mpvBpRes,mpvLpRes,mpvInertia };
int const				 mpvC0		=   mpvType;

#pragma pack(1)		
struct CGlobalParameters			  { byte C[c_numControls]; };
struct CParameters					  { };
#pragma pack()


/*
 *		Attributes
 */

CMachineAttribute const  maDynRange	= { "Track available at (dB)",-120,-30,-50 };
CMachineAttribute const *maArray[]	= { &maDynRange };

#pragma pack(1)		
struct CAttributes					  { int DynRange; };
#pragma pack()


/*
 *		CMachineInfo
 */

CMachineInfo const miMachineInfo = { 
	MT_EFFECT,MI_VERSION,0,
	0,0,c_numControls,0,mpArray,0,maArray,
	"Geonik's " c_strName,c_strShortName,"George Nicolaidis aka Geonik","About..." };

enum miCommands { micAbout };


/*
 *		Custom DSP classes
 */


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
	bool		 Work(float *psamples, int numsamples, int const Mode);
	void		 Stop();
	void		 Command(int const);
	void		 About();
	void		 ControlChange(int const,double const);

//	CSharedResource		 cSharedResource;
	CGlobalParameters	 cGlobalParameters;
	CAttributes			 cAttributes;

	byte				 bControl[c_numControls];
	double				 fControl[c_numControls];

	C2p2qFilter			 cFilter;

	int					 iCount;
	double				 fStep; };


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

void CMachine::Command(int const i) {
	switch(i) {
	case micAbout:
		#include "Banner.h"
//		About();
		break; }
 }


//#include "../Common/About.h"


void CMachine::Init(CMachineDataInput * const pi) {
	dspcSampleRate	= pMasterInfo->SamplesPerSec;		// Dsp Classes
	dspcMachine		= this;
	for(int i=0; i<c_numControls; i++) {				// Controls
		CMachineParameter const *p = mpArray[i + mpvC0];
		bControl[i] = p->DefValue;
		fControl[i] = ControlByteToDouble(i,bControl[i]); }
	iCount			= 0;
	fStep			= 0;
 }


void CMachine::SetNumTracks(int const n) {
 }

char const *CMachine::DescribeValue(int const pc, int const iv) {
	static char b[16];
	switch(pc) {
	case mpvType:
		switch(iv) {
		case 0: return "Lowpass";
		case 1: return "Highpass"; }
		break;
	case mpvCutOff:
		sprintf(b,"%.0f", (float)iv * (1000.0 / 240.0));
		break;
	case mpvLpRes:
	case mpvBpRes:
		sprintf(b,"%.1f%%", (float)iv * (100.0 / 128.0));
		break;
	case mpvInertia:
		if(iv==0) return "None";
		else if(iv==1) return "1 Tick";
		else sprintf(b,"%d Ticks",iv);
		break;
	default:
		return NULL; }
	return b;
 }

void CMachine::Tick() {
	for(int i=c_numControls-1; i>=0; i--) {					// Reverse scanning
		if(cGlobalParameters.C[i] != 0xff) {				// Waring: Hardcoded novalue
			bControl[i] = cGlobalParameters.C[i];
			fControl[i] = ControlByteToDouble(i,bControl[i]);
			ControlChange(i,fControl[i]); } }
 }

void CMachine::AttributesChanged() {
 }

void CMachine::ControlChange(int const cc, double const v) {
	switch(cc) {
	case mpvCutOff:
		if(bControl[mpvInertia] == 0)
			cFilter.fCutOff = v;
		else {
			iCount = (int)fControl[mpvInertia];
			fStep = (v - cFilter.fCutOff) / fControl[mpvInertia]; }
		break;
	case mpvBpRes:
		cFilter.fBpRes = v; break;
	case mpvLpRes:
		cFilter.fLpRes = v; break;
	case mpvInertia:
		if(v==0) fStep = 0; break; }
 }

void CMachine::Stop() {
 }


/*
 *		General purpose functions
 */

double ControlByteToDouble(int const cc, byte const b) {
	switch(cc) {
	case mpvCutOff:	return 0.06 + (double)b * (0.94 / 240.0);
	case mpvBpRes:
	case mpvLpRes:  return 1.0 - ((double)b)*(0.995/128);
	case mpvInertia:return dspcMachine->pMasterInfo->SamplesPerTick * (double)(b);
	default:		return ((double)b); }
 }


/*
 *		Worker functions
 */

#pragma optimize ("a", on)

bool CMachine::Work(float *pb, int ns, int const mode) {

	double		ps = fStep;

	if(mode != WM_READWRITE) {
		if(ps) {
			if(!iCount) { ps = 0; fStep = 0; }
			else {
				int s = __min(iCount, ns);
				iCount -= s;
				cFilter.fCutOff += ps*s; } }
		if(mode == WM_READ) return true;
		else return false; }
	else {
		if(bControl[mpvType] == 0) {

		/*	Do Lowpass */

			double		 L1 = cFilter.fLopass[0];
			double		 B1 = cFilter.fBdpass[0];
			double		 H1 = cFilter.fHipass[0];
			double		 L2 = cFilter.fLopass[1];
			double		 B2 = cFilter.fBdpass[1];
			double		 H2 = cFilter.fHipass[1];
			double const bq = cFilter.fBpRes;
			double const lq = cFilter.fLpRes;
			double		 f	= cFilter.fCutOff;
			if(ps) {
				if(!iCount) { ps = 0; fStep = 0; goto lp_NoStep; }
				int			 cnt = __min(iCount,ns);
				ns				-= cnt;
				iCount			-= cnt;
				do {
					double const ff = f*f;
					L1 += ff * B1;
					H1 = *pb - L1 - bq * B1;
					B1 += ff * H1;
					L2 += ff * B2;
					H2 = L1 - lq * L2 - B2;
					B2 += ff * H2;
					*pb++ = (float)(L2);
					f += ps;
				} while(--cnt);
				cFilter.fCutOff = f;
				if(ns > 0) goto lp_NoStep; }
			else {
lp_NoStep:		double const ff = f*f;
				do {
					L1 += ff * B1;
					H1 = *pb - L1 - bq * B1;
					B1 += ff * H1;
					L2 += ff * B2;
					H2 = L1 - lq * L2 - B2;
					B2 += ff * H2;
					*pb++ = (float)(L2);
				} while(--ns); }
			cFilter.fLopass[0] = L1;
			cFilter.fBdpass[0] = B1;
			cFilter.fHipass[0] = H1;
			cFilter.fLopass[1] = L2;
			cFilter.fBdpass[1] = B2;
			cFilter.fHipass[1] = H2;
			return true; }

		else {

		/*	Do Highpass */

			double		 L1 = cFilter.fLopass[0];
			double		 B1 = cFilter.fBdpass[0];
			double		 H1 = cFilter.fHipass[0];
			double		 L2 = cFilter.fLopass[1];
			double		 B2 = cFilter.fBdpass[1];
			double		 H2 = cFilter.fHipass[1];
			double const bq = cFilter.fBpRes;
			double		 f	= cFilter.fCutOff;
			if(ps) {
				if(!iCount) { ps = 0; fStep = 0; goto hp_NoStep; }
				int			 cnt = __min(iCount,ns);
				ns				-= cnt;
				iCount			-= cnt;
				do {
					double const ff = f*f;
					L1 += ff * B1;
					H1 = *pb - L1 - bq * B1;
					B1 += ff * H1;
					L2 += ff * B2;
					H2 = H1 - L2 - B2;
					B2 += ff * H2;
					*pb++ = (float)(H2);
					f += ps;
				} while(--cnt);
				cFilter.fCutOff = f;
				if(ns > 0) goto hp_NoStep; }
			else {
hp_NoStep:		double const ff = f*f;
				do {
					L1 += ff * B1;
					H1 = *pb - L1 - bq * B1;
					B1 += ff * H1;
					L2 += ff * B2;
					H2 = H1 - L2 - B2;
					B2 += ff * H2;
					*pb++ = (float)(H2);
				} while(--ns); }
			cFilter.fLopass[0] = L1;
			cFilter.fBdpass[0] = B1;
			cFilter.fHipass[0] = H1;
			cFilter.fLopass[1] = L2;
			cFilter.fBdpass[1] = B2;
			cFilter.fHipass[1] = H2;
			return true; } } }

