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

#include <string>

#if defined(POSIX)
#include <dlfcn.h>
#endif

#if defined(__GNUC__)
// it seems these defines are also missing from the gnu headers
#endif

#include "common.h"
#include "tools.h"

char toLower(char c) { return tolower(c); }

char backslashToSlash(char c) { if (c=='\\') return '/'; return c; }

void AddM2SPan(float* output, float* input, int numSamples, float inAmp, float inPan) {
	float panR=1.0f, panL=1.0f;
	if (inPan<1) {
		panR=inPan;	// when inPan<1, fade out right
	}
	if (inPan>1) {
		panL=2-inPan;	// when inPan>1, fade out left
	}
	for (int i=0; i<numSamples; i++) {
		float L=input[i]*panL * inAmp;
		float R=input[i]*panR * inAmp;

		output[i*2]+=L;
		output[i*2+1]+=R;

	}
}

void AddS2SPan(float* output, float* input, int numSamples, float inAmp, float inPan) {
	float panR=1.0f, panL=1.0f;
	if (inPan<1) {
		panR=inPan;	// when inPan<1, fade out right
	}
	if (inPan>1) {
		panL=2-inPan;	// when inPan>1, fade out left
	}
	for (int i=0; i<numSamples; i++) {
		float L=input[i*2]*panL * inAmp;
		float R=input[i*2+1]*panR * inAmp;

		output[i*2]+=L;
		output[i*2+1]+=R;

	}
}

void AddS2SPanMC(float** output, float** input, int numSamples, float inAmp, float inPan) {
	if (!numSamples)
		return;
	float panR=1.0f, panL=1.0f;
	if (inPan<1) {
		panR=inPan;	// when inPan<1, fade out right
	}
	if (inPan>1) {
		panL=2-inPan;	// when inPan>1, fade out left
	}
	float *pI0 = input[0];
	float *pI1 = input[1];
	float *pO0 = output[0];
	float *pO1 = output[1];
	do
	{
		*pO0++ += *pI0++ * panL * inAmp;
		*pO1++ += *pI1++ * panR * inAmp;
	} while (--numSamples);
}

// buffer=output, machineBuffer=input, ch1=outch,ch2=inch (1 or 2)
void mixBuffers(float* buffer, float* machineBuffer, int ch1, int ch2, int numSamples, float inAmp, float inPan) {
	if (ch2==1) {
		if (ch1==1) {
			Add(buffer, machineBuffer, numSamples, inAmp);
		} else {
			AddM2SPan(buffer, machineBuffer, numSamples, inAmp, inPan);
		}
	} else {
		// there are two channels in the input:
		if (ch1==1) {
			// the inPan stuff here is for connections marked with L or R in buzz
			AddStereoToMono(buffer, machineBuffer, numSamples, inAmp, inPan<1?0:1);
		} else {
			AddS2SPan(buffer, machineBuffer, numSamples, inAmp, inPan);
		}
	}
}

void Amp(float *pout, int numsamples, float amp) {
	for (int i=0; i<numsamples; i++) {
		pout[i]*=amp;
	}
}


 inline bool my_isnan(double x)
 {
   return x != x;
 }

 // TODO: finn bevis på at vi trenger sjekk for både 0 og isnan
bool isBufferZero(float** buf, int numsamples) {
	// lr: it makes sense to run through this code twice
	// especially when there is a signal at the end,
	// with separate buffer checks we find this after
	// numsamples-1 steps, with interleaved buffers
	// we find this after (numsamples-1)*2 steps.
	//
	// lr: checking for an empty buffer is a stupid 
	// optimization idea anyway, since with growing
	// complexity of tracks there will be simply
	// no empty buffers ever (delays, reverbs),
	// and thus this checking code here runs on every
	// damn buffer for nothing.
	//
	// it will be more performant to take it out.
	
	for (int c=0; c < 2; ++c) {
		for (int i=0; i<numsamples; i++) {
	//		if (fabs(buf[i])!=0.0) {
			if (fabs(buf[c][i])>=0.00001 && !my_isnan(buf[c][i])) {
				return false;
			}
		}
	}
	return true;
}

