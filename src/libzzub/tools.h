/*
Copyright (C) 2003-2007 Anders Ervik <calvin@countzero.no>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

float linear_to_dB(float val);
float dB_to_linear(float val);
void handleError(std::string errorTitle);

// wave tools
double square(double v);
double sawtooth(double v);
double triangle(double v);

// buffer tools
void AddS2SPanMC(float** output, float** input, int numSamples, float inAmp, float inPan);
void CopyM2S(float *pout, float *pin, int numsamples, float amp);
void Add(float *pout, float *pin, int numsamples, float amp);
void AddStereoToMono(float *pout, float *pin, int numsamples, float amp);
void AddStereoToMono(float *pout, float *pin, int numsamples, float amp, int ch);
void CopyStereoToMono(float *pout, float *pin, int numsamples, float amp);
void Amp(float *pout, int numsamples, float amp);

// disse trenger vi for lavnivå redigering på flere typer bitformater, waveFormat er buzz-style
// det er kanskje mulig å oppgradere copy-metodene med en interleave på hver buffer for å gjøre konvertering mellom stereo/mono integrert
void CopyMonoToStereoEx(void* srcbuf, void* targetbuf, size_t numSamples, int waveFormat);
void CopyStereoToMonoEx(void* srcbuf, void* targetbuf, size_t numSamples, int waveFormat);

// from 16 bit conversion
void Copy16To24(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);
void Copy16ToS32(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);
void Copy16ToF32(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);

// from 32 bit floating point conversion
void CopyF32To16(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);
void CopyF32To24(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);
void CopyF32ToS32(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);

// from 32 bit integer conversion
void CopyS32To16(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);
void CopyS32To24(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);
void CopyS32ToF32(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);

// from 24 bit integer conversion
void Copy24To16(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);
void Copy24ToF32(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);
void Copy24ToS32(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);

// trivial conversions
void Copy16(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);
void Copy24(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);
void CopyS32(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);
void CopyF32(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);

struct S24 {
	union {
		struct {
			char c3[3];
		};
		struct {
			short s;
			char c;
		};
	};
};

// auto select based on waveformat
void CopySamples(void *srcbuf, void *targetbuf, size_t numSamples, int srcWaveFormat, int dstWaveFormat, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);

inline void ConvertSample(const short &src, short &dst) { dst = src; }
inline void ConvertSample(const short &src, S24 &dst) { dst.s = src; dst.c = 0; }
inline void ConvertSample(const short &src, int &dst) { dst = (int)src * (1<<16); }
inline void ConvertSample(const short &src, float &dst) { dst = (float)src / 32767.0f; }

inline void ConvertSample(const S24 &src, short &dst) { dst = src.s; }
inline void ConvertSample(const S24 &src, S24 &dst) { dst = src; }
inline void ConvertSample(const S24 &src, int &dst) { assert(0); }
inline void ConvertSample(const S24 &src, float &dst) { assert(0); }

inline void ConvertSample(const int &src, short &dst) { dst = (short)(src / (1<<16)); }
inline void ConvertSample(const int &src, S24 &dst) { assert(0); }
inline void ConvertSample(const int &src, int &dst) { dst = src; }
inline void ConvertSample(const int &src, float &dst) { dst = (float)src / 2147483648.0f; }

inline void ConvertSample(const float &src, short &dst) { dst = (short)(src * 32767.0f); }
inline void ConvertSample(const float &src, S24 &dst) { assert(0); }
inline void ConvertSample(const float &src, int &dst) { dst = (int)(src * 2147483648.0f); }
inline void ConvertSample(const float &src, float &dst) { dst = src; }

template <typename srctype, typename dsttype>
inline void CopySamplesT(const srctype *src, dsttype *dst, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0) {
	src += srcoffset;
	dst += dstoffset;
	while (numSamples--) {
		ConvertSample(*src, *dst);
		src += srcstep;
		dst += dststep;
	}
}

// this is a wrapper for quick GCC support, since GCC's transform() doesnt accept regular tolower
char backslashToSlash(char c);

// zzub tools
int transposeNote(int v, int delta);
int getNoValue(const zzub::parameter* para);
bool validateParameter(int value, const zzub::parameter* p);

// string tools
std::string& trim( std::string& s );
std::string trim( const std::string& s );

// cross platform functions
#if defined(_WIN32)
	typedef HMODULE xp_modulehandle;
#elif defined(POSIX)
	typedef void *xp_modulehandle;
#endif
xp_modulehandle xp_dlopen(const char* path);
void* xp_dlsym(xp_modulehandle handle, const char* symbol);
void xp_dlclose(xp_modulehandle handle);
const char *xp_dlerror();
