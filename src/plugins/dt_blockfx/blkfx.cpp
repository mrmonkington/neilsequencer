/******************************************************************************
DT Block FX - classes to perform actual effects on the FFT'd data

BlkFx
	Base class for effects

BlkFx*
	Individual classes to perform a particular effect on the FFT spectrum

History
	Date       Version    Programmer         Comments
	16/2/03    1.0        Darrell Tam		Created
******************************************************************************/

#include <fstream>
#include "blkfx.h"
#include "dt_blockfx.h"

//------------------------------------------------------------------------------------------
// cos & sin table used for phase randomization
const int sincos_table_bits = 12;
cplxf sincos_table[1<<sincos_table_bits];

struct SinCosInit{ SinCosInit() {
	float angle_step = 2.0f*PI/(1<<sincos_table_bits);
	float angle = 0;
	for(long i = 0; i < (1<<sincos_table_bits); i++, angle += angle_step)
		sincos_table[i] = polar(1.0f, angle);
}} _sin_cos_init;


//------------------------------------------------------------------------------------------
void BlkFx::process(void)
// default BlkFx process calls 
{
	if(mi->curr_start_bin <= mi->curr_stop_bin) {
		process(
			&(mi->x1[mi->curr_start_bin]),				// real start
			&(mi->x1[mi->blk_len_fft-mi->curr_start_bin]),	// imag start
			&(mi->x1[mi->curr_stop_bin+1])				// real end
		);
	}
	else {
		// process in 2 parts - everything except for the region
		process(
			&(mi->x1[1]),								// real start
			&(mi->x1[mi->blk_len_fft-1]),				// imag start
			&(mi->x1[mi->curr_stop_bin+1])				// real end
		);
		process(
			&(mi->x1[mi->curr_start_bin]),				// real start
			&(mi->x1[mi->blk_len_fft-mi->curr_start_bin]),	// imag start
			&(mi->x1[mi->blk_len_fft/2])				// real end
		);
	}
}

//------------------------------------------------------------------------------------------
void BlkFx::process(float* x1_re, float* x1_im, float* x1_re_end)
// default process : scale block, no effect
{
	float in_pwr = 0;
	float curr_amp = mi->curr_amp;
	if(curr_amp == 1.0f) return;

	for(; x1_re != x1_re_end; x1_re++, x1_im--) {
		in_pwr += norm(*x1_re, *x1_im);
		*x1_re *= curr_amp;
		*x1_im *= curr_amp;
	}
	
	// adjust output power according to amp
	mi->total_out_pwr += (curr_amp*curr_amp-1)*in_pwr;
}



//------------------------------------------------------------------------------------------
inline void BlkFxContrast::process(
			float* x1_re, float* x1_im, float* x1_re_end, float raise, bool raise_neg)
// Change the contrast of the spectrum
//
// perform: abs(X)^(2.raise+1) on part of the spectrum
//
// raise_neg should be passed as a constant to help optimizer
{
	float in_pwr = 0;
	float out_pwr = 0;
	float *t[3] = {x1_re, x1_im, x1_re_end};

	// set limits to prevent overflow
	float limit;
	float r = raise*2;
	if(r < 0) {
		if(r >= -1) limit = 1e-28f;
		else limit = powf(1e28f, 1.0f/r);
	}
	else {
		if(r < 1) limit = 1e28f;
		else limit = powf(1e28f, 1.0f/r);
	}

	x1_re = t[0]; x1_im = t[1]; x1_re_end = t[2];
	for(; x1_re != x1_re_end; x1_re++, x1_im--) {
		float t0 = norm(*x1_re, *x1_im);
		float t1;

		in_pwr += t0;
		if(raise_neg) {
			if(t0 < limit) t1 = 1.0f;
			else t1 = powf(t0, raise);
		}
		else {
			if(t0 > limit) t0 = limit;
			t1 = powf(t0, raise);
		}

		out_pwr += t0*t1*t1;
		*x1_re *= t1;
		*x1_im *= t1;
	}

	// match output power to input power
	double s = mi->curr_amp;
	if(out_pwr > 0) s *= sqrt(in_pwr/out_pwr);
	float scale;
	if(s > 1e30) scale = 1.0f;
	else if(s < 1e-30) scale = 0.0f;
	else scale = (float)s;

	x1_re = t[0]; x1_im = t[1]; x1_re_end = t[2];
	for(; x1_re != x1_re_end; x1_re++, x1_im--) {
		*x1_re *= scale;
		*x1_im *= scale;
	}

	// adjust total output power according to amp
	mi->total_out_pwr += (mi->curr_amp*mi->curr_amp-1)*in_pwr;
}

