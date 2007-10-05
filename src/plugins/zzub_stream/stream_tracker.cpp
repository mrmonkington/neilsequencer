#include "main.h"

/***

	this uses laurent de soras resampler code.
	resamples raw output from a zzub stream plugin to desired output frequency

***/

stream_tracker_machine_info stream_tracker_info;

/***

	machine_info

***/

namespace {

const zzub::parameter *paraNote = 0;
const zzub::parameter *paraWave = 0;
const zzub::parameter *paraVolume = 0;
const zzub::parameter *paraEffect1 = 0;
const zzub::parameter *paraEffect1Value = 0;
const zzub::parameter *paraEffect2 = 0;
const zzub::parameter *paraEffect2Value = 0;

}

stream_tracker_machine_info::stream_tracker_machine_info() {
	this->flags = zzub::plugin_flag_plays_waves | zzub::plugin_flag_has_audio_output;
	this->name = "zzub Stream Tracker (resampled)";
	this->short_name = "StreamTracker";
	this->author = "Andy Werk";
	this->uri = "@zzub.org/streamtracker;1";
	
	this->min_tracks = 1;
	this->max_tracks = stream_tracker_plugin::max_tracks;

	paraNote = &add_track_parameter()
		.set_note();

	paraWave = &add_track_parameter()
		.set_wavetable_index()
		.set_state_flag();

	paraVolume = &add_track_parameter()
		.set_byte()
		.set_name("Volume")
		.set_description("Volume")
		.set_value_min(0)
		.set_value_max(0xFE)
		.set_value_none(0xFF)
		.set_value_default(0x80);

	paraEffect1 = &add_track_parameter()
		.set_byte()
		.set_name("Effect")
		.set_description("Effect")
		.set_value_min(1)
		.set_value_max(0xFF)
		.set_value_none(0)
		.set_value_default(0);

	paraEffect1Value = &add_track_parameter()
		.set_byte()
		.set_name("Effect Value")
		.set_description("Effect Value")
		.set_value_min(0)
		.set_value_max(0xFE)
		.set_value_none(0xFF)
		.set_value_default(0xFF);

	paraEffect2 = &add_track_parameter()
		.set_byte()
		.set_name("Effect")
		.set_description("Effect")
		.set_value_min(1)
		.set_value_max(0xFF)
		.set_value_none(0)
		.set_value_default(0);

	paraEffect2Value = &add_track_parameter()
		.set_byte()
		.set_name("Effect Value")
		.set_description("Effect Value")
		.set_value_min(0)
		.set_value_max(0xFE)
		.set_value_none(0xFF)
		.set_value_default(0xFF);


}


/***

	stream_plugin

***/

stream_tracker_plugin::stream_tracker_plugin() {
	attributes = NULL;
	global_values = &gval;
	track_values = &tval;

}


void stream_tracker_plugin::init(zzub::archive* pi) {
	for (int i = 0; i<max_tracks; i++) {
		tracks[i].init(this, i);
	}
}

void stream_tracker_plugin::save(zzub::archive* po) {
}

void stream_tracker_plugin::set_track_count(int i) {
	num_tracks = i;
}

void stream_tracker_plugin::process_events() {

	for (int i = 0; i < num_tracks; i++) {
		tracks[i].process_events();
	}

}

bool stream_tracker_plugin::process_stereo(float **pin, float **pout, int numsamples, int mode) {
	
	bool result = false;

	for (int i = 0; i < num_tracks; i++) {
		result |= tracks[i].process_stereo(pin, pout, numsamples, mode);
	}

	return result;
}

void stream_tracker_plugin::command(int i) {
}

void stream_tracker_plugin::stop() {
	for (int i = 0; i < num_tracks; i++) {
		tracks[i].stop();
	}
}

void stream_tracker_plugin::destroy() {
	for (int i = 0; i < num_tracks; i++) {
		tracks[i].destroy();
	}
	delete this; 
}



/***

	streamtrack

***/


streamtrack::streamtrack() {
	stream = 0;
	note = zzub::note_value_none;
	wave = 0;
	last_wave = 0;
	tval = 0;
	_host = 0;
	_plugin = 0;
	last_level = -1;
	amp = 1.0f;
}

void streamtrack::init(stream_tracker_plugin* plugin, int index) {
	_host = plugin->_host;
	_plugin = plugin;
	tval = &plugin->tval[index];
}

void streamtrack::process_events() {

	bool trigger = false;

	if (tval->wave != paraWave->value_none) {
		wave = tval->wave;
	}

	const zzub::wave_info* current_wave = 0;
	const zzub::wave_level* current_level = 0;

	if (tval->note != paraNote->value_none) {
		if (note == zzub::note_value_off) {
			return ;
		}
		note = buzz_to_midi_note(tval->note);

		if (wave != 0) {
			// TODO: cache used streams?
			// and wtf is up with all the get_wave/get_nearest_level_/find_level etc
			current_wave = _host->get_wave(wave);
			if (!current_wave) return ;

			current_level = _host->get_nearest_wave_level(wave, tval->note);
			if (!current_level) return ;

			int levelIndex = find_level_index(current_wave, current_level);

			if (wave != last_wave || levelIndex != last_level) {
				if (stream) _host->stream_destroy(stream);
				stream = _host->stream_create(wave, levelIndex);

				if (resampler) delete resampler;
				resampler = new stream_resampler(stream);

			}

			resampler->note = note;
			last_wave = wave;
			last_level = levelIndex;
		}

		if (tval->volume != paraVolume->value_none)
			amp = (float)tval->volume / 0x80; else
			amp = 1.0f;
		trigger = true;
	}

	if (!stream) return ;

	if (!current_wave) {
		current_wave = _host->get_wave(last_wave);
		current_level = current_wave->get_level(last_level);
	}

	resampler->stream_base_note = buzz_to_midi_note(current_level->root_note);
	resampler->stream_sample_rate = current_level->samples_per_second;

	if (trigger) {
		resampler->set_stream_pos(0);
	}
}

void streamtrack::stop() {
	if (!stream) return ;
	stream->stop();
	resampler->playing = false;
}

bool streamtrack::process_stereo(float **pin, float **pout, int numsamples, int mode) {
	if (!stream) return false;

	float l[zzub::buffer_size], r[zzub::buffer_size];
	float* lr[] = { l, r };
	bool result = resampler->process_stereo(lr, numsamples);
	if (!result) return false;

	add_samples(pout[0], l, numsamples, amp);
	add_samples(pout[1], r, numsamples, amp);
	return true;
}

void streamtrack::destroy() {
	if (stream)
		_host->stream_destroy(stream);
	if (resampler)
		delete resampler;
	stream = 0;
	resampler = 0;
}
