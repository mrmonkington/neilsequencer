/*
 *		Overdrive plug-in for Buzz
 *
 *			Written by George Nicolaidis aka Geonik
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include "MachineInterface.h"
#include "DspClasses.h"

#define	 dspcSat_OutputAmp	2.0
#define  dspcSat_InputAmp	4.0
#include "../DspClasses/Saturator.h"

typedef unsigned long  ulong;
typedef unsigned short uword;
typedef unsigned char  ubyte;

#define StdVol		32768

CMachineParameter const mpDrive	= {
	pt_byte,"Drive","Drive amp in dB (30=-16dB, 40=0, 80=64dB, Default = 16dB)",
	48,0x80,0xFF,MPF_STATE,80 };

CMachineParameter const mpAssym = {
	pt_byte,"Assymetry","Assymetry (0-80, Default = 44)",
	0,0x80,0xFF,MPF_STATE,44 };

CMachineParameter const mpCutOff = {
	pt_byte,"CutOff","CutOff frequency (0-80, Default = 108)",
	0,0x80,0xFF,MPF_STATE,108 };

CMachineParameter const mpEmphasis = {
	pt_byte,"Emphasis","Emphasis (0-80, Default = 95)",
	0,0x80,0xFF,MPF_STATE,95 };

CMachineParameter const mpVolume = {
	pt_byte,"Volume","Output Volume (0-80, Default = -2dB)",
	0,0x80,0xFF,MPF_STATE,124 };

CMachineParameter const *mpArray[] = {
	&mpDrive,&mpAssym,&mpCutOff,&mpEmphasis,&mpVolume };

enum mpValues { mpvDrive,mpvAssym,mpvCutOff,mpvEmphasis,mpvVolume };

CMachineInfo const MachineInfo = { 
	MT_EFFECT,MI_VERSION,0,
	0,0,5,0,mpArray,0,NULL,
	"Geonik's Overdrive 3","Overdrive3","George Nicolaidis aka Geonik",NULL };

#pragma pack(1)		

class Parameters {
public:
	byte	Drive;
	byte	Assym;
	byte	CutOff;
	byte	Emphasis;
	byte	Volume; };

class Attributes {
public:
	int		HighFreq; };

#pragma pack()

class Effect : public CMachineInterface {
public:
						 Effect();
	virtual				~Effect();

	virtual void		 Init(CMachineDataInput * const pi);
	virtual void		 Tick();
	virtual bool		 Work(float *psamples, int numsamples, int const Mode);
	virtual char const	*DescribeValue(int const param, int const value);

private:
	void				 DcOffset(float *psamples, int numsamples);

private:
	double			DriveAmp;
	double			Assymetry;
	double			Correction;
	double			Volume;

	CQFilter		cFilter;
	CSaturator		cSaturator;
	CDcBlock		cDcBlock;

	Parameters		Param;
	Attributes		Attr; };

extern "C" {
__declspec(dllexport) CMachineInfo const * __cdecl GetInfo() {
	return &MachineInfo; }
__declspec(dllexport) CMachineInterface * __cdecl CreateMachine() {
	return new Effect; } }

	
Effect::Effect() {
	GlobalVals	= &Param;
	AttrVals	= (int *)&Attr; }


Effect::~Effect() { }


void Effect::Init(CMachineDataInput * const pi) {
	DriveAmp	= 7.0 * (1.0/StdVol);
	Assymetry	= 1.7;
	Correction	= -1.33875;
	Volume		= 0.89 * StdVol; }


char const *Effect::DescribeValue(int const ParamNum, int const Value) {
	static char TxtBuffer[16];

	switch(ParamNum) {
	case mpvDrive:
		sprintf(TxtBuffer,"%d dB", Value-64); break;
	case mpvAssym:
	case mpvCutOff:
	case mpvEmphasis:
		sprintf(TxtBuffer,"%.1f%%", (float)Value * (100.0 / 128.0)); break;
	case mpvVolume:
		sprintf(TxtBuffer,"%.1f dB", (float)(Value-128) * (1.0 / 2.0)); break;
	default:
		return NULL; }
	return TxtBuffer; }


void Effect::Tick() {

	if(Param.Drive != mpDrive.NoValue) { 
		DriveAmp = pow(2.0,(double)(Param.Drive-64)/6.0) * (1.0/StdVol); }

	if(Param.Assym != mpAssym.NoValue) { 
		Assymetry = Param.Assym*(4.0/129.0);
		Correction = -Assymetry*(Assymetry*(-0.125)+1.0); }

	if(Param.Emphasis != mpEmphasis.NoValue) {
		cFilter.fResonance = 1.0 - (Param.Emphasis * (960.0 / 1000.0) / 127.0); }

	if(Param.CutOff != mpCutOff.NoValue) {
		cFilter.fCutOff = Param.CutOff * 1.0 / 128.0; }

	if(Param.Volume != mpVolume.NoValue) { 
		Volume = pow(2.0,(double)(Param.Volume-128)/12.0) * StdVol; } }


#pragma optimize ("a", on)

bool Effect::Work(float *pout, int numsamples, int const mode) {

	if (mode == WM_WRITE || mode == WM_NOIO)
		return false;
	if (mode == WM_READ)
		return true;

{	int			 ns  = numsamples;
	float		*pb  = pout;
	double const amp = DriveAmp;
	double const a   = Assymetry;
	do {
		double s = *pb;
		*pb++ = (float)((s*amp) + a); } while(--ns); }

	cSaturator.WorkSamples(pout,numsamples);

{	int			 ns  = numsamples;
	float		*pb  = pout;
	double const ac  = Correction;
	double const vol = Volume;
	do {
		double s = *pb;
		*pb++ = (float)((s+ac) * vol); } while(--ns); }

	cFilter.WorkSamples(pout,numsamples);
	cDcBlock.WorkSamples(pout,numsamples);
	return true; }