//------------------------------------------------------------------------------------------
void BlkFxContrast::process(float* x1_re, float* x1_im, float* x1_re_end)
{
	float t = mi->curr_fxval;
	if(t == 0.0f) {
		BlkFx::process(x1_re, x1_im, x1_re_end); // effect off
		return;
	}
	t -= 0.5f;
	if(t >= 0.0f)
		process(x1_re, x1_im, x1_re_end, lin_interp(t*t*4.0f, 0.0f, 4.0f), false);
	else
		process(x1_re, x1_im, x1_re_end, lin_interp(t*t*4.0f, 0.0f, -0.5f), true);
}

//------------------------------------------------------------------------------------------
void BlkFxSmear::process(float* x1_re, float* x1_im, float* x1_re_end)
//
// randomize the phase
//
{
	float smear = mi->curr_fxval;
	if(smear == 0.0f) BlkFx::process(x1_re, x1_im, x1_re_end); // effect off

	float in_pwr = 0;
	float curr_amp = mi->curr_amp;
	long rand_i = mi->rand_i;

	for(; x1_re != x1_re_end; x1_re++, x1_im--) {
		cplxf x(*x1_re, *x1_im);
		in_pwr += norm(x);

		int angle = rand_i & ((1<<sincos_table_bits)-1);
		x = x*(sincos_table[angle]*smear+1.0f-smear)*curr_amp;
		*x1_re = x.real();
		*x1_im = x.imag();
		rand_i = prbs29(rand_i);
	}

	// adjust output power according to amp
	mi->total_out_pwr += (curr_amp*curr_amp-1)*in_pwr;
	mi->rand_i = rand_i;
}

//------------------------------------------------------------------------------------------
void BlkFxPhaseShift::process(float* x1_re, float* x1_im, float* x1_re_end)
//
// shift the phase by a constant angle
//
{
	float curr_amp = mi->curr_amp;
	const long f = (1<<sincos_table_bits);
	long angle = (mi->curr_fxval-0.5f)*f;
	cplxf shift = sincos_table[angle & (f-1)];
	for(; x1_re != x1_re_end; x1_re++, x1_im--) {
		cplxf x(*x1_re, *x1_im);
		x = x*shift*curr_amp;
		*x1_re = x.real();
		*x1_im = x.imag();
	}

}

//------------------------------------------------------------------------------------------
void BlkFxWeed::process(float* x1_re, float* x1_im, float* x1_re_end)
// weed out bins with magnitude lower than the weed threshold
{
	float *t[3] = {x1_re, x1_im, x1_re_end};

	float in_pwr = 0;
	float out_pwr = 0;

	// scale threshold: -1 .. 1
	float thresh = mi->curr_fxval*2.0f-1.0f; 

	if(thresh >= 0) {
		// weed (set to 0) any bins lower than the threshold (arbitary scaling)
		thresh *= thresh*24.0f*mi->total_out_pwr/mi->blk_len_fft;

		for(; x1_re != x1_re_end; x1_re++, x1_im--) {
			float t0 = norm(*x1_re, *x1_im);
			in_pwr += t0;

			if(t0 < thresh) {
				*x1_re = 0;
				*x1_im = 0;
			}
			else out_pwr += t0;		
		}
	}
	else {
		// weed (set to 0) any bins higher than the threshold (arbitary scaling)
		thresh = 1+thresh;
		thresh *= thresh*24.0f*mi->total_out_pwr/mi->blk_len_fft;

		float rt = sqrtf(thresh);

		for(; x1_re != x1_re_end; x1_re++, x1_im--) {
			float t0 = norm(*x1_re, *x1_im);
			in_pwr += t0;

			if(t0 >= thresh) {
				*x1_re = 0;
				*x1_im = 0;
			}
			else out_pwr += t0;
		}
	}

	// match output power to input power
	float scale = mi->curr_amp;
	if(out_pwr > 1e-30f) scale *= sqrtf(in_pwr/out_pwr);

	x1_re = t[0]; x1_im = t[1]; x1_re_end = t[2];
	for(; x1_re != x1_re_end; x1_re++, x1_im--) {
		*x1_re *= scale;
		*x1_im *= scale;
	}

	// adjust total output power according to amp
	mi->total_out_pwr += (mi->curr_amp*mi->curr_amp-1)*in_pwr;
}

//------------------------------------------------------------------------------------------
void BlkFxClip::process(float* x1_re, float* x1_im, float* x1_re_end)
// clip bins with magnitude higher than the threshold to the threshold
{
	float *t[3] = {x1_re, x1_im, x1_re_end};

	float in_pwr = 0;
	float out_pwr = 0;

	// arbitary threshold function
	float thresh = 1-mi->curr_fxval;
	thresh *= thresh*8.0f*mi->total_out_pwr/mi->blk_len_fft;

	float rt = sqrtf(thresh);

	for(; x1_re != x1_re_end; x1_re++, x1_im--) {
		float t0 = norm(*x1_re, *x1_im);
		in_pwr += t0;

		if(t0 >= thresh && t0 > 1e-37f) {
			float t1 = rt / sqrtf(t0);
			*x1_re *= t1;
			*x1_im *= t1;
			out_pwr += t1*t1*t0;

		}
		else out_pwr += t0;
	}
	
	// match output power to input power
	float scale = mi->curr_amp;
	if(out_pwr > 1e-30f) scale *= sqrtf(in_pwr/out_pwr);

	x1_re = t[0]; x1_im = t[1]; x1_re_end = t[2];
	for(; x1_re != x1_re_end; x1_re++, x1_im--) {
		*x1_re *= scale;
		*x1_im *= scale;
	}

	// adjust total output power according to amp
	mi->total_out_pwr += (mi->curr_amp*mi->curr_amp-1)*in_pwr;
}

