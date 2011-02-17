/******************************************************************************
Wrap process

  classes to perform an operation on a FIFO - if the "process()" operation
  passes the end of the FIFO then it is called again for the remainder
  of data at the start of FIFO.

History
	Date       Version    Programmer         Comments
	16/2/03    1.0        Darrell Tam		Created
******************************************************************************/

#ifndef _WRAP_PROCESS_H_
#define _WRAP_PROCESS_H_
#include <vector>
#include "misc_stuff.h"

//------------------------------------------------------------------------------------------
template <class P> inline void wrapProcess(
		P& p,			// class containing "process(float*, int n)"
		std::vector<float>& x,	// vector to process (e.g. x is of type vector<float>)
		long i,			// index into vector "x" to start at
		long n			// number of elements to process
	)
// process vector "x", wrapping at the end of "x" back to the start 
// implemented as a template instead of interface class to help optimizer
{
	long n0 = x.size()-i;
	if(n >= n0) {
		p.process(&x[i], n0);
		n -= n0;
		i = 0;
	}
	if(n > 0) p.process(&x[i], n);
}

//------------------------------------------------------------------------------------------
struct PFadeOut 
// apply fade out to existing data
{
	float fade, fade_s;

	inline PFadeOut(long n) {
		fade = 1.0f;
		fade_s = -1.0f/n;
	}
	
	inline void process(float *x, long n)
	{
		for(long i = 0; i < n; i++) {
			x[i] *= fade;
			fade += fade_s;
		}
	}
};

//------------------------------------------------------------------------------------------
struct PZero 
// zero data
{
	inline void process(float *x, long n)
	{
		memset(x, 0, n*sizeof(float));
	}
};
//------------------------------------------------------------------------------------------
struct PXFade 
// cross fade between "x" (existing FIFO vector) and "y" (new vector vector)
{
	float x_fade, x_fstep, *y, y_fade, y_fstep;

	inline PXFade(float *_y, float y_scale, long n)
	{
		y = _y;
		x_fade = 1.0f;
		x_fstep = -1.0f/(float)n;
		y_fade = 0;
		y_fstep = y_scale/(float)n;	
	}

	inline void process(float *x, long n)
	{
		float *xe = x+n;
		for(; x != xe; x++) {
			*x = limit_range((*x)*x_fade+(*y++)*y_fade, -32768, 32767);
			x_fade += x_fstep;
			y_fade += y_fstep;
		}
	}
};

//------------------------------------------------------------------------------------------
struct PScaleCopy 
// copy and scale src into dst (existing FIFO vector)
{
	float *src, src_scale;

	inline PScaleCopy(float *_src, float _src_scale)
	{
		src = _src;
		src_scale = _src_scale;
	}

	inline void process(float *dst, long n)
	{
		float *dst_e = dst+n;
		for(; dst != dst_e; dst++)
			*dst = limit_range((*src++)*src_scale, -32768, 32767);
	}
};

//------------------------------------------------------------------------------------------
struct PXFade2 
// cross fade between "x" (existing FIFO vector) and "y"+"z" (new vector vector)
{
	float x_fade, x_fstep, *y, y_fade, y_fstep, *z, z_fade, z_fstep;

	inline PXFade2(float *_y, float y_scale, float *_z, float z_scale, long n)
	{
		x_fade = 1.0f;
		x_fstep = -1.0f/(float)n;
		y = _y;
		y_fade = 0;
		y_fstep = y_scale/(float)n;	
		z = _z;
		z_fade = 0;
		z_fstep = z_scale/(float)n;	
	}

	inline void process(float *x, long n)
	{
		float *xe = x+n;
		for(; x != xe; x++) {
			*x = limit_range((*x)*x_fade+(*y++)*y_fade+(*z++)*z_fade, -32768, 32767);
			x_fade += x_fstep;
			y_fade += y_fstep;
			z_fade += z_fstep;
		}
	}
};

//------------------------------------------------------------------------------------------
struct PScaleCopy2 
// copy and scale src1+src2 into dst (existing FIFO vector)
{
	float *src1, src1_scale, *src2, src2_scale;

	inline PScaleCopy2(float *_src1, float _src1_scale, float *_src2, float _src2_scale)
	{
		src1 = _src1;
		src1_scale = _src1_scale;
		src2 = _src2;
		src2_scale = _src2_scale;
	}

	inline void process(float *dst, long n)
	{
		float *dst_e = dst+n;
		for(; dst != dst_e; dst++)
			*dst = limit_range((*src1++)*src1_scale+(*src2++)*src2_scale, -32768, 32767);
	}
};

//------------------------------------------------------------------------------------------
struct PCopyOut 
// copy existing FIFO data to another buffer
{
	float *dst;
	long dst_i;
	inline PCopyOut(float *_dst)
	{
		dst_i = 0;
		dst = _dst;
	}

	inline void process(float *src, long n)
	{
		memcpy(&dst[dst_i], src, n*sizeof(float));
		dst_i += n;
	}
};


#endif
