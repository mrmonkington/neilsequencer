#ifndef _DT_CPLX_
#define _DT_CPLX_
/*
 * Complex numbers class
 *
 * I wrote this class because std::complex didn't optimize very well (when I checked the
 * asm code it had rubbish that wouldn't optimize away)
 *
 * History
 * 15/3/03	Darrell Tam		created
 *
 * TODO: lots of things - I only implemented what I needed
 */
#include <complex>

template <class A> struct cplx
{
	A r, i; // real, imaginary
	inline cplx(std::complex<A> a) { r = a.real(); i = a.imag(); }
	inline cplx(A _r) { r = _r; i = 0.0f; }
	inline cplx(A _r, A _i) { r = _r; i = _i; }
	inline cplx(void) { }
	inline cplx operator () (A _r, A _i) { r = _r; i = _i; return *this; }
	inline A real(void) const { return r; }
	inline A imag(void) const { return i; }
	inline A& real(void) { return r; }
	inline A& imag(void) { return i; }
	inline operator std::complex<A> (void) { return std::complex<A>(r, i); }
};

template<class T> inline cplx<T> operator*(cplx<T> a, cplx<T> b) { cplx<T> r; r.r=a.r*b.r-a.i*b.i; r.i=a.r*b.i+a.i*b.r; return r; } 
template<class T> inline cplx<T> operator*(T a, cplx<T> b) { cplx<T> r; r.r=a*b.r; r.i=a*b.i; return r; } 
template<class T> inline cplx<T> operator*(cplx<T> a, T b) { cplx<T> r; r.r=a.r*b; r.i=a.i*b; return r; } 
template<class T> inline cplx<T> operator+(cplx<T> a, cplx<T> b) { cplx<T> r; r.r=a.r+b.r; r.i=a.i+b.i; return r; } 
template<class T> inline cplx<T> operator-(cplx<T> a, cplx<T> b) { cplx<T> r; r.r=a.r-b.r; r.i=a.i-b.i; return r; } 
template<class T> inline cplx<T> operator+(T a, cplx<T> b) { cplx<T> r; r.r=a+b.r; r.i=a.i; return r; } 
template<class T> inline cplx<T> operator+(cplx<T> a, T b) { cplx<T> r; r.r=a.r+b; r.i=a.i; return r; } 
template<class T> inline cplx<T> operator-(T a, cplx<T> b) { cplx<T> r; r.r=a-b.r; r.i=-b.i; return r; } 
template<class T> inline cplx<T> operator-(cplx<T> a, T b) { cplx<T> r; r.r=a.r-b; r.i=a.i; return r; } 
template<class T> inline cplx<T> operator-(cplx<T> a) { cplx<T> r; r.r=-a.r; r.i=-a.i; return r; }
template<class T> inline T norm(cplx<T> a) { return a.r*a.r+a.i*a.i; } 

typedef cplx<float> cplxf;
typedef cplx<double> cplxd;
const cplxf If(0.0f, 1.0f);

#endif
