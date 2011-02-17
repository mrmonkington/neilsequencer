#pragma once

namespace zzub {

struct audio_connection_parameter_volume : parameter {
	audio_connection_parameter_volume();
};

struct audio_connection_parameter_panning : parameter {
	audio_connection_parameter_panning();
};

struct audio_connection_values {
	unsigned short amp, pan;
};

struct audio_connection : connection {
	static audio_connection_parameter_volume para_volume;
	static audio_connection_parameter_panning para_panning;

	audio_connection_values values;
	audio_connection_values cvalues;
	
	float lastFrameMaxSampleL, lastFrameMaxSampleR;
	
	audio_connection();
	virtual void process_events(zzub::song& player, const zzub::connection_descriptor& conn);
	virtual bool work(zzub::song& player, const zzub::connection_descriptor& conn, int sample_count);
};

struct event_connection : connection {
	
	std::vector<event_connection_binding> bindings;

	event_connection();
	virtual void process_events(zzub::song& player, const zzub::connection_descriptor& conn);
	virtual bool work(zzub::song& player, const zzub::connection_descriptor& conn, int sample_count);
	int convert(int value, const zzub::parameter *oldparam, const zzub::parameter *newparam);
	const zzub::parameter *getParam(struct metaplugin *mp, int group, int index);
};

struct midi_connection : connection {

	int device;
	std::string device_name;

	midi_connection();
	int get_midi_device(zzub::song& player, int plugin, std::string name);
	virtual void process_events(zzub::song& player, const zzub::connection_descriptor& conn);
	virtual bool work(zzub::song& player, const zzub::connection_descriptor& conn, int sample_count);
};

} // namespace zzub
