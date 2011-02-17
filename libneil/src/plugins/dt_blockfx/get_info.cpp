/*
 * Buzz get info entry point
 *
 * History
 * Date       Version    Programmer         Comments
 * 16/2/03    1.0        Darrell Tam		Created
 */

#include <windows.h>
#include <memory.h>
#include <math.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <fstream>

#include "params.h"
#include "get_info.h"

using namespace std;

//------------------------------------------------------------------------------------------
vector<ParamStuff::AP> G_param_stuff;
vector<const CMachineParameter*> G_parameters;

// base structure that is passed to buzz so that it knows about what params we have
CMachineInfo G_macinfo = 
{
	MT_EFFECT,			// type
	MI_VERSION,	
	0,					// flags
	0,					// min tracks (filled in later)
	0,					// max tracks (filled in later)
	0,					// numGlobalParameters (filled in later)
	0,					// numTrackParameters (filled in later)
	NULL,				// pointer to global params filled in later
	0,
	NULL,
#ifdef _DEBUG
	"DT Block Effects (debug)",	// name
#else
#ifndef ALL_PARAMS_GLOBAL
	"DT Block Effects",		// name
#else
	"DT Block Effects (all global)",	// name
#endif
#endif
	"DT BlkFx",		// short name
	"Darrell Tam",		// author
	"About",			// menu items
	NULL			// ?
};


//------------------------------------------------------------------------------------------
void addParam(ParamStuff* t, ostringstream& name, ostringstream& desc)
{
	t->name_str = name.str();
	t->desc_str = desc.str();
	t->param.Name = t->name_str.c_str();
	t->param.Description = t->desc_str.c_str();
	G_param_stuff.push_back(ParamStuff::AP(t));
}

//------------------------------------------------------------------------------------------
struct ParamsInit { ParamsInit()
{
	int i;

	// generate parameter table of specific parameter classes
	#define _PARAM(BUZZ_TYPE, CODE_NAME, NAME, DESC, MIN_VAL, MAX_VAL, NO_VAL, FLAGS, DEFAULT) \
	{ \
		string def_desc; \
		STRSTR(def_desc, " (range=0x"<<hex<<MIN_VAL<<"..0x"<<MAX_VAL<<", default="<<DEFAULT<< ")"); \
		ostringstream name, desc; \
		name << NAME; \
		desc << DESC; \
		addParam(new Param_##CODE_NAME, name, desc); \
	}
	#define PARAM(BUZZ_TYPE, CODE_NAME, NAME, DESC, MIN_VAL, MAX_VAL, NO_VAL, FLAGS, DEFAULT) \
		_PARAM(BUZZ_TYPE, CODE_NAME, NAME, DESC, MIN_VAL, MAX_VAL, NO_VAL, FLAGS, DEFAULT) 

	MAIN_PARAMS

	#ifndef ALL_PARAMS_GLOBAL
	G_macinfo.numGlobalParameters = G_param_stuff.size();
	TRK_PARAMS
	G_macinfo.numTrackParameters = G_param_stuff.size()-G_macinfo.numGlobalParameters;
	G_macinfo.minTracks = 4;
	G_macinfo.maxTracks = N_FX_PARAM_SETS;

	#else // ALL_PARAMS_GLOBAL (no track params)
	#undef PARAM
	#define PARAM(BUZZ_TYPE, CODE_NAME, NAME, DESC, MIN_VAL, MAX_VAL, NO_VAL, FLAGS, DEFAULT) \
		_PARAM(BUZZ_TYPE, CODE_NAME, i<<":"<<NAME, i<<":"<<DESC, MIN_VAL, MAX_VAL, NO_VAL, FLAGS, DEFAULT) 
	for(i = 0; i < N_FX_PARAM_SETS; i++) { TRK_PARAMS }
	G_macinfo.numGlobalParameters = G_param_stuff.size();
	#endif

	#undef PARAM	

	// make a table of pointers to the actual parameter structures
	long n = G_param_stuff.size();
	G_parameters.resize(n);
	for(i = 0; i < n; i++) {
		ParamStuff& x = *G_param_stuff[i];
		G_parameters[i] = &x.param;
	}
	G_macinfo.Parameters = &G_parameters[0];

} } _G_params_init;



//------------------------------------------------------------------------------------------
extern "C"
{
	__declspec(dllexport) CMachineInfo const * __cdecl GetInfo()
	{
		return &G_macinfo;
	}
} 
