#include "../mdk/MachineInterface.h"
#include "../dsplib/dsplib.h"

#pragma optimize ("awy", on)

//___________________________________________________________________
// 
// Parameters and gruelling machine paperwork starts here
//___________________________________________________________________

CMachineParameter const paraNote = {
	pt_note,
	"Note",
	"Note",
	NOTE_MIN,
	NOTE_OFF,
	NOTE_NO,
	MPF_TICK_ON_EDIT,
	0
};

CMachineParameter const paraWaveNumber = {
	pt_byte,
	"Wave Number",
	"Wave Number",
	WAVE_MIN,
	WAVE_MAX,
	WAVE_NO,
	MPF_WAVE,
	0
};

CMachineAttribute const attrMIDIChannel = 
{
return null;
};

#pragma pack(1)

class tvals
{
public:
	byte note;
	byte number;
	
};

class avals
{
public:

};

#pragma pack()

CMachineInfo const MacInfo =
{

	MT_GENERATOR,
	MI_VERSION,
	MIF_PLAYS_WAVES | MIF_MONO_TO_STEREO,
	1,
	MAX_TRACKS,
	0,
	7,
	pParameters,
	4,
	pAttributes,
	"Intoxicat Asynchrous Cloud",
	"ACloud",
	"Karl J. Nilsson",
	NULL
};

class mi;

#pragma pack(8)

#pragma pack()

class mi : public CMachineInterface
{
public:
	mi();
	virtual ~mi();

	virtual void Tick();
	virtual void Stop();

	virtual void Init(CMachineDataInput * const pi);
	virtual bool WorkMonoToStereo(float *pin, float *pout, int numsamples, int const mode);
	virtual char const *mi::DescribeValue(int const param, int const value);
	virtual void SetNumTracks(int const n) { numTracks = n; }
	virtual int AllocateTrack(CSequence *pseq, int note);
	virtual void MidiNoteOff(int c, CSequence *pseq, int notedelay);
	virtual void MidiNote(int const channel, int const value, int const velocity);

public:
	CMachine *ThisMachine;
	int numTracks;
	CTrack Tracks[MAX_TRACKS];
	tvals tval[MAX_TRACKS];
	avals aval;

};

DLL_EXPORTS

mi::mi()
{
	TrackVals = tval;
	AttrVals = (int *)&aval;
}

mi::~mi() 
{

}

//___________________________________________________________________
// 
// Track Level functions start here
//___________________________________________________________________



//___________________________________________________________________
// 
// Machine Level functions start here
//___________________________________________________________________

void mi::Init(CMachineDataInput * const pi)
{
	ThisMachine = pCB->GetThisMachine();


}

void mi::Tick()
{

}

bool mi::WorkMonoToStereo(float *pin, float *pout, int numsamples, int const mode)
{
	bool gotsomething = false;
	bool firstone = true;
	int i;

	float *paux = pCB->GetAuxBuffer();

	for (i = 0; i < numTracks; i++){
		Tracks[i].Generate(paux, numsamples);
		if (Tracks[i].gotsomething == 1) {
			gotsomething = true;
			if (firstone == true) {
				DSP_Copy(pout, paux, numsamples*2);
				firstone = false;
			} else {
				DSP_Add(pout, paux, numsamples*2);
			};
		}
	}

	return gotsomething;
}

void mi::Stop()
{
	for (int c = 0; c < numTracks; c++)
		Tracks[c].Stop();
}

char const *mi::DescribeValue(int const param, int const value)
{
	static char txt[16];

	switch(param)
	{
	case 0:				
		return NULL;
		break;

	default: return NULL;
		break;
	};
}
