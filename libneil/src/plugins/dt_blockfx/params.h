/******************************************************************************
Buzz parameter interface


MAIN_PARAMS
	Macro holding information for all main params (params that exist only once)
  
TRK_PARAMS
	Macro holding information for per-effect params

GVals
	Structure generated from MAIN_PARAMS that is stored inside of the machine
	interface class and is directly updated by Buzz. Also used in ParamsVec.

TVals
    Similar to GVals but generated from TRK_PARAMS for per-track values.

ParamsVec
	Vector of GVals used for storing previous GVals so that GVals corresponding
	to delayed audio data can be retrieved. Also performs linear interpolation of
	params when requested sample position falls between ticks.

	Behaves like a FIFO.

ParamStuff
	Basically a base class wrapper for CMachineParameter.

	A specific class is derived for each particular param, instantiated and a
	pointer to the CMachineParameter member is passed to Buzz so that it knows
	about each param.


History
	Date       Version    Programmer         Comments
	16/2/03    1.0        Darrell Tam		Created
******************************************************************************/

#ifndef _DTBLOCKTRK_PARAMS_H_
#define _DTBLOCKTRK_PARAMS_H_

#include <MachineInterface.h>
#include <string>
#include <vector>
#include <iostream>

#include "misc_stuff.h"
using namespace std;


// set what is 0 dB for "amplitude" params
const int zero_dB_setting = 0xa0;

// max value for "fraction" values
const long param_frac_max = 0xA000;

// number of effect sets
#ifdef ALL_PARAMS_GLOBAL
// if ALL_PARAMS_GLOBAL is set there are no track params
// params that are normally track params are generated as global params
// which is useful for "FruityLoops"
const long N_FX_PARAM_SETS = 4;
#else
const long N_FX_PARAM_SETS = 16;
#endif

// number of FFT block sizes (used when creating FFT plans)
const long N_FFT_BLK_SZ = 12;

// smallest block size, block size of 0 corresponds to this value
const long BLK_SZ_0 = 32;

// largest block size
const long max_blk_sz = BLK_SZ_0 << (N_FFT_BLK_SZ-1);

// internal output delay buffer length
const long x3_len = 44100*6;

// maximum samples delay that we can do
const long max_x3_delay = x3_len-max_blk_sz-256;



#define mdk_switch byte
#define pt_mdk_switch pt_switch

class MI_BlockFx;

//------------------------------------------------------------------------------------------
// paramaters are generated from these macros

#define MAIN_PARAMS \
	PARAM(mdk_switch, blk_sync, "BlockSync", "Synchronize block with this tick", SWITCH_OFF, SWITCH_ON, SWITCH_NO, 0, SWITCH_OFF) \
	PARAM(byte, interp_mode, "InterpMode", "Parameter Interpolation Mode (0=normal, 1=off, 2=continue previous)", 0, 2, 0xff, 0, 0) \
	PARAM(byte, mix_back, "MixBack", "Percentage mix back of original audio (0=0%, 0xA0=100%)", 0, 0xa0, 0xff, MPF_STATE, 0) \
	PARAM(byte, out_amp, "OutAmp", "Output Amplification (0xA0 = 0 dB)", 0, 0xe0, 0xff, MPF_STATE, zero_dB_setting) \
	PARAM(word, tick_delay, "TickDelay", "Audio is delayed for this many ticks" << def_desc, 0, 0x4000, 0xffff, MPF_STATE, 0x400) \
	PARAM(byte, blk_len, "BlockLen", "Audio block length" << def_desc, 0, N_FFT_BLK_SZ-1, 0xff, MPF_STATE, N_FFT_BLK_SZ-3) \
	PARAM(byte, overlap, "Overlap", "Percentage overlap of blocks" << def_desc, 5, 80, 0xff, MPF_STATE, 50)

#define TRK_PARAMS \
	FRAC_PARAM(freqA,	"FreqA",	"Frequency A" << def_desc, 0) \
	FRAC_PARAM(freqB,	"FreqB",	"Frequency B" << def_desc,	1) \
	PARAM(byte, amp,	"Amp",		"Amplitude (0xA0 = 0 dB)", 0, 0xe0, 0xff, MPF_STATE, zero_dB_setting) \
	PARAM(byte, effect, "Effect", "Effect (0=contrast, 1=smear, 2=clip, 3=weed, 4=shift add, 5=shift replace)", 0, 5, 0xff, MPF_STATE, 0) \
	FRAC_PARAM(fxval, 	"Value",	"Value" << def_desc, 0)

