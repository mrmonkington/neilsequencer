#if defined(_WIN32)
#include <windows.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <zzub/signature.h>
#include <zzub/plugin.h>
#include "module.h"
#include "tracker.h"

int buzz_to_midi_note(int note) {
	return 12 * (note >> 4) + (note & 0xf) - 1;
}

int midi_to_buzz_note(int value) {
	return ((value / 12) << 4) + (value % 12) + 1;
}

float note_to_frequency(float note) {
	// A-4 = 440Hz = Midi note 57
	return (float)440.0 * powf(2, (note - (float)57.0f) / (float)12.0f);
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

/***

	zzub entry points

***/

const char *zzub_get_signature() { 
	return ZZUB_SIGNATURE; 
}

zzub::plugincollection *zzub_get_plugincollection() {
	return new moduleplugincollection();
}

/***

	machine_info

***/

const zzub::parameter *paraNote = 0;
const zzub::parameter *paraWave = 0;
const zzub::parameter *paraVolume = 0;
const zzub::parameter *paraEffect = 0;
const zzub::parameter *paraEffectValue = 0;

const int module_plugin_value_none = 0xFF;

module_plugin_info::module_plugin_info() {
	this->name = "zzub Module";
	this->short_name = "Module";
	this->author = "Andy Werk <calvin@countzero.no>";
	this->uri = "@zzub.org/module;1";
	this->flags = 0;//zzub::plugin_flag_plays_waves;
	this->min_tracks = 1;
	this->max_tracks = module_plugin::max_tracks;
	this->commands = "Load module...";
	
	paraNote = &add_track_parameter()
		.set_note();

	paraWave = &add_track_parameter()
		.set_wavetable_index();

	paraVolume = &add_track_parameter()
		.set_byte()
		.set_name("Volume")
		.set_description("Volume (0=0%, 80=100%, FE=199%)")
		.set_value_min(0)
		.set_value_max(0xFE)
		.set_value_none(module_plugin_value_none)
		.set_value_default(module_plugin_value_none);

	paraEffect = &add_track_parameter()
		.set_byte()
		.set_name("Effect")
		.set_description("Effect")
		.set_value_min(0)
		.set_value_max(0xFE)
		.set_value_none(module_plugin_value_none)
		.set_value_default(module_plugin_value_none);

	paraEffectValue = &add_track_parameter()
		.set_byte()
		.set_name("Effect Value")
		.set_description("Effect Value")
		.set_value_min(0)
		.set_value_max(0xFE)
		.set_value_none(module_plugin_value_none)
		.set_value_default(module_plugin_value_none);

}

/***

	module_plugin

***/

module_plugin::module_plugin() {
	attributes = NULL;
	global_values = NULL;
	track_values = &tval;
}

module_plugin::~module_plugin() {
}

void module_plugin::init(zzub::archive * const pi) {
}

void module_plugin::set_track_count(int i) { 
}

void module_plugin::process_events() {
}

bool module_plugin::process_stereo(float **pin, float **pout, int numsamples, int mode) {
	if (mode == zzub::process_mode_read) return false;
	if (mode == zzub::process_mode_no_io) return false;
	return false;
}

const char * module_plugin::describe_value(int param, int value) {
	return 0;
}

void module_plugin::command(int i) {
	switch (i) {
		case 0: {
			const char* open_file = get_open_filename("", "Modules (*.mod, *.xm, *.s3m, *.it)\0*.mod;*.s3m;*.it;*.xm\0All files\0*.*\0\0");
			if (!open_file) return ;
			importModule(open_file);
			break;
		}
	}
}

// TODO: offset all sample values by X, so we can load more than one module at the same time
void module_plugin::importModule(std::string fileName) {
	using namespace modfile;
	module* modf = module::create(fileName);
	for (int i = 0; i<modf->instruments.size(); i++) {
		for (int j = 0; j<modf->instruments[i].samples.size(); j++) {
			sample_info& si = modf->instruments[i].samples[j];
			if (si.samples > 0) {
				_host->allocate_wave(i+1, j, si.samples, zzub::wave_buffer_type_si16, si.is_stereo, si.name.c_str());
				const zzub::wave_info* wave = _host->get_wave(i+1);
				void* buffer = wave->get_sample_ptr(j);
				// TODO: convert buffer if samples != si16
				modf->sample_read(i, j, buffer, si.get_bytes());
			}
		}
	}

	int numChannels = 4;
	if (modf->patterns.size() > 0)
		numChannels = modf->patterns[0].tracks;
	
	_host->set_track_count(numChannels);

	for (int i = 0; i<modf->patterns.size(); i++) {
		pattern_type& pt = modf->patterns[i];
		zzub::pattern* pattern = _host->create_pattern("test", pt.rows);
		for (int row = 0; row < pt.rows; row++) {
			for (int chn = 0; chn<numChannels; chn++) {
				int note = pt.note(chn, row);
				int sample = pt.sample(chn, row);
				int volume = pt.volume(chn, row);
				int effect = pt.effect(chn, row);
				int effect_value = pt.effect_value(chn, row);

				if (note != 0) {
					note = midi_to_buzz_note(note);
				}

				_host->set_pattern_data(pattern, row, 2, chn, 0, note);
				_host->set_pattern_data(pattern, row, 2, chn, 1, sample);
				_host->set_pattern_data(pattern, row, 2, chn, 2, volume);
				_host->set_pattern_data(pattern, row, 2, chn, 3, effect);
				_host->set_pattern_data(pattern, row, 2, chn, 4, effect_value);

			}
		}
	}
	// import patterns and waves
	modf->close();
}
