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

#define PI 3.14

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
void Copy16To24(void* srcbuf, void* targetbuf, size_t numSamples);
void Copy16ToS32(void* srcbuf, void* targetbuf, size_t numSamples);
void Copy16ToF32(void* srcbuf, void* targetbuf, size_t numSamples);

// from 32 bit floating point conversion
void CopyF32To16(void* srcbuf, void* targetbuf, size_t numSamples);
void CopyF32To24(void* srcbuf, void* targetbuf, size_t numSamples);
void CopyF32ToS32(void* srcbuf, void* targetbuf, size_t numSamples);

// from 32 bit integer conversion
void CopyS32To16(void* srcbuf, void* targetbuf, size_t numSamples);
void CopyS32To24(void* srcbuf, void* targetbuf, size_t numSamples);
void CopyS32ToF32(void* srcbuf, void* targetbuf, size_t numSamples);

// from 24 bit integer conversion
void Copy24To16(void* srcbuf, void* targetbuf, size_t numSamples);
void Copy24ToF32(void* srcbuf, void* targetbuf, size_t numSamples);
void Copy24ToS32(void* srcbuf, void* targetbuf, size_t numSamples);

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
