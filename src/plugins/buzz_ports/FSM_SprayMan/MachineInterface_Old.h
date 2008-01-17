// Copyright (C) 1998 Oskari Tammelin (ot@iki.fi)
// This header file may be used to write _freeware_ DLL "machines" for Buzz or a BMX player
// Using it for anything else is not allowed without a permission from the author
   
#ifndef __MACHINE_INTERFACE_H
#define __MACHINE_INTERFACE_H

#include <stdio.h>
#include <assert.h>

#pragma pack(push,4)

#define MI_VERSION				15
  
/* #define __max max */
/* #define __min min */
typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;

double const PI = 3.14159265358979323846;

#define MAX_BUFFER_LENGTH		256			// in number of samples

// machine types
#define MT_MASTER				0 
#define MT_GENERATOR			1
#define MT_EFFECT				2

// special parameter values
#define NOTE_NO					0
#define NOTE_OFF				255
#define NOTE_MIN				1					// C-0
#define NOTE_MAX				((16 * 9) + 12)		// B-9
#define SWITCH_OFF				0
#define SWITCH_ON				1
#define SWITCH_NO				255
#define WAVE_MIN				1
#define WAVE_MAX				200
#define WAVE_NO					0

// CMachineParameter flags
#define MPF_WAVE				1
#define MPF_STATE				2	
#define MPF_TICK_ON_EDIT		4				

// CMachineInfo flags

#define MIF_MONO_TO_STEREO		1
#define MIF_PLAYS_WAVES			2
#define MIF_USES_LIB_INTERFACE	4
#define MIF_USES_INSTRUMENTS	8
#define MIF_DOES_INPUT_MIXING	16
#define MIF_NO_OUTPUT			32		// used for effect machines that don't actually use any outputs (WaveOutput, AuxSend etc.)

// work modes
#define WM_NOIO					0
#define WM_READ					1
#define WM_WRITE				2
#define WM_READWRITE			3

enum BEventType
{
	DoubleClickMachine					// return true to ignore default handler (open parameter dialog), no parameters
};

class CMachineInterface;
typedef bool (CMachineInterface::*EVENT_HANDLER_PTR)(void *);

enum CMPType { pt_note, pt_switch, pt_byte, pt_word };

class CMachineParameter
{
public:

	CMPType Type;			// pt_byte
	char const *Name;		// Short name: "Cutoff"
	char const *Description;// Longer description: "Cutoff Frequency (0-7f)"
	int MinValue;			// 0
	int MaxValue;			// 127
	int NoValue;			// 255
	int Flags;
	int DefValue;			// default value for params that have MPF_STATE flag set
};

class CMachineAttribute
{
public:
	char const *Name;
	int MinValue;
	int MaxValue;
	int DefValue;
};

class CMasterInfo
{
public:
	int BeatsPerMin;		// [16..500] 	
	int TicksPerBeat;		// [1..32]
	int SamplesPerSec;		// usually 44100, but machines should support any rate from 11050 to 96000
	int SamplesPerTick;		// (int)((60 * SPS) / (BPM * TPB))  
	int PosInTick;			// [0..SamplesPerTick-1]
	float TicksPerSec;		// (float)SPS / (float)SPT  

};

// CWaveInfo flags
#define WF_LOOP			1
#define WF_STEREO		8
#define WF_BIDIR_LOOP	16

class CWaveInfo
{
public:
	int Flags;
	float Volume;

};

class CWaveLevel
{
public:
	int numSamples;
	short *pSamples;
	int RootNote;
	int SamplesPerSec;
	int LoopStart;
	int LoopEnd;
};

// oscillator waveforms (used with GetOscillatorTable function)
#define OWF_SINE		0
#define OWF_SAWTOOTH	1
#define OWF_PULSE		2		// square 
#define OWF_TRIANGLE	3
#define OWF_NOISE		4	