float dB_to_linear(float val) {
	if (val == 0.0) return(1.0);
	return (float)(pow(10.0f, val / 20.0f));
}

double square(double v) {
		double sqmod=fmod(v, 2.0f*PI);
		return sqmod<PI?-1:1;
	}

double sawtooth(double v) {
	return (fmod(v, 2.0f*PI) / PI)-1;
}

double triangle(double v) {
	double sqmod=fmod(v, 2.0f*PI);

	if (sqmod<PI) {
		return sqmod/PI;
	} else
		return (PI-(sqmod-PI)) / PI;
}


#ifdef _USE_SEH
// the translator function
void __cdecl SEH_To_Cpp(unsigned int u, EXCEPTION_POINTERS *exp) {
  throw u;        // throw an exception of type int
}

#endif

void handleError(std::string errorTitle) {
#if defined(_WIN32)
	LPVOID lpMsgBuf;
	if (!FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL )) {
		// error during error handling
		return;
	}

	// Display the string.
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, errorTitle.c_str(), MB_OK | MB_ICONINFORMATION);

	// Free the buffer.
	LocalFree(lpMsgBuf);
#else
	printf("%s: There was an error", errorTitle.c_str());
#endif

}



void CopyStereoToMono(float *pout, float *pin, int numsamples, float amp)
{
	do
	{
		*pout++ = (pin[0] + pin[1]) * amp;
		pin += 2;
	} while(--numsamples);
}


void AddStereoToMono(float *pout, float *pin, int numsamples, float amp)
{
	do
	{
		*pout++ += (pin[0] + pin[1]) * amp;
		pin += 2;
	} while(--numsamples);
}

void AddStereoToMono(float *pout, float *pin, int numsamples, float amp, int ch)
{
	do
	{
		*pout++ += pin[ch] * amp;
		pin += 2;
	} while(--numsamples);
}

void CopyM2S(float *pout, float *pin, int numsamples, float amp)
{
	do
	{
		double s = *pin++ * amp;
		pout[0] = (float)s;
		pout[1] = (float)s;
		pout += 2;
	} while(--numsamples);

}

void Add(float *pout, float *pin, int numsamples, float amp)
{
	do
	{
		*pout++ += *pin++ * amp;
	} while(--numsamples);
}

size_t sizeFromWaveFormat(int waveFormat) {
	switch (waveFormat) {
		case zzub::wave_buffer_type_si16:
			return 2;
		case zzub::wave_buffer_type_si24:
			return 3;
		case zzub::wave_buffer_type_f32:
		case zzub::wave_buffer_type_si32:
			return 4;
		default:
			return -1;
	}
}

// disse trenger vi for lavnivå redigering på flere typer bitformater, waveFormat er buzz-style
// det er kanskje mulig å oppgradere copy-metodene med en interleave på hver buffer for å gjøre konvertering mellom stereo/mono integrert
void CopyMonoToStereoEx(void* srcbuf, void* targetbuf, size_t numSamples, int waveFormat) {

	int sampleSize=sizeFromWaveFormat(waveFormat);
	char* tbl=(char*)targetbuf;
	char* tbr=(char*)targetbuf;
	tbr+=sampleSize;
	char* sb=(char*)srcbuf;

	int temp;

	for (size_t i=0; i<numSamples; i++) {
		switch (waveFormat) {
			case zzub::wave_buffer_type_si16:
				*((short*)tbr)=*((short*)tbl)=*(short*)sb;
				break;
			case zzub::wave_buffer_type_si24:
				temp=(*(int*)sb) >> 8;
				*((int*)tbr)=*((int*)tbl)=temp;
				break;
			case zzub::wave_buffer_type_f32:
			case zzub::wave_buffer_type_si32:
				temp=*(int*)sb;
				*((int*)tbr)=*((int*)tbl)=temp;
				break;
		}
		tbl+=sampleSize*2;
		tbr+=sampleSize*2;
		sb+=sampleSize;
	}
}

void CopyStereoToMonoEx(void* srcbuf, void* targetbuf, size_t numSamples, int waveFormat) {
}


