/*

TODO:

things we can do with a semi-native wave-player (ui-wise):

	- preview waves in wave table
	- preview waves on disk
	- set preview volume
	- play notes in wavetable
	- play selected range in wave editor
	- play back frozen machine audio
	- play mp3s and oggs and streams in songs (use new zzub api)
	- preview mp3s, oggs in wavetable and filebrowser
	- sync long waves with song position
	- play ranges of long waves synced with song position

*/

#if defined(_WIN32)
#include <windows.h>
#endif

#include <zzub/signature.h>

#include "main.h"

int buzz_to_midi_note(int note) {
	return 12 * (note >> 4) + (note & 0xf) - 1;
}

const char* get_open_filename(const char* fileName, const char* filter) {

	static char szFile[260];       // buffer for file name
	strcpy(szFile, fileName);

#if defined(_WIN32)
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = filter;
	ofn.lpstrDefExt="";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_NOCHANGEDIR |OFN_OVERWRITEPROMPT|OFN_EXTENSIONDIFFERENT|OFN_NOREADONLYRETURN;

	if (::GetOpenFileName(&ofn)==TRUE) {
		return szFile;
	}
#else
	printf("get_open_filename not implemented!");
#endif
	return 0;
}

float lognote(int freq) {
	return (logf(freq) - logf(440.0)) / logf(2.0) + 4.0;
}

void add_samples(float *pout, float *pin, int numsamples, float amp) {
	do {
		*pout++ += *pin++ * amp;
	} while(--numsamples);
}

/***

	zzub entry points

***/

const char *zzub_get_signature() { 
	return ZZUB_SIGNATURE; 
}

zzub::plugincollection *zzub_get_plugincollection() {
	return new streamplugincollection();
}

/***

	machine_info

***/

namespace {

const zzub::parameter *paraNote = 0;
const zzub::parameter *paraOffsetHigh = 0;
const zzub::parameter *paraOffsetLow = 0;
const zzub::parameter *paraLengthHigh = 0;
const zzub::parameter *paraLengthLow = 0;
	
const zzub::attribute *attrOffsetFromPlayPos = 0;

}

stream_machine_info::stream_machine_info() {
	this->flags = zzub::plugin_flag_plays_waves 
	| zzub::plugin_flag_has_audio_output
	| zzub::plugin_flag_stream;
	
	paraNote = &add_global_parameter()
		.set_note();

	// what about a 32bit parameter instead
	paraOffsetLow = &add_global_parameter()
		.set_word()
		.set_name("Offset Low")
		.set_description("32 bit Offset (Lower 16 bits)")
		.set_value_min(0)
		.set_value_max(0xFFFE)
		.set_value_none(0xFFFF)
		.set_value_default(0xFFFF);

	paraOffsetHigh = &add_global_parameter()
		.set_word()
		.set_name("Offset High")
		.set_description("32 bit Offset (Higher 16 bits)")
		.set_value_min(0)
		.set_value_max(0xFFFE)
		.set_value_none(0xFFFF)
		.set_value_default(0xFFFF);

	paraLengthLow = &add_global_parameter()
		.set_word()
		.set_name("Length Low")
		.set_description("32 bit Length (Lower 16 bits)")
		.set_value_min(0)
		.set_value_max(0xFFFE)
		.set_value_none(0xFFFF)
		.set_value_default(0xFFFF);

	paraLengthHigh = &add_global_parameter()
		.set_word()
		.set_name("Length High")
		.set_description("32 bit Length (Higher 16 bits)")
		.set_value_min(0)
		.set_value_max(0xFFFE)
		.set_value_none(0xFFFF)
		.set_value_default(0xFFFF);
		
	attrOffsetFromPlayPos = &add_attribute()
		.set_name("Offset from Song")
		.set_value_min(0)
		.set_value_max(1)
		.set_value_default(0);
}


/***

	stream_plugin

***/

stream_plugin::stream_plugin() {
	attributes = (int*)&aval;
	global_values = &gval;
	track_values = 0;
}