// each oscillator table contains one full cycle of a bandlimited waveform at 11 levels
// level 0 = 2048 samples  
// level 1 = 1024 samples
// level 2 = 512 samples
// ... 
// level 9 = 8 samples 
// level 10 = 4 samples
// level 11 = 2 samples
//
// the waves are normalized to 16bit signed integers   
//
// GetOscillatorTable retusns pointer to a table 
// GetOscTblOffset returns offset in the table for a specified level 
 
inline int GetOscTblOffset(int const level)
{
	assert(level >= 0 && level <= 10);
	return (2048+1024+512+256+128+64+32+16+8+4) & ~((2048+1024+512+256+128+64+32+16+8+4) >> level);
}

class CPattern;
class CSequence;
class CMachine;
class CMachineInterfaceEx;

#undef FASTCALL
#define FASTCALL __attribute__((__fastcall__))


struct CMICallbacks
{
public:
	virtual FASTCALL CWaveInfo const *GetWave(int dummy, int const i);
	inline CWaveInfo const *GetWave(int const i) { return GetWave(0,i); }
	virtual FASTCALL CWaveLevel const *GetWaveLevel(int dummy, int const i, int const level);
	inline CWaveLevel const *GetWaveLevel(int const i, int const level) { return GetWaveLevel(0,i,level); }
	virtual FASTCALL void MessageBox(int dummy, char const *txt);
	inline void MessageBox(char const *txt) { MessageBox(0,txt); }
	virtual FASTCALL void Lock();
	virtual FASTCALL void Unlock();
	virtual FASTCALL int GetWritePos();			
	virtual FASTCALL int GetPlayPos();	
	virtual FASTCALL float *GetAuxBuffer();
	virtual FASTCALL void ClearAuxBuffer();
	virtual FASTCALL int GetFreeWave();
	virtual FASTCALL bool AllocateWave(int dummy, int const i, int const size, char const *name);
	virtual FASTCALL void ScheduleEvent(int dummy, int const time, dword const data);
	virtual FASTCALL void MidiOut_(int dummy, int const dev, dword const data);
	inline void MidiOut(int const dev, dword const data) { MidiOut_(0,dev,data); }
	virtual FASTCALL short const *GetOscillatorTable(int dummy, int const waveform);

	// envelopes
	virtual FASTCALL int GetEnvSize(int dummy, int const wave, int const env);
	virtual FASTCALL bool GetEnvPoint(int dummy, int const wave, int const env, int const i, word &x, word &y, int &flags);

	virtual FASTCALL CWaveLevel const *GetNearestWaveLevel(int dummy, int const i, int const note);
	
	// pattern editing
	virtual FASTCALL void SetNumberOfTracks(int dummy, int const n);
	virtual FASTCALL CPattern *CreatePattern(int dummy, char const *name, int const length);
	virtual FASTCALL CPattern *GetPattern(int dummy, int const index);
	virtual FASTCALL char const *GetPatternName(int dummy, CPattern *ppat);
	virtual FASTCALL void RenamePattern(int dummy, char const *oldname, char const *newname);
	virtual FASTCALL void DeletePattern(int dummy, CPattern *ppat);
	virtual FASTCALL int GetPatternData(int dummy, CPattern *ppat, int const row, int const group, int const track, int const field);
	virtual FASTCALL void SetPatternData(int dummy, CPattern *ppat, int const row, int const group, int const track, int const field, int const value);
 		
	// sequence editing
	virtual FASTCALL CSequence *CreateSequence();
	virtual FASTCALL void DeleteSequence(int dummy, CSequence *pseq);
	

	// special ppat values for GetSequenceData and SetSequenceData 
	// empty = NULL
	// <break> = (CPattern *)1
	// <mute> = (CPattern *)2
	// <thru> = (CPattern *)3
	virtual FASTCALL CPattern *GetSequenceData_(int dummy, int const row);
	inline CPattern *GetSequenceData(int const row) { return GetSequenceData_(0,row); }
	
