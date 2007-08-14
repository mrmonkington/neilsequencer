#pragma once

namespace psycle
{
	int const MI_VERSION = 11;
	int const SEQUENCER = 1;
	int const EFFECT = 0;
	int const GENERATOR = 3;
	int const NOTE_MAX = 119;
	int const NOTE_NO = 120;
	int const NOTE_OFF = 255;

#if !defined MAX_TRACKS
#define MAX_TRACKS 64
#endif

	// todo use #include <inttypes.h> for that!
	typedef unsigned char uint8;
	typedef unsigned short int uint16;
	typedef unsigned long int uint32;

	double const PI = 3.14159265358979323846;

	/// in number of samples (per channel).
	int const MAX_BUFFER_LENGTH = 256;

	/// CMachineParameter flags.
	int const MPF_LABEL = 0;

	/// CMachineParameter flags.
	int const MPF_STATE = 2;
	//}

	class CMachineParameter
	{
	public:

		/// Short name: "Cutoff"
		char const *Name;
		/// Longer description: "Cutoff Frequency (0-7f)"
		char const *Description;
		/// >= 0
		int MinValue;
		/// <= 65535
		int MaxValue;
		/// flags.
		int Flags;
		/// default value for params that have MPF_STATE flag set
		int DefValue;
	};

	class CMachineInfo
	{
	public:
		int Version;
		int Flags;
		int numParameters;
		CMachineParameter const **Parameters;
		/// "Rambo Delay"
		char const *Name;
		/// "Delay"
		char const *ShortName;
		/// "John Rambo"
		char const *Author;
		/// "About"
		char const *Command;
		/// number of columns
		int numCols;
	};

	class CFxCallback
	{
	public:
		virtual void MessBox(char* ptxt,char*caption,unsigned int type){}
		virtual int CallbackFunc(int cbkID,int par1,int par2,int par3){return 0;}
		virtual float *GetWaveLData(int inst,int wave){return 0;} ///\todo USELESS if you cannot get the length!
		virtual float *GetWaveRData(int inst,int wave){return 0;} ///\todo USELESS if you cannot get the length!
		virtual int GetTickLength(){return 2048;}
		virtual int GetSamplingRate(){return 44100;}
		virtual int GetBPM(){return 125;}
		virtual int GetTPB(){return 4;}
		// Don't get fooled by the above return values.
		// You get a pointer to a subclass of this one that returns the correct ones.
	};

	class CMachineInterface
	{
	public:
		virtual ~CMachineInterface() {}
		virtual void Init() {}
		virtual void SequencerTick() {}
		virtual void ParameterTweak(int par, int val) {}

		/// Work function
		virtual void Work(float *psamplesleft, float *psamplesright , int numsamples, int tracks) {}

		virtual void Stop() {}

		///\name Export / Import
		///\{
		virtual void PutData(uint8 * pData) {}
		virtual void GetData(uint8 * pData) {}
		virtual int GetDataSize() { return 0; }
		///\}

		virtual void Command() {}

		virtual void MuteTrack(int const i) {} /// Not used (yet?)
		virtual bool IsTrackMuted(int const i) const { return false; } 	// Not used (yet?)

		virtual void MidiNote(int const channel, int const value, int const velocity) {} /// Not used (yet?)
		virtual void Event(uint32 const data) {} /// Not used (yet?)

		virtual bool DescribeValue(char* txt,int const param, int const value) { return false; }

		virtual bool PlayWave(int const wave, int const note, float const volume) { return false; } /// Not used (prolly never)
		virtual void SeqTick(int channel, int note, int ins, int cmd, int val) {}

		virtual void StopWave() {} 	/// Not used (prolly never)

	public:
		// initialize these members in the constructor
		int *Vals;

		/// Callback.
		/// this member is initialized by the
		/// engine right after it calls CreateMachine()
		/// don't touch it in the constructor
		CFxCallback * pCB;
	};
};


#define PSYCLE__PLUGIN__INSTANCIATOR(typename, info) \
	extern "C" \
	{ \
		__declspec(dllexport) ::CMachineInfo const * const cdecl GetInfo() { return &info; } \
		__declspec(dllexport) ::CMachineInterface * cdecl CreateMachine() { return new typename; } \
		__declspec(dllexport) void cdecl DeleteMachine(::CMachineInterface & plugin) { delete &plugin; } \
	}
