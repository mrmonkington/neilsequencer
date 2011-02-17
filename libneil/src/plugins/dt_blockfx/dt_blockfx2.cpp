// DT block fx
//
// See DT_block_fx.html for more information
//
// History
// Date       Version    Programmer         Comments
// 16/2/03    1.0        Darrell Tam		Created
//

#include <windows.h>
#include <memory.h>
#include <rfftw.h>
#include <math.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <assert.h>
#include <vector>
#include <algorithm>
#include <fstream>

#include "dt_blockfx.h"
#include "blkfx.h"
#include "params.h"
#include "wrap_process.h"
#include "FreqToNote.h"
#include "get_info.h"

using namespace std;

#pragma optimize ("awy", on) 

#ifdef _DEBUG
ofstream debug("c:\\t.1");
#endif

//------------------------------------------------------------------------------------------
MI_BlockFx::MI_BlockFx()
{
	long i;

	// populate the effect table
	blk_fx.push_back(BlkFx::AP(new BlkFxContrast));
	blk_fx.push_back(BlkFx::AP(new BlkFxSmear));
	blk_fx.push_back(BlkFx::AP(new BlkFxClip));
	blk_fx.push_back(BlkFx::AP(new BlkFxWeed));
	blk_fx.push_back(BlkFx::AP(new BlkFxShiftAdd));
	blk_fx.push_back(BlkFx::AP(new BlkFxShiftReplace));
	for(i = 0; i < blk_fx.size(); i++) blk_fx[i]->mi = this;
	
	// stuff for buzz

	#ifndef ALL_PARAMS_GLOBAL
	TrackVals = &params.tv;
	params.tv_n = 0;
	#else
	params.tv_n = N_FX_PARAM_SETS;
	TrackVals = NULL;
	#endif

	GlobalVals = &params.gv;
	AttrVals = NULL;


	rand_i = 1;

	reset();
}


//------------------------------------------------------------------------------------------
inline float freq_value(float v)
{
	if(v == 0.0f) return 0.0f;
	return powf(2.0f, lin_interp((float)v/param_frac_max, -10.2f, 0));
}

inline float amp_value(float v)
{
	if(v == 0) return 0;
	return powf(10, (v-zero_dB_setting)/40.0f);
}

inline float fxval_value(float v)
{ return (float)v/param_frac_max; }

inline float blk_len_value(float v)
{ return limit_range(v, 0, N_FFT_BLK_SZ-1); }

inline float overlap_value(float v)
{
	return limit_range(v, GVals::overlap_min(), GVals::overlap_max())/100.0f;
}

inline float tick_delay_value(float v, long samps_per_tick)
// tick delay is returned in samples
{
	return limit_range(
		v/0x100*samps_per_tick,
		BLK_SZ_0,
		max_x3_delay
	);
}
inline float mix_back_value(float v)
{ return v/GVals::mix_back_max(); }


//------------------------------------------------------------------------------------------
long MI_BlockFx::getBlkLenDisp(void)
{
	long r = params_disp.gv.blk_len;
	unsigned long n = tick_delay_value(params_disp.gv.tick_delay, pMasterInfo->SamplesPerTick);
	while(BLK_SZ_0 << (r-1) > n) r--;
	return r;
}

//------------------------------------------------------------------------------------------
inline void freq_desc(char* txt, int v, MI_BlockFx& i)
{
	long blk_len = BLK_SZ_0 << i.getBlkLenDisp();
	float freq = .5f*((long)(freq_value(v)*blk_len))/blk_len*i.pMasterInfo->SamplesPerSec;
	if(freq < 1000.0f) txt += sprintf(txt, "%.0fHz,", freq);
	else if(freq < 10000.0f) txt += sprintf(txt, "%.2fkHz,", freq*1e-3);
	else txt += sprintf(txt, "%.1fkHz,", freq*1e-3f);
	FreqToNote(txt, freq);
}

void Param_freqA::desc(char* txt, int v, MI_BlockFx& i)
{ freq_desc(txt, v, i); }

void Param_freqB::desc(char* txt, int v, MI_BlockFx& i)
{ freq_desc(txt, v, i); }

void Param_amp::desc(char* txt, int v, MI_BlockFx& i)
{
	float t = amp_value(v);
	if(t > 1e-10) sprintf(txt, "%.1fdB", 20*log10(t));
	else strcpy(txt, "-inf dB");
}

void Param_effect::desc(char* txt, int v, MI_BlockFx& i)
{
	i.blkFx(v).name(txt);
}