//------------------------------------------------------------------------------------------
inline float/*-0.5..0.5*/ BlkFxShiftAdd::getFreq(float v/*0..1*/)
// return frequency as a fraction between -.5 & .5
// maps the linear input "v" to an arbitary linear+exponential function
// (linear transitioning into exponential)
{
	const float lin = 2048.0f/32768.0f;
	const int exp_raise = 8;
	const float exp_scale = (1.0f-lin)/(1<<exp_raise);
	v = (v-0.5f)*2;

	bool pos = true;
	if(v < 0) {
		v = -v;
		pos = false;
	}
	float r = (powf(2, v*exp_raise)-1.0f)*exp_scale + v*lin;
	return (pos ? r:-r)*0.5f;
}

//------------------------------------------------------------------------------------------
void BlkFxShiftAdd::desc(char *txt, float v) {
	float blk_len = BLK_SZ_0 << mi->getBlkLenDisp();
	sprintf(txt, "%.0fHz", (long)(getFreq(v)*blk_len)/blk_len*mi->pMasterInfo->SamplesPerSec);
}

//------------------------------------------------------------------------------------------
template <class A> inline void shiftApply(
	A& a,
	vector<float>& x,	// FFT spectrum as prduced by rfftw
	long fft_len,
	long start_bin,		// must be in range of fft size
	long dest_bin,		// can be out of range
	long n_bins
)
// instead of using virtual functions, this is implemented as a template so as to help optimizer
{
	// adjust ranges if destination goes outside of fft data
	if(dest_bin < 1)  {
		start_bin += (1-dest_bin);
		n_bins -= (1-dest_bin);
		dest_bin = 1;
	}
	if(dest_bin+n_bins >= fft_len/2-1) n_bins = fft_len/2-1 - dest_bin;

	if(n_bins < 1) return;

	float *xs_re = &(x[start_bin]);
	float *xs_im = &(x[fft_len-start_bin]);
	float *xd_re = &(x[dest_bin]);
	float *xd_im = &(x[fft_len-dest_bin]);

	if(start_bin > dest_bin) {
		// process spectrum forwards
		float *xs_re_end = xs_re+n_bins;
		
		for(; xs_re != xs_re_end; xs_re++, xs_im--, xd_re++, xd_im--) {
			cplxf src(*xs_re, *xs_im), dst(*xd_re, *xd_im);
			a.apply(src, dst);
			*xd_re = dst.real();
			*xd_im = dst.imag();
		}
	}
	else {
		// process spectrum in reverse
		xs_re += n_bins-1;
		xs_im -= n_bins-1;
		xd_re += n_bins-1;
		xd_im -= n_bins-1;
		float *xs_re_end = xs_re-n_bins;
		
		for(; xs_re != xs_re_end; xs_re--, xs_im++, xd_re--, xd_im++) {
			cplxf src(*xs_re, *xs_im), dst(*xd_re, *xd_im);
			a.apply(src, dst);
			*xd_re = dst.real();
			*xd_im = dst.imag();
		}
	}
}

//------------------------------------------------------------------------------------------
template <class A> inline void shiftApply(A& a, MI_BlockFx &i)
//
// determine whether to process inside or outside of frequency range
// template to help optimizer
//
{
	a.curr_amp = i.curr_amp;
	long shift = a.getFreq(i.curr_fxval)*i.blk_len_fft;
	if(i.curr_start_bin <= i.curr_stop_bin) {
		// inside frequency range
		shiftApply(a, i.x1, i.blk_len_fft,
			i.curr_start_bin, i.curr_start_bin+shift, i.curr_stop_bin-i.curr_start_bin+1);
	}
	else {
		// exclude frequency range
		shiftApply(a, i.x1, i.blk_len_fft, 1, 1+shift, i.curr_stop_bin);

		shiftApply(a, i.x1, i.blk_len_fft,
			i.curr_start_bin, i.curr_start_bin+shift, i.blk_len_fft/2-i.curr_start_bin);
	}	
}

//------------------------------------------------------------------------------------------
void BlkFxShiftAdd::process(void)
{
	shiftApply(*this, *mi);
}

//------------------------------------------------------------------------------------------
void BlkFxShiftReplace::process(void)
{
	shiftApply(*this, *mi);
}