#define FRAC_PARAM(CODE_NAME, NAME, DESC, DEFAULT) \
	PARAM(word,CODE_NAME,NAME,DESC,0,param_frac_max,0xffff,MPF_STATE,DEFAULT*param_frac_max)

//------------------------------------------------------------------------------------------
#define PARAM(BUZZ_TYPE, CODE_NAME, NAME, DESC, MIN_VAL, MAX_VAL, NO_VAL, FLAGS, DEFAULT) \
	BUZZ_TYPE CODE_NAME; \
	static inline int CODE_NAME##_no_val(void) { return NO_VAL; } \
	static inline int CODE_NAME##_min(void) { return MIN_VAL; } \
	static inline int CODE_NAME##_max(void) { return MAX_VAL; }

#pragma pack(1)
struct GVals
// this structure is stored inside of our machine structure and is updated by Buzz
{
	MAIN_PARAMS
	static const GVals& _default(void);
};
struct TVals
// this structure is stored inside of our machine structure and is updated by Buzz
{
	TRK_PARAMS
	static const TVals& _default(void);
};
#undef PARAM
struct _Params
{
	GVals gv;
	TVals tv[N_FX_PARAM_SETS];
};
#pragma pack()

// combined global & track values
struct Params : public _Params
{
	int tv_n;	// number of valid track vals

	void update(const Params& p);
	void copy(const Params& p);
	void clear(void);

	Params() { clear(); }
};

//------------------------------------------------------------------------------------------
inline ostream& operator << (ostream& o, const Params& p)
// stream output operator of Params included for debugging
{
	o << "tv_n=" << p.tv_n;

	#define PARAM(BUZZ_TYPE, CODE_NAME, NAME, DESC, MIN_VAL, MAX_VAL, NO_VAL, FLAGS, DEFAULT) \
		o << ", " #CODE_NAME "=" << (long)p.gv.CODE_NAME;
	MAIN_PARAMS
	#undef PARAM

	for(int i=0; i < p.tv_n; i++) {
		#define PARAM(BUZZ_TYPE, CODE_NAME, NAME, DESC, MIN_VAL, MAX_VAL, NO_VAL, FLAGS, DEFAULT) \
			o << ", " << i << "-" NAME "=" << (long)p.tv[i].CODE_NAME;
		TRK_PARAMS
		#undef PARAM
	}
	return o;
}