void Param_fxval::desc(char* txt, int v, MI_BlockFx& i)
{
	// unfortunately we can't find the correct param to use the real description

	//i.blkFx(0).desc(txt, fxval_value(v));
	//i.blkFx(i.gvals_disp.fx[fx_set_n].effect).desc(txt, fxval_value(v));
	sprintf(txt, "%.3f", fxval_value(v));
}

void Param_out_amp::desc(char* txt, int v, MI_BlockFx& i)
{
	float t = amp_value(v);
	if(t > 1e-10) sprintf(txt, "%.1fdB", 20*log10f(t));
	else strcpy(txt, "-inf dB");
}

void Param_overlap::desc(char* txt, int v, MI_BlockFx& i)
{ sprintf(txt, "%2.0f%%", overlap_value(v)*100.0f); }

void Param_blk_len::desc(char* txt, int v, MI_BlockFx& i)
{
	i.params_disp.gv.blk_len = v;
	sprintf(txt, "%d", BLK_SZ_0 << i.getBlkLenDisp());
}

void Param_mix_back::desc(char* txt, int v, MI_BlockFx& i)
{ sprintf(txt, "%.1f%%", mix_back_value(v)*100.0f); }

void Param_tick_delay::desc(char* txt, int v, MI_BlockFx& i)
{
	i.params_disp.gv.tick_delay = v;
	long spt = i.pMasterInfo->SamplesPerTick;
	float t = tick_delay_value(v, spt);
	sprintf(txt, "%.2f", t/spt);
}
void Param_blk_sync::desc(char* txt, int v, MI_BlockFx& i)
{ strcpy(txt, v?"Block sync":"Free run"); }

void Param_interp_mode::desc(char* txt, int v, MI_BlockFx& i)
{
	switch(v) {
	case 1: sprintf(txt, "None"); break;
	case 2: sprintf(txt, "Continue Previous"); break;
	default: sprintf(txt, "Normal");
	}
}


//------------------------------------------------------------------------------------------
void MI_BlockFx::reset(void)
{
	params_vec.reset();

	x0i_abs = 0;
	x0_i = 0;
	x0_n = 0;
	x0_lastreal_abs = 0;

	x3o_abs = 0;
	x3_o = 0;
	x3_lastreal_abs = 0;

	prev_blkend_abs = 0;
	out_delay_n = 0;

	buffering_block = false;

	curr_blk_abs = 0;
	blk_len = 0;
	blk_len_fft = 0;

	prev_work_mode = 0;

}

//------------------------------------------------------------------------------------------
MI_BlockFx::~MI_BlockFx()
{
	for_each(plan_fft.begin(), plan_fft.end(), rfftw_destroy_plan);
	for_each(plan_ifft.begin(), plan_ifft.end(), rfftw_destroy_plan);
}

//------------------------------------------------------------------------------------------
void MI_BlockFx::Init(CMachineDataInput * const pi)
{
	this_machine = pCB->GetThisMachine();
	long i;
	for(i = 0; i < N_FFT_BLK_SZ; i++) {
		plan_fft.push_back(rfftw_create_plan(BLK_SZ_0<<i, FFTW_REAL_TO_COMPLEX, FFTW_ESTIMATE));
		plan_ifft.push_back(rfftw_create_plan(BLK_SZ_0<<i, FFTW_COMPLEX_TO_REAL, FFTW_ESTIMATE));
	}

    x0.resize(max_blk_sz+2048);
    x1.resize(max_blk_sz);
    x2.resize(max_blk_sz);
	x3.resize(x3_len);
}

//------------------------------------------------------------------------------------------
void MI_BlockFx::SetNumTracks(int const n)
{
	#ifndef ALL_PARAMS_GLOBAL
	params.tv_n = n;
	#endif
}

//------------------------------------------------------------------------------------------
void MI_BlockFx::Tick(void)
{
	ParamsVec::Node* n;

	if(params_vec.initialParam() || // first param
		params_vec.getIn().pos_abs == x3o_abs || // no new audio
		params_vec.full() // can't fit another param update
	) {
		// stay on the same param
		n = &params_vec.getIn();
		n->update(params);
	}
	else {
		// go to the next param
		params_vec.incIn();
		n = &params_vec.getIn();
		n->copy(params);
		n->pos_abs = x3o_abs;
	}


	n->samps_per_tick = pMasterInfo->SamplesPerTick;
	n->ticks_per_samp = 1.0f/n->samps_per_tick;

	params_vec.inUpdate();

	params_disp.update(params);
}

