/*
 *		AutoPan plug-in for Buzz
 *
 *			Written by George Nicolaidis aka Geonik
 */

#include <windows.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include "MachineInterface.h"

#include "../Common/Shared.h"

typedef unsigned long  ulong;
typedef unsigned short uword;
typedef unsigned char  ubyte;

#define c_strName			"AutoPan"
#define c_strShortName		"AutoPan"

#pragma warning (disable:4305)

const float Sin129[] = {
  0,0.0122715382857,0.0245412285229,0.0368072229414,0.0490676743274,
  0.0613207363022,0.0735645635997,0.0857973123444,0.0980171403296,
  0.110222207294,0.122410675199,0.134580708507,0.146730474455,0.158858143334,
  0.17096188876,0.183039887955,0.195090322016,0.207111376192,0.219101240157,
  0.231058108281,0.242980179903,0.254865659605,0.266712757475,0.278519689385,
  0.290284677254,0.302005949319,0.313681740399,0.325310292162,0.336889853392,
  0.348418680249,0.359895036535,0.371317193952,0.382683432365,0.393992040061,
  0.405241314005,0.416429560098,0.42755509343,0.438616238539,0.449611329655,
  0.460538710958,0.471396736826,0.482183772079,0.49289819223,0.503538383726,
  0.514102744193,0.524589682678,0.534997619887,0.545324988422,0.55557023302,
  0.565731810784,0.575808191418,0.585797857456,0.595699304492,0.605511041404,
  0.615231590581,0.624859488142,0.634393284164,0.64383154289,0.653172842954,
  0.66241577759,0.671558954847,0.680600997795,0.689540544737,0.698376249409,
  0.707106781187,0.715730825284,0.724247082951,0.732654271672,0.740951125355,
  0.749136394523,0.757208846506,0.765167265622,0.773010453363,0.780737228572,
  0.788346427627,0.795836904609,0.803207531481,0.810457198253,0.817584813152,
  0.824589302785,0.831469612303,0.838224705555,0.84485356525,0.851355193105,
  0.85772861,0.863972856122,0.870086991109,0.876070094195,0.881921264348,
  0.887639620403,0.893224301196,0.898674465694,0.903989293123,0.909167983091,
  0.914209755704,0.91911385169,0.923879532511,0.928506080473,0.932992798835,
  0.937339011913,0.941544065183,0.945607325381,0.949528180593,0.953306040354,
  0.956940335732,0.960430519416,0.963776065795,0.966976471045,0.970031253195,
  0.972939952206,0.975702130039,0.97831737072,0.980785280403,0.983105487431,
  0.985277642389,0.987301418158,0.989176509965,0.990902635428,0.992479534599,
  0.993906970002,0.995184726672,0.996312612183,0.997290456679,0.9981181129,
  0.998795456205,0.999322384588,0.999698818696,0.999924701839,1. };

#pragma warning (default:4305)


CMachineParameter const mpSet = {
	pt_byte,"Set","Set position (Left=0, Right=128, Default=64)",
	0,0x80,0xFF,MPF_STATE,0x40 };

CMachineParameter const mpTarget = {
	pt_byte,"Target","Target position (Left=0, Right=128, Default=64)",
	0,0x80,0xFF,MPF_STATE,0x40 };

CMachineParameter const mpInertia = {
	pt_word,"Inertia","Response time in 1/10 ticks (Default=80)",
	0,10000,0xFFFF,MPF_STATE,80 };

CMachineParameter const *mpArray[] = {
	&mpSet,&mpTarget,&mpInertia };

enum mpValues { mpvSet = 0,mpvTarget,mpvInertia };

CMachineAttribute const maHighFreq	= {
	"High Frequency Lfo",0,1,0 };

CMachineAttribute const *maArray[]	= { &maHighFreq };

CMachineInfo const MachineInfo = { 
	MT_EFFECT,MI_VERSION,MIF_MONO_TO_STEREO,
	0,0,3,0,mpArray,0,maArray,
	"Geonik's AutoPan","AutoPan","George Nicolaidis aka Geonik","About..." };

#pragma pack(1)		

class Parameters {
public:
	byte	Set;
	byte	TargetPos;
	word	Inertia; };

class Attributes {
public:
	int		HighFreq; };

#pragma pack()

class CMachine : public CMachineInterface {
public:
				 CMachine();
				~CMachine();

	void		 Init(CMachineDataInput * const pi);
	void		 Tick();
	bool		 WorkMonoToStereo(float *pin, float *pout, int numsamples, int const mode);
	char const	*DescribeValue(int const param, int const value);
	void		 Command(int const);
	void		 About();