//------------------------------------------------------------------------------------------
class ParamsVec
// FIFO of previous GVals & TVals, return interpolated values parameter values
{
public:
	struct Node : public Params
	{
		long pos_abs;			// absolute sample position for these params
		long samps_per_tick;	// number of samples to the next tick
		float ticks_per_samp;	// 1/samps_per_tick
	};

	vector<Node> v;	// vector of parameters
	Node *in;			// input position
	Node *curr;			// current output params
	Node *next;			// next set of output params
	Params running[2];	// all gvals & tvals filled, 0: current, 1: previous

	float tick_frac;	// current tick fraction between curr & next


	inline float _getParam(
		bool interp,
		long prev_val, long curr_val, long next_val,
		long min_val, long max_val, long no_val
	)
	{
		if(!interp) return curr_val;
		switch(running[0].gv.interp_mode) {
		case 1: // no interpolation
			return curr_val;

		case 2: // continue interp using previous params
			return limit_range(
				lin_interp(tick_frac+1.0f, prev_val, curr_val),
				min_val, max_val
			);

		default:// normal interp
			if(next_val == no_val) return curr_val;
			return limit_range(
				lin_interp(tick_frac, curr_val, next_val),
				min_val, max_val
			);
		}
	}


	// generate access methods for the current output param
	//
	// Access individual params with optional interpolate to next param (note, these
	// are valid even when param was set to NO_VAL)
	// <CODE_NAME>(bool interpolate = true); (MAIN_PARAMS)
	// <CODE_NAME>(int fx_set, bool interpolate = true); (TRK_PARAMS)
	//
	// Return whether param was not set during tick (buzz set it to NO_VAL)
	// <CODE_NAME>_no_val(); (MAIN_PARAMS)
	// <CODE_NAME>_no_val(int fx_set); (TRK_PARAMS)
	//

	#define PARAM(BUZZ_TYPE, CODE_NAME, NAME, DESC, MIN_VAL, MAX_VAL, NO_VAL, FLAGS, DEFAULT) \
	inline float CODE_NAME(bool interp = true) { \
		return _getParam(interp, \
			running[1].gv.CODE_NAME, running[0].gv.CODE_NAME, next->gv.CODE_NAME, \
			MIN_VAL, MAX_VAL, NO_VAL \
		); \
	} \
	inline bool CODE_NAME##_no_val(void) { return curr->gv.CODE_NAME == NO_VAL; }
	MAIN_PARAMS
	#undef PARAM

	#define PARAM(BUZZ_TYPE, CODE_NAME, NAME, DESC, MIN_VAL, MAX_VAL, NO_VAL, FLAGS, DEFAULT) \
	inline float CODE_NAME(int n, bool interp = true) { \
		return _getParam(interp,  \
			running[1].tv[n].CODE_NAME, running[0].tv[n].CODE_NAME, next->tv[n].CODE_NAME, \
			MIN_VAL, MAX_VAL, NO_VAL \
		); \
	} \
	inline bool CODE_NAME##_no_val(int n) { return curr->tv[n].CODE_NAME == NO_VAL; }
	TRK_PARAMS
	#undef PARAM


public:
	ParamsVec();
	void reset(void);

	// check whether the next params have been entered
	bool existsNext(void)
		{ return curr != in; }

	// find the absolute tick position
	long absTickPosNext(void)
		{ return next->pos_abs; }

	// go to the next param
	void incOutPos(void);

	int nTrkVals(void)
		{ return curr->tv_n; }

	// return samples per tick of current output param set
	long sampsPerTick(void)
		{ return curr->samps_per_tick; }

	// return true if the current output is invalid (initial param set)
	bool initialParam(void)
		{ return in->samps_per_tick == -1; }

	// get current input node
	Node& getIn(void)
		{ return *in; }

	// increment the input
	void incIn(void)
		{ if(++in == v.end()) in = &v[0]; }

	// notify in param updated
	void inUpdate(void)
	{
		if(in == curr) {
			running[0].update(*curr);
			running[1] = running[0];
		}
	}

	// return true if the FIFO is full
	bool full(void)
		{ return in+1==v.end()? curr==&v[0] : in+1==curr; }

	// set fraction for interpolation of output params
	void setFrac(long pos_abs)
	{
		tick_frac = limit_range(
			(pos_abs-curr->pos_abs)*curr->ticks_per_samp, 0.0f, 1.0f
		);
	}

};



//------------------------------------------------------------------------------------------
struct ParamStuff
// internal class to hold information about each parameter including data passed to
// Buzz on initialization
{
	CMachineParameter param;	// param structure read by buzz
	string name_str, desc_str;	// these hold the memory for strings in "param"

	virtual void desc(char*,int,MI_BlockFx&) = 0;	// describe text for value
	
	typedef auto_ptr<ParamStuff> AP;
};


// each parameter has a specific ParamStuff class associated with it
// this class includes a description method to print the value as text in the
// buzz window
//
// note, description code is in dt_blockfx.cpp
//
#define PARAM(BUZZ_TYPE, CODE_NAME, NAME, DESC, MIN_VAL, MAX_VAL, NO_VAL, FLAGS, DEFAULT) \
	struct Param_##CODE_NAME : public ParamStuff \
	{ \
		Param_##CODE_NAME() { \
			param.Type = pt_##BUZZ_TYPE; \
			param.MinValue = MIN_VAL; \
			param.MaxValue = MAX_VAL; \
			param.NoValue = NO_VAL; \
			param.Flags = FLAGS; \
			param.DefValue = DEFAULT; \
		} \
		virtual void desc(char*,int,MI_BlockFx&); \
	};

MAIN_PARAMS TRK_PARAMS
#undef PARAM

#endif