	virtual FASTCALL void SetSequenceData_(int dummy, int const row, CPattern *ppat);
	inline void SetSequenceData(int const row, CPattern *ppat) { SetSequenceData_(0,row,ppat); }
		
	// buzz v1.2 (MI_VERSION 14) additions start here
	
	virtual FASTCALL void SetMachineInterfaceEx(int dummy, CMachineInterfaceEx *pex);
	inline void SetMachineInterfaceEx(CMachineInterfaceEx *pex) { SetMachineInterfaceEx(0,pex); }
	// group 1=global, 2=track
	virtual FASTCALL void ControlChange_obsolete_(int dummy, int group, int track, int param, int value);						// set value of parameter
	
	// direct calls to audiodriver, used by WaveInput and WaveOutput
	// shouldn't be used for anything else
	virtual FASTCALL int ADGetnumChannels(int dummy, bool input);
	virtual FASTCALL void ADWrite(int dummy, int channel, float *psamples, int numsamples);
	virtual FASTCALL void ADRead(int dummy, int channel, float *psamples, int numsamples);

	virtual FASTCALL CMachine *GetThisMachine();	// only call this in Init()!
	virtual FASTCALL void ControlChange(int dummy, CMachine *pmac, int group, int track, int param, int value);		// set value of parameter

	// returns pointer to the sequence if there is a pattern playing
	virtual FASTCALL CSequence *GetPlayingSequence(int dummy, CMachine *pmac);

	// gets ptr to raw pattern data for row of a track of a currently playing pattern (or something like that)
	virtual FASTCALL void *GetPlayingRow(int dummy, CSequence *pseq, int group, int track);

	virtual FASTCALL int GetStateFlags();

	virtual FASTCALL void SetnumOutputChannels(int dummy, CMachine *pmac, int n);	// if n=1 Work(), n=2 WorkMonoToStereo()

	virtual FASTCALL void SetEventHandler(int dummy, CMachine *pmac, BEventType et, EVENT_HANDLER_PTR p, void *param);

	virtual FASTCALL char const *GetWaveName(int dummy, int const i);

	virtual FASTCALL void SetInternalWaveName(int dummy, CMachine *pmac, int const i, char const *name);	// i >= 1, NULL name to clear

	 
};

struct CMachineInfo
{
public:
	int Type;								// MT_GENERATOR or MT_EFFECT
	int Version;							// MI_VERSION
	int Flags;				
	int minTracks;							// [0..256] must be >= 1 if numTrackParameters > 0 
	int maxTracks;							// [minTracks..256] 
	int numGlobalParameters;				
	int numTrackParameters;					
	CMachineParameter const **Parameters;
	int numAttributes;
	CMachineAttribute const **Attributes;
	char const *Name;						// "Jeskola Reverb"
	char const *ShortName;					// "Reverb"
	char const *Author;						// "Oskari Tammelin"
	char const *Commands;					// "Command1\nCommand2\nCommand3..."

};

struct CMachineDataInput
{
public:
	virtual FASTCALL void Read(int dummy, void *pbuf, int const numbytes);

	void Read(int &d) { Read(0, &d, sizeof(int)); }
	void Read(dword &d) { Read(0, &d, sizeof(dword)); }
	void Read(short &d) { Read(0, &d, sizeof(short)); }
	void Read(word &d) { Read(0, &d, sizeof(word)); }
	void Read(char &d) { Read(0, &d, sizeof(char)); }
	void Read(byte &d) { Read(0, &d, sizeof(byte)); }
	void Read(float &d) { Read(0, &d, sizeof(float)); }
	void Read(double &d) { Read(0, &d, sizeof(double)); }
	void Read(bool &d) { Read(0, &d, sizeof(bool)); }

};

struct CMachineDataOutput
{
public:
	virtual FASTCALL void Write(int dummy, void *pbuf, int const numbytes);