//------------------------------------------------------------------------------------------
char const *MI_BlockFx::DescribeValue(int const param, int const value)
{
    static char txt[128];
	if(param < 0 || param >= G_param_stuff.size()) return NULL;
	G_param_stuff[param]->desc(txt, value, *this);
	return txt;
}

//------------------------------------------------------------------------------------------
void MI_BlockFx::processFFT(void)
//
// process the FFT'd data (x1) using the fx_params
//
{
	long i;
	
	// do the FFT
	rfftw_one(plan_fft[plan_n], &x0[x0_i], &x1[0]);

	// scale FFT before processing to reduce overflows
	float scale = 1e-3f/blk_len_fft;
	float in_pwr = 0;
	//x1_absmaxnorm = 0.0f;
	//x1_absminnorm = 1e34f;
	//x1[0] *= scale;
	for(i = 1; i < blk_len_fft; i++) {
		//if(t > x1_absmaxnorm) x1_absmaxnorm = t;
		//if(t < x1_absminnorm) x1_absminnorm = t;
		x1[i] *= scale;
		float t = x1[i]*x1[i];
		in_pwr += t;
	}
	total_out_pwr = in_pwr;

	for(i = 0; i < params_vec.nTrkVals(); i++) {
		// get the parameters for this processing
		float freqA = params_vec.freqA(i, true);
		float freqB = params_vec.freqB(i, true);

		// do nothing if the effect is off
		if(freqA == 0.0f && freqB == 0.0f) continue;

		curr_start_bin = limit_range(freq_value(freqA)*blk_len_fft/2, 1, blk_len_fft/2);
		curr_stop_bin = limit_range(freq_value(freqB)*blk_len_fft/2, 1, blk_len_fft/2);
		curr_amp = amp_value(params_vec.amp(i, true));
		curr_fxval = fxval_value(params_vec.fxval(i, true));

		// run the effect if it isn't turned off
		blkFx(params_vec.effect(i, false)).process();
	}
	x1[0] = 0; // no DC too hard to deal with

	// perform inverse FFT
	rfftw_one(plan_ifft[plan_n], &x1[0], &x2[0]);

	// set to undo scaling & match input power
	double s = amp_value(params_vec.out_amp())*1e3;
	if(total_out_pwr > 0) s *= sqrt(in_pwr/total_out_pwr);
	if(s > 1e30) x2_scale = 1.0f;
	else if(s < 1e-30) x2_scale = 0.0f;
	else x2_scale = (float)s;

	//debug << scientific << "x2_scale=" << x2_scale << ", in_pwr=" << in_pwr << ", total_out_pwr="<<total_out_pwr<<endl;
}


//------------------------------------------------------------------------------------------
void MI_BlockFx::mixToOutputBuffer(void)
//
// mix from x2 (post-IFFT) & x0 (pre-FFT) into x3
//
{
	// number of samples over which current blk fades in over
	long fadein_n = prev_blkend_abs - curr_blk_abs;

	// check whether there's an overlap
	if(fadein_n < 0) {
		// arbitary fade out of previous data

		// offsets relative to x3_o
		long prev_blkend_rel = prev_blkend_abs - x3o_abs;
		long fade_start_rel = prev_blkend_rel - BLK_SZ_0;

		// don't fade data that has already been output
		if(fade_start_rel < 0) fade_start_rel = 0;

		long n = prev_blkend_rel-fade_start_rel;
		if(n > 0) wrapProcess(PFadeOut(n),x3, wrap(x3_o+fade_start_rel, x3), n);

		// zero fill if there's a gap between end of previous block and
		// fade-in of this block
		n = -fadein_n;
		fadein_n = BLK_SZ_0; // arbitary fade in of new block
		wrapProcess(PZero(), x3, wrap(x3_o+prev_blkend_rel, x3), n+fadein_n);
	}

	fadein_n = limit_range(fadein_n, 0, fade_samps);

	// absolute start position of current blk into the output buffer
	long curr_blk_i = wrap(x3_o+curr_blk_abs-x3o_abs, x3);

	if(mix_back <= 0.0f) {
		// output data is completely processed
		if(fadein_n > 0)
			wrapProcess(PXFade(&x2[0], x2_scale, fadein_n), x3, curr_blk_i, fadein_n);

		wrapProcess(PScaleCopy(&x2[fadein_n], x2_scale),
			x3, wrap(curr_blk_i+fadein_n, x3), blk_len-fadein_n);
	}
	else if(mix_back >= 1.0f) {
		// output data is completely original
		if(fadein_n > 0)
			wrapProcess(PXFade(&x0[x0_i], 1.0f, fadein_n), x3, curr_blk_i, fadein_n);

		wrapProcess(PScaleCopy(&x0[x0_i+fadein_n], 1.0f),
			x3, wrap(curr_blk_i+fadein_n, x3), blk_len-fadein_n);
		//wrapProcess(PScaleCopy(&x0[x0_i], 1.0f),
		//	x3, wrap(curr_blk_i, x3), blk_len);
	}
	else {
		// output data is a mix of original and processed
		x2_scale *= 1.0f-mix_back; // processed data
		float x0_scale = mix_back; // original data

		if(fadein_n > 0)
			wrapProcess(PXFade2(&x2[0], x2_scale, &x0[x0_i], x0_scale, fadein_n),
				x3, curr_blk_i, fadein_n);

		wrapProcess(PScaleCopy2(&x2[fadein_n], x2_scale, &x0[x0_i+fadein_n], x0_scale),
			x3, wrap(curr_blk_i+fadein_n, x3), blk_len-fadein_n);
	}
}

