#include "main.h"

/***

	this uses laurent de soras resampler code.
	resamples raw output from a zzub stream plugin to desired output frequency

***/

stream_player_machine_info stream_player_info;

/***

	machine_info

***/

namespace {

const zzub::parameter *paraNote = 0;
const zzub::parameter *paraOffsetHigh = 0;
const zzub::parameter *paraOffsetLow = 0;
const zzub::parameter *paraLengthHigh = 0;
const zzub::parameter *paraLengthLow = 0;

}

stream_player_machine_info::stream_player_machine_info() {
	this->flags = zzub::plugin_flag_plays_waves | zzub::plugin_flag_has_audio_output;
	this->name = "zzub Stream Player (resampled)";
	this->short_name = "StreamPlayer";
	this->author = "Andy Werk";
	this->uri = "@zzub.org/streamplayer;1";
	
	paraNote = &add_global_parameter()
		.set_note();

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

}

/***

	stream_plugin

***/

stream_player_plugin::stream_player_plugin() {
	attributes = NULL;
	global_values = &gval;
	track_values = 0;
	stream = 0;
	resampler = 0;
}


void stream_player_plugin::init(zzub::archive* pi) {
	if (!pi) return ;
	zzub::instream* inf = pi->get_instream("");
	std::string dataUrl, pluginUri;
	inf->read(dataUrl);
	inf->read(pluginUri);
	if (stream) _host->stream_destroy(stream);
	if (resampler) delete resampler;

	stream = _host->stream_create(pluginUri.c_str(), dataUrl.c_str());
	resampler = new stream_resampler(stream);

	// TODO: we need more details about the stream, i.e at least samplerate and/or base note
	// wavelevel* level = _host->get_stream_level(stream);	<- under the hood, the stream could be saved and deserialized into a wavelevel-struct
}

void stream_player_plugin::save(zzub::archive* po) {
	if (stream)
		stream->save(po);
}

void stream_player_plugin::process_events() {

	if (!stream) return ;

	bool trigger = false;
	unsigned int ofs = 0, len = 0xFFFFFFFF;

	const zzub::wave_info* current_wave = 0;
	const zzub::wave_level* current_level = 0;

	if (gval.note != paraNote->value_none) {
		if (gval.note == zzub::note_value_off) {
			resampler->playing = false;
			return ;
		}
		resampler->note = buzz_to_midi_note(gval.note);
		trigger = true;
	}

	if (gval.offset != 0xFFFFFFFF) {
		ofs = get_offset();
		trigger = true;
	}

	if (gval.length != 0xFFFFFFFF) {
		len = get_length();
	}

	if (trigger)
		resampler->set_stream_pos(ofs);//, len);


}

bool stream_player_plugin::process_stereo(float **pin, float **pout, int numsamples, int mode) {
	return resampler?resampler->process_stereo(pout, numsamples):false;
}

void stream_player_plugin::command(int i) {
}

void stream_player_plugin::stop() {
	if (!stream) return ;
	stream->stop();
	resampler->playing = false;
}

void stream_player_plugin::destroy() {
	if (stream) 
		_host->stream_destroy(stream);

	if (resampler) 
		delete resampler;

	stream = 0;
	resampler = 0;
	delete this; 
}