	void Write(int d) { Write(0, &d, sizeof(int)); }
	void Write(dword d) { Write(0, &d, sizeof(dword)); }
	void Write(short d) { Write(0, &d, sizeof(short)); }
	void Write(word d) { Write(0, &d, sizeof(word)); }
	void Write(char d) { Write(0, &d, sizeof(char)); }
	void Write(byte d) { Write(0, &d, sizeof(byte)); }
	void Write(float d) { Write(0, &d, sizeof(float)); }
	void Write(double d) { Write(0, &d, sizeof(double)); }
	void Write(bool d) { Write(0, &d, sizeof(bool)); }

};

// envelope info flags
#define EIF_SUSTAIN			1
#define EIF_LOOP			2

struct CEnvelopeInfo
{
public:
	char const *Name;
	int Flags;
};

struct CMachineInterface0
{
public:
	virtual FASTCALL void *ScalarDeletingDestructor(int foo, int flags) { void *th=this; if(flags&1) delete this; return th; }
	virtual FASTCALL void Init(int poo, CMachineDataInput * const pi) {}
	virtual FASTCALL void Tick() {}
	virtual FASTCALL bool Work(int poo, float *psamples, int numsamples, int const mode) { return false; }
	virtual FASTCALL bool WorkMonoToStereo(int poo, float *pin, float *pout, int numsamples, int const mode) { return false; }
	virtual FASTCALL void Stop() {}
	virtual FASTCALL void Save(int poo, CMachineDataOutput * const po) {}
	virtual FASTCALL void AttributesChanged() {}
	virtual FASTCALL void Command(int poo, int const i) {}

	virtual FASTCALL void SetNumTracks(int poo, int const n) {}
	virtual FASTCALL void MuteTrack(int poo, int const i) {}
	virtual FASTCALL bool IsTrackMuted(int poo, int const i) const { return false; }

	virtual FASTCALL void MidiNote(int poo, int const channel, int const value, int const velocity) {}
	virtual FASTCALL void Event(int poo, dword const data) {}

	virtual FASTCALL char const *DescribeValue(int poo, int const param, int const value) { return NULL; }

	virtual FASTCALL CEnvelopeInfo const **GetEnvelopeInfos() { return NULL; }

	virtual FASTCALL bool PlayWave(int poo, int const wave, int const note, float const volume) { return false; }
	virtual FASTCALL void StopWave() {}
	virtual FASTCALL int GetWaveEnvPlayPos(int poo, int const env) { return -1; }
public:
	// initialize these members in the constructor 
};
struct CMachineInterface: public CMachineInterface0
{
		
	// these members are initialized by the 
	// engine right after it calls CreateMachine()
	// don't touch them in the constructor

	void *GlobalVals;
	void *TrackVals;
	int *AttrVals;
	CMasterInfo *pMasterInfo;
	CMICallbacks *pCB;					
};
 
class CLibInterface
{
public:
	virtual FASTCALL void GetInstrumentList(CMachineDataOutput *pout) {}			
	
	// make some space to vtable so this interface can be extended later 
	virtual FASTCALL void Dummy1() {}
	virtual FASTCALL void Dummy2() {}
	virtual FASTCALL void Dummy3() {}
	virtual FASTCALL void Dummy4() {}
	virtual FASTCALL void Dummy5() {}
	virtual FASTCALL void Dummy6() {}
	virtual FASTCALL void Dummy7() {}
	virtual FASTCALL void Dummy8() {}
	virtual FASTCALL void Dummy9() {}
	virtual FASTCALL void Dummy10() {}
	virtual FASTCALL void Dummy11() {}
	virtual FASTCALL void Dummy12() {}
	virtual FASTCALL void Dummy13() {}
	virtual FASTCALL void Dummy14() {}
	virtual FASTCALL void Dummy15() {}
	virtual FASTCALL void Dummy16() {}
	virtual FASTCALL void Dummy17() {}
	virtual FASTCALL void Dummy18() {}
	virtual FASTCALL void Dummy19() {}
	virtual FASTCALL void Dummy20() {}
	virtual FASTCALL void Dummy21() {}
	virtual FASTCALL void Dummy22() {}
	virtual FASTCALL void Dummy23() {}
	virtual FASTCALL void Dummy24() {}
	virtual FASTCALL void Dummy25() {}
	virtual FASTCALL void Dummy26() {}
	virtual FASTCALL void Dummy27() {}
	virtual FASTCALL void Dummy28() {}
	virtual FASTCALL void Dummy29() {}
	virtual FASTCALL void Dummy30() {}
	virtual FASTCALL void Dummy31() {}
	virtual FASTCALL void Dummy32() {}
};

