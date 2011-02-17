/*
 * Buzz parameter interface, see params.h for more information
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

using namespace std;

//------------------------------------------------------------------------------------------
void Params::update(const Params &p)
{
	tv_n = p.tv_n;

	#define PARAM(BUZZ_TYPE, CODE_NAME, NAME, DESC, MIN_VAL, MAX_VAL, NO_VAL, FLAGS, DEFAULT) \
				if(p.gv.CODE_NAME != NO_VAL) gv.CODE_NAME = p.gv.CODE_NAME;
	MAIN_PARAMS
	#undef PARAM

	for(int i = 0; i < tv_n; i++) {
		#define PARAM(BUZZ_TYPE, CODE_NAME, NAME, DESC, MIN_VAL, MAX_VAL, NO_VAL, FLAGS, DEFAULT) \
				if(p.tv[i].CODE_NAME != NO_VAL) tv[i].CODE_NAME = p.tv[i].CODE_NAME;
		TRK_PARAMS
		#undef PARAM	
	}
}

//------------------------------------------------------------------------------------------
void Params::copy(const Params &p)
{
	tv_n = p.tv_n;
	memcpy(&gv, &p.gv, sizeof(GVals));
	memcpy(&tv, &p.tv, sizeof(TVals)*tv_n);
}

//------------------------------------------------------------------------------------------
void Params::clear(void)
{
	tv_n = 0;
	memset(&gv, 0, sizeof(gv));
	memset(&tv, 0, sizeof(tv));
}


//------------------------------------------------------------------------------------------
ParamsVec::ParamsVec()
{
	v.resize(128); // arbitary initial size
	reset();
}

//------------------------------------------------------------------------------------------
void ParamsVec::reset(void)
{
	in = &v[0];
	in->clear();
	in->pos_abs = 0;
	in->tv_n = 0;
	in->samps_per_tick = -1; // tag as initialParam

	curr = &v[0];
	next = curr;
}


//------------------------------------------------------------------------------------------
void ParamsVec::incOutPos(void)
// go to the next output
{
	if(++curr == v.end()) curr = &v[0];
	next = curr+1;
	if(next == v.end()) next = &v[0];

	running[1] = running[0];
	running[0].update(*curr);
	tick_frac = 0.0f;
}