//------------------------------------------------------------------------------------------
bool MI_BlockFx::paramsRdy(void)
//
// return true if we can interpolate the current block of data or the output delay
// is too short
//
{
	if(params_vec.existsNext()) {
		// parameters can be interpolated as there is a next value
		params_vec.setFrac(x0i_abs);

		// get interpolated params for this tick

		// update the output delay only if the tick delay was explicitly set
		if(!params_vec.tick_delay_no_val())
			out_delay_n = tick_delay_value(params_vec.tick_delay(true), params_vec.sampsPerTick());
		curr_blk_abs = out_delay_n+x0i_abs;

		plan_n = blk_len_value(params_vec.blk_len(true));
		blk_len_fft = BLK_SZ_0 << plan_n;

		buffering_block = true;
		return true;
	}

	// do nothing if this is the initial (default) param
	if(params_vec.initialParam()) return false;

	// we can't interpolate the tick delay params yet but check whether
	// the tick delay on the previously encountered param forces a block
	// to be output
	if(!params_vec.tick_delay_no_val())
		out_delay_n = tick_delay_value(params_vec.tick_delay(false), pMasterInfo->SamplesPerTick);
	curr_blk_abs = out_delay_n+x0i_abs;

	// if the next block doesn't need to be output, do nothing
	if(curr_blk_abs - x3o_abs >= buf_n) return false;

	// get current params (non-interpolated) params
	plan_n = blk_len_value(params_vec.blk_len(false));
	blk_len_fft = BLK_SZ_0 << plan_n;

	buffering_block = true;
	return true;
}


//------------------------------------------------------------------------------------------
bool MI_BlockFx::blkRdy(void)
//
// return true if there's enough data to process the current block or we are forced
// to output
//
{
	// are we forced to output a block?
	if(curr_blk_abs - x3o_abs < buf_n) {

		// check how much data is in the input buffer and reduce block size if not enough
		// so as to reduce processing load 
		while((BLK_SZ_0<<(plan_n-1)) > x0_n) plan_n--;
		if(plan_n < 0) plan_n = 0;

		blk_len_fft = BLK_SZ_0 << plan_n;
		
		// check whether how much data is actually available
		long n = blk_len_fft-x0_n;
		if(n <= 0) blk_len = blk_len_fft;
		else {
			// need to zero-pad the pre-fft buffer as there isn't enough data
			blk_len = x0_n;

			// delete old data if there's not enough room
			if(x0_i+blk_len_fft > x0.size()) {
				if(x0_n > 0) memcpy(&x0[0], &x0[x0_i], x0_n*sizeof(float));
				x0_i = 0;
			}

			// zero unfilled data
			memset(&x0[x0_i+blk_len], 0, n*sizeof(float));
		}
		return true;

	}

	// is there enough data to process the block?
	if(x0_n >= blk_len_fft) {
		blk_len = blk_len_fft;
		return true;
	}

	// not enough data to process block
	return false;
}