// buzz v1.2 extended machine interface
class CMachineInterfaceEx
{
public:
	virtual FASTCALL char const *DescribeParam(int dummy, int const param) { return NULL; }		// use this to dynamically change name of parameter
	virtual FASTCALL bool SetInstrument(int dummy, char const *name) { return false; }

	virtual FASTCALL void GetSubMenu(int dummy, int const i, CMachineDataOutput *pout) {}

	virtual FASTCALL void AddInput(int dummy, char const *macname, bool stereo) {}	// called when input is added to a machine
	virtual FASTCALL void DeleteInput(int dummy, char const *macename) {}			
	virtual FASTCALL void RenameInput(int dummy, char const *macoldname, char const *macnewname) {}

	virtual FASTCALL void Input(int dummy, float *psamples, int numsamples, float amp) {} // if MIX_DOES_INPUT_MIXING

	virtual FASTCALL void MidiControlChange(int dummy, int const ctrl, int const channel, int const value) {}

	virtual FASTCALL void SetInputChannels(int dummy, char const *macname, bool stereo) {}

	// make some space to vtable so this interface can be extended later 
	virtual FASTCALL void Dummy1() {}
	virtual FASTCALL void Dummy2() {}
	virtual FASTCALL void Dummy3() {}
	virtual FASTCALL void Dummy4() {}
	virtual FASTCALL void Dummy5() {}
	virtual FASTCALL void Dummy6() {}
	virtual FASTCALL void Dummy7() {}
	virtual FASTCALL void Dummy8() {}
	virtual FASTCALL void Dummy9() {}
	virtual FASTCALL void Dummy10() {}
	virtual FASTCALL void Dummy11() {}
	virtual FASTCALL void Dummy12() {}
	virtual FASTCALL void Dummy13() {}
	virtual FASTCALL void Dummy14() {}
	virtual FASTCALL void Dummy15() {}
	virtual FASTCALL void Dummy16() {}
	virtual FASTCALL void Dummy17() {}
	virtual FASTCALL void Dummy18() {}
	virtual FASTCALL void Dummy19() {}
	virtual FASTCALL void Dummy20() {}
	virtual FASTCALL void Dummy21() {}
	virtual FASTCALL void Dummy22() {}
	virtual FASTCALL void Dummy23() {}
	virtual FASTCALL void Dummy24() {}
	virtual FASTCALL void Dummy25() {}
	virtual FASTCALL void Dummy26() {}
	virtual FASTCALL void Dummy27() {}
	virtual FASTCALL void Dummy28() {}
	virtual FASTCALL void Dummy29() {}
	virtual FASTCALL void Dummy30() {}
	virtual FASTCALL void Dummy31() {}
	virtual FASTCALL void Dummy32() {}

};
 
class CMILock
{
public:
	CMILock(CMICallbacks *p) { pCB = p; pCB->Lock(); }
	~CMILock() { pCB->Unlock(); }
private:
	CMICallbacks *pCB;
};

#define MACHINE_LOCK CMILock __machinelock(pCB);

#define DLL_EXPORTS extern "C" { \
__declspec(dllexport) CMachineInfo const * __cdecl GetInfo() \
{ \
	return &MacInfo; \
} \
__declspec(dllexport) CMachineInterface * __cdecl CreateMachine() \
{ \
	return new mi; \
} \
} 

#endif 