// from 16 bit conversion
void Copy16To24(void* srcbuf, void* targetbuf, size_t numSamples) {
}

void Copy16ToS32(void* srcbuf, void* targetbuf, size_t numSamples) {
}

void Copy16ToF32(void* srcbuf, void* targetbuf, size_t numSamples) {
}


// from 32 bit floating point conversion
void CopyF32To16(void* srcbuf, void* targetbuf, size_t numSamples) {
}

void CopyF32To24(void* srcbuf, void* targetbuf, size_t numSamples) {
}

void CopyF32ToS32(void* srcbuf, void* targetbuf, size_t numSamples) {
}


// from 32 bit integer conversion
void CopyS32To16(void* srcbuf, void* targetbuf, size_t numSamples) {
}

void CopyS32To24(void* srcbuf, void* targetbuf, size_t numSamples) {
}

void CopyS32ToF32(void* srcbuf, void* targetbuf, size_t numSamples) {
}


// from 24 bit integer conversion
void Copy24To16(void* srcbuf, void* targetbuf, size_t numSamples) {
}

void Copy24ToF32(void* srcbuf, void* targetbuf, size_t numSamples) {
}

void Copy24ToS32(void* srcbuf, void* targetbuf, size_t numSamples) {
}



// found the trims in one of the comments at http://www.codeproject.com/vcpp/stl/stdstringtrim.asp

std::string& trimleft( std::string& s )
{
   std::string::iterator it;

   for( it = s.begin(); it != s.end(); it++ )
      if( !isspace((unsigned char) *it ) )
         break;

   s.erase( s.begin(), it );
   return s;
}

std::string& trimright( std::string& s )
{
   std::string::difference_type dt;
   std::string::reverse_iterator it;

   for( it = s.rbegin(); it != s.rend(); it++ )
      if( !isspace((unsigned char) *it ) )
         break;

   dt = s.rend() - it;

   s.erase( s.begin() + dt, s.end() );
   return s;
}

std::string& trim( std::string& s )
{
   trimleft( s );
   trimright( s );
   return s;
}

std::string trim( const std::string& s )
{
   std::string t = s;
   return trim( t );
}


int transposeNote(int v, int delta) {
	// 1) convert to "12-base"
	// 2) transpose
	// 3) convert back to "16-base"
	int note=(v&0xF)-1;
	int oct=(v&0xF0) >> 4;

	int twelve=note+12*oct;

	twelve+=delta;

	note=(twelve%12)+1;
	oct=twelve/12;
	return (note) + (oct<<4);
}


int getNoValue(const zzub::parameter* para) {
	switch (para->type) {
		case zzub::parameter_type_switch:
			return zzub::switch_value_none;
		case zzub::parameter_type_note:
			return zzub::note_value_none;
		default:
			return para->value_none;
	}
}


bool validateParameter(int value, const zzub::parameter* p) {
	if (p->type==zzub::parameter_type_switch) return true;
	// TODO: validate note
	if (p->type==zzub::parameter_type_note) return true;

	return (value==getNoValue(p) || (value>=p->value_min && value<=p->value_max) );

}

// cross platform library loading

xp_modulehandle xp_dlopen(const char* path)
{
#if defined(_WIN32)
	return LoadLibrary(path);
#elif defined(POSIX)
	return dlopen(path, RTLD_LAZY | RTLD_LOCAL);
#endif
}

void* xp_dlsym(xp_modulehandle handle, const char* symbol)
{
#if defined(_WIN32)
	return (void*)GetProcAddress((HINSTANCE)handle, symbol);
#elif defined(POSIX)
	return dlsym(handle, symbol);
#endif
}
	
void xp_dlclose(xp_modulehandle handle)
{
#if defined(_WIN32)
	FreeLibrary((HINSTANCE)handle);
#elif defined(POSIX)
	dlclose(handle);
#endif
}

const char *xp_dlerror() {
#if defined(_WIN32)
	LPVOID* lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL );
	return (LPCTSTR)lpMsgBuf;
#elif defined(POSIX)
	return dlerror();
#endif
}