//------------------------------------------------------------------------------------------
void MI_BlockFx::nextBlk(void)
//
// update everything ready for the next block
//
{
	// determine overlap with next block
	long nonoverlap_n = limit_range(
		blk_len-fade_samps,
		8, blk_len
	);

	// check for block sync & increment params_vec
	while(params_vec.existsNext()) {
		long tick_rel = params_vec.absTickPosNext() - x0i_abs;
		if(tick_rel > nonoverlap_n) break;
		params_vec.incOutPos();
		if(!params_vec.blk_sync_no_val() && params_vec.blk_sync(false)) {
			// force next block to start on this tick
			/*debug << "forcing tick sync"
				<< ", nonoverlap_n=" << nonoverlap_n
				<< ", tick_rel=" << tick_rel
				<< endl;*/
			nonoverlap_n = tick_rel;
			break;
		}
	}
	/*
	long fadein_n = prev_blkend_abs - curr_blk_abs;
	debug << "samps_per_tick" << params_vec.curr->samps_per_tick
		<<", fadein_n="<<fadein_n
		<<", nonoverlap_n="<<nonoverlap_n
		<<", blk_len="<<blk_len
		<<", blk_len_fft="<<blk_len_fft
		<< endl;
	*/

	// update positions	
	x0_i += nonoverlap_n;
	x0_n -= nonoverlap_n;
	x0i_abs += nonoverlap_n;

	long blkend_abs = curr_blk_abs + blk_len;

	if(blkend_abs-prev_blkend_abs > 0) prev_blkend_abs = blkend_abs;
	buffering_block = false;
}

//------------------------------------------------------------------------------------------
bool MI_BlockFx::Work(float *buf, int _buf_n, int const work_mode)
{
	buf_n = _buf_n;

	if (work_mode == WM_NOIO) return false;
	if (work_mode == WM_READ) return true;

	if(work_mode == WM_WRITE) {
		// writing has finished, determine whether we need to keep outputting

		if(prev_work_mode != WM_WRITE) {
			// first block after real data
			x0_lastreal_abs = x3o_abs+buf_n; // amount of data remaining to be processed
			x3_lastreal_abs = curr_blk_abs+blk_len; // end of previous block
		}

		if(x0i_abs-x0_lastreal_abs <= 0) {
			// haven't finished with the real input data, keep track of where
			// the block that it belongs to will end
			long t = curr_blk_abs+blk_len;
			if(t-x3_lastreal_abs > 0) x3_lastreal_abs = t;
		}
		else if(x3o_abs-x3_lastreal_abs > 0) {
			// all done when we pass the end of the last block to contain real data
			return false;
		}

		// input has stopped, still writing
		memset(buf, 0, buf_n*sizeof(float));
	}

	// shift out used data if not enough room for the new buffer
	if(x0_i+x0_n+buf_n > x0.size()) {
		// hopefully this won't happen - increase buffer length if too full
		if(x0_n+buf_n > x0.size()) x0.resize(x0_n+buf_n);

		// shift out old data
		if(x0_n > 0) memcpy(&x0[0], &x0[x0_i], x0_n*sizeof(float));
		x0_i = 0;
	}

	// copy input data into the pre-fft buffer
	memcpy(&x0[x0_i+x0_n], buf, buf_n*sizeof(float));
	x0_n += buf_n;

	while(1) {
		if(!buffering_block) {
			if(!paramsRdy()) break;
		}

		if(buffering_block) {
			if(!blkRdy()) break;
			fade_samps = overlap_value(params_vec.overlap(true))*blk_len;

			// fraction of original data
			mix_back = mix_back_value(params_vec.mix_back(true));
			if(mix_back < 1.0f) processFFT();
			//x2_scale = 1.0f;
			//memcpy(&x2[0], &x0[x0_i], blk_len*sizeof(float));
			mixToOutputBuffer();
			nextBlk();
		}
	}

	// copy from x3 (output FIFO) to the output buffer
	wrapProcess(PCopyOut(buf), x3, x3_o, buf_n);

	// adjust output pointers
	x3_o = wrap(buf_n+x3_o, x3);
	x3o_abs += buf_n;

	prev_work_mode = work_mode;

	return true;
}

//------------------------------------------------------------------------------------------
void MI_BlockFx::Command(int const i)
{
	switch(i) {
	case 0: {
		ostringstream s;
		s << "Block FX, written by Darrell Tam" << endl
		  << endl
		  << "email: ymtam2@tpg.com.au" << endl
		  << endl
		  << "build date: " << __DATE__ << endl
		  << endl
		  << "Uses FFTW 2.1.3 (Fastest-Fourier-Transform in the west), www.fftw.org" << endl
		;
		pCB->MessageBox(s.str().c_str());
		}
	}
}


//------------------------------------------------------------------------------------------
extern "C"
{
	__declspec(dllexport) CMachineInterface * __cdecl CreateMachine()
	{
		return new MI_BlockFx;
	}
} 