	CSharedResource		 cSharedResource;

private:
	int				panCount;
	double			panTargetPos;
	double			panStep;
	double			panCurrentPos;
	int				panInertiaSamples;

	Parameters		Param;
	Attributes		Attr; };

extern "C" {
__declspec(dllexport) CMachineInfo const * __cdecl GetInfo() {
	return &MachineInfo; }
__declspec(dllexport) CMachineInterface * __cdecl CreateMachine() {
	return new CMachine; } }

	
CMachine::CMachine() {
	GlobalVals	= &Param;
	AttrVals	= (int *)&Attr; }


CMachine::~CMachine() { }


void CMachine::Init(CMachineDataInput * const pi) { }


void CMachine::Command(int const i) {
	switch(i) {
	case 0:
		About();
		break; }
 }


#include "../Common/About.h"


char const *CMachine::DescribeValue(int const ParamNum, int const Value) {
	static char TxtBuffer[16];

	switch(ParamNum) {
	case mpvSet:
	case mpvTarget:
		if(Value < 64)
			sprintf(TxtBuffer, "%.0f%% Left", (double)(64-Value) * (100.0 / 64.0));
		else if(Value > 64)
			sprintf(TxtBuffer, "%.0f%% Right", (double)(Value-64) * (100.0 / 64.0));
		else
			return "Center";
		break;
	case mpvInertia:
		sprintf(TxtBuffer, "%.1f Ticks", (double)Value * (1.0 / 10.0)); break;
	default:
		return NULL; }
	return TxtBuffer; }


inline double CalcLinearStep(double from, double to, int c) { return (to - from) / (double)c; }

void CMachine::Tick() {

	if(Param.Set != mpSet.NoValue) {
		panCurrentPos = (double)Param.Set;
		panCount = 0x7FFFFFFF;
		panStep = 0; }

	if(Param.TargetPos != mpTarget.NoValue) {
		if(panInertiaSamples) {
			panTargetPos = (double)Param.TargetPos;
			panStep = CalcLinearStep(panCurrentPos,panTargetPos,panInertiaSamples);
			panCount = panInertiaSamples; }
		else {
			panTargetPos = 
			panCurrentPos = (double)Param.TargetPos;
			panCount = 0x7FFFFFFF;
			panStep = 0; } }

	if(Param.Inertia != mpInertia.NoValue) {
		panInertiaSamples = (int)((double)Param.Inertia * (pMasterInfo->SamplesPerTick / 10.0));
		if(panInertiaSamples) {
			panStep = CalcLinearStep(panCurrentPos,panTargetPos,panInertiaSamples);
			panCount = panInertiaSamples; }
		else {
			panCurrentPos = panTargetPos;
			panCount = 0x7FFFFFFF;
			panStep = 0; } } }


#pragma optimize ("a", on)

bool CMachine::WorkMonoToStereo(float *Source, float *Dest, int numSamples, int const Mode) {

	double const d2i = (1.5 * (1 << 26) * (1 << 26));
	double pp = panCurrentPos;
	double ps = panStep;

	if(Mode != WM_READWRITE) {
		if(ps) {
			if(!panCount) { ps = 0; panStep = 0; return false; }
			int s = __min(panCount, numSamples);
			panCount -= s;
			panCurrentPos += ps*s; }
		return false; }
	else {
		if(ps) {
			if(!panCount) { ps = 0; panStep = 0; goto NoPs; }
			int cnt = __min(panCount, numSamples);
			numSamples -= cnt;
			panCount -= cnt;
			do {
				pp += ps;
				double ip_ = pp + d2i;
				int	   ip  = *(int *)&ip_;
				float s = *Source++;
				*Dest++ = (float)(s*Sin129[128-ip]);
				*Dest++ = (float)(s*Sin129[ip]); } while(--cnt);
			if(numSamples>0) {
				double ip_ = pp + d2i;
				int	   ip  = *(int *)&ip_;
				double l = Sin129[128-ip];
				double r = Sin129[ip];
				do {
					float s = *Source++;
					*Dest++ = (float)(s*l);
					*Dest++ = (float)(s*r); } while(--numSamples); }
			panCurrentPos = pp; }
		else {
NoPs:		double ip_ = pp + d2i;
			int	   ip  = *(int *)&ip_;
			double l = Sin129[128-ip];
			double r = Sin129[ip];
			do {
				float s = *Source++;
				*Dest++ = (float)(s*l);
				*Dest++ = (float)(s*r); } while(--numSamples); }
		return true; } }
