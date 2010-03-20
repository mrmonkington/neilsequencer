/*
Copyright (C) 2007 Marcin Dabrowski

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
#define _USE_MATH_DEFINES
#include <cstdio>
#include <math.h>
#include <float.h>
#include <zzub/signature.h>
#include <zzub/plugin.h>
#include "ComplexFloat.h"
#include "FastCosSin.h"
#include "HilbertPair.h"
#include "Allpass2.h"
#include "LinLog.h"
#include "Bigyo_FrequencyShifter.h"

#pragma optimize ("awy", on) 


/////////////////////////////////////////////////////////////////////////////////////


/***

	zzub entry points

***/

const char *zzub_get_signature() { 
	return ZZUB_SIGNATURE; 
}

zzub::plugincollection *zzub_get_plugincollection() {
	return new freqshifterplugincollection();
}

/***

	machine_info

***/

const zzub::parameter *paraDirectionL = 0;
const zzub::parameter *paraDirectionR = 0;
const zzub::parameter *paraRate = 0;

machine_info::machine_info() {
	this->flags = zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
	this->name = "Bigyo FrequencyShifter";
	this->short_name = "FreqShift";
	this->author = "Marcin Dabrowski";
	this->uri = "@bigyo/frequency+shifter;1";

	paraRate = &add_global_parameter()
		.set_word()
		.set_state_flag()
		.set_name("Frequency")
		.set_description("Frequency")
		.set_value_min(0x0000)
		.set_value_max(0xFFFE)
		.set_value_none(0xFFFF)
		.set_value_default(0x0000);

	paraDirectionL = &add_global_parameter()
		.set_byte()
		.set_state_flag()
		.set_name("Left Direction")
		.set_description("Left Direction")
		.set_value_min(0x00)
		.set_value_max(0x02)
		.set_value_none(0xFF)
		.set_value_default(0x00);
                

	paraDirectionR = &add_global_parameter()
		.set_byte()
		.set_state_flag()
		.set_name("Right Direction")
		.set_description("Right Direction")
		.set_value_min(0x00)
		.set_value_max(0x02)
		.set_value_none(0xFF)
		.set_value_default(0x00);

	add_attribute()
		.set_name("Frequency non-linearity")
		.set_value_min(0)
		.set_value_max(10)
		.set_value_default(5);

	add_attribute()
		.set_name("Max. frequency (Hz)")
		.set_value_min(20)
		.set_value_max(20000)
		.set_value_default(5000);
}


#define miMACHINE_NAME "Bigyo FrequencyShifter"
#define miSHORT_NAME "FreqShift"
#define miMACHINE_AUTHOR "Marcin Dabrowski"
#define miVERSION "1.12"
#define miABOUTTXT1 "Marcin Dabrowski\n"  
#define miABOUTTXT2	"bigyo@wp.pl\n" 
#define miABOUTTXT miMACHINE_NAME" v"miVERSION"\n\nbuild: "__DATE__"\n\n"miABOUTTXT1""miABOUTTXT2

/////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////
freqshifter::freqshifter()
{	
	global_values = &gval;
	attributes = (int *)&aval;
}

freqshifter::~freqshifter() { }

void freqshifter::init(zzub::archive * const pi) {
}

void freqshifter::attributes_changed() {
	MaxRate = (float) aval.maxfreq ;
	slope = powf( 0.5f, (float)aval.nonlinearity + 1.0f );
	float freq = (  rate / (float) paraRate->value_max ) * MaxRate ; 
	float omega = freq2omega( (float) linlog(freq , 0.0 , MaxRate , slope) ) ;
	carrier.setOmega( omega ); 
}

void freqshifter::save(zzub::archive * const po) { }

void freqshifter::process_events() {
	if (gval.Rate != paraRate->value_none) {
		rate = gval.Rate;
		float freq = ( rate / (float) paraRate->value_max ) * MaxRate ; 
		float omega = freq2omega( (float) linlog(freq , 0.0 , MaxRate , slope) ) ;
		carrier.setOmega( omega ); 
	};

	if (gval.DirectionL != paraDirectionL->value_none) {
		dirL =  gval.DirectionL  ;
	};

	if (gval.DirectionR != paraDirectionR->value_none) {
		dirR =  gval.DirectionR  ;
	};
}

bool freqshifter::process_stereo(float** pin, float** pout, int numsamples, int mode) {
	if (mode==zzub::process_mode_write)
				return false;
	if (mode==zzub::process_mode_no_io)
				return false;
	if (mode==zzub::process_mode_read)                        // <thru>
				return true;
	float* psamples[2] = { pin[0], pin[1] };
	float* rsamples[2] = { pout[0], pout[1] };
	do
	{
		complex<float> c = carrier.process();

		if (dirL)
		{
			complex<float> l = hL.process(*psamples[0]++);
			*rsamples[0]++ = (dirL==1) ? c.re * l.re - c.im * l.im : c.re * l.re + c.im * l.im ;
		} else {
			psamples[0]++;
			rsamples[0]++;
		}
		if (dirR)
		{
			complex<float> r = hR.process(*psamples[1]++);
			*rsamples[1]++ = (dirR==1) ? c.re * r.re - c.im * r.im : c.re * r.re + c.im * r.im  ;
		} else {
			psamples[1]++;
			rsamples[1]++;
		}
	} while (--numsamples);

	return true;
}

void freqshifter::command(int const i) {
	switch (i)
	{
		case 0:
				_host->message(miABOUTTXT);
				break;
		default:
				break;
	}
}

char const *freqshifter::describe_value(int const param, int const value) {
	int n;
	float v,v1;
	static char txt[16];
	switch(param)
	{
		case 0: //  Rate
			v = (float) linlog(  MaxRate * value / paraRate->value_max, 0.0f , MaxRate , slope ) ;
			v1 = (float) linlog(  MaxRate * (value+1) / paraRate->value_max, 0.0f , MaxRate , slope ) ;
			n=(int) (1.0f-log10f( v1 - v )) ;
			if (n<0) n=0;
			sprintf	(txt,"%.*f Hz", n, 	v	);
			break;		

		case 1: // Dir L
		case 2: // Dir R
			switch(value)
			{	case 0: return("Off");
				case 1: return("Down");
				case 2: return("Up");
			}
		
		default:
			sprintf(txt,"%.2f %%",	(float) value / 65534.0f * 100.0f  );
	}
	return txt;
}

#pragma optimize ("", on) 

//DLL_EXPORTS

