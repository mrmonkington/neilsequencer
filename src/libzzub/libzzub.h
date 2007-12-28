/*
Copyright (C) 2003-2007 Anders Ervik <calvin@countzero.no>
Copyright (C) 2006-2007 Leonard Ritter

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

/*
	This is a pure C library header for libzzub. If you wish
	to use classes directly, please include the according
	class headers.
*/

#if defined(ZZUB_DOC_CPP_API)
/*! \mainpage libzzub C++ documentation

	\section intro Introduction

	The <a href="annotated.html">class list</a> shows an overview of the C++ classes in libzzub.
	
	If you want to use libzzub in your own project, please refer to the more stable 
	<a href="">C API Reference</a> (../../api/html/index.html).

 */
#elif defined(ZZUB_DOC_C_API)
/*! \mainpage libzzub C documentation

	\section intro Introduction
	
	See the documentation on <a href="libzzub_8h.html">libzzub.h</a> for an overview 
	of all C methods exposed by libzzub.

	If you want to get into the finer details of libzzub, the 
	<a href="">C++ class reference</a> gives you more
	information. (../../api-cpp/html/index.html)
 */
#endif

#if !defined(__LIBZZUB_H)
#define __LIBZZUB_H

#include "zzub/types.h"

#if defined(__cplusplus)
extern "C" {
#endif

/** \brief Get number of detected audio devices */
int zzub_audiodriver_get_count(zzub_player_t *player);

/** \brief Get name of specified audio device */
int zzub_audiodriver_get_name(zzub_player_t *player, int index, char* name, int maxLen);

/** \brief Create specified audio device */
int zzub_audiodriver_create(zzub_player_t *player, int input_index, int output_index);
	
/** \brief Enable or disable current audio driver */
void zzub_audiodriver_enable(zzub_player_t *player, int state);

/** \brief Disassociate audio driver and player */
void zzub_audiodriver_destroy(zzub_player_t *player);

/** \brief Set audio driver sample rate */
void zzub_audiodriver_set_samplerate(zzub_player_t *player, unsigned int samplerate);

/** \brief Retreive audio driver sample rate */
unsigned int zzub_audiodriver_get_samplerate(zzub_player_t *player);

void zzub_audiodriver_set_buffersize(zzub_player_t *player, unsigned int buffersize);
unsigned int zzub_audiodriver_get_buffersize(zzub_player_t *player);
double zzub_audiodriver_get_cpu_load(zzub_player_t *player);
int zzub_audiodriver_is_output(zzub_player_t *player, int index);
int zzub_audiodriver_is_input(zzub_player_t *player, int index);
	
/* midi driver */
	
int zzub_mididriver_get_count(zzub_player_t *player);
const char *zzub_mididriver_get_name(zzub_player_t *player, int index);
int zzub_mididriver_is_input(zzub_player_t *player, int index);
int zzub_mididriver_is_output(zzub_player_t *player, int index);
int zzub_mididriver_open(zzub_player_t *player, int index);
int zzub_mididriver_close_all(zzub_player_t *player);

/* plugin collection */

zzub_plugincollection_t *zzub_player_get_plugincollection_by_uri(zzub_player_t *player, const char *uri);
void zzub_plugincollection_configure(zzub_plugincollection_t *collection, const char *key, const char *value);

/* Player methods */

zzub_player_t *zzub_player_create();
void zzub_player_destroy(zzub_player_t *player);
void zzub_player_add_plugin_path(zzub_player_t *player, const char* path);
void zzub_player_blacklist_plugin(zzub_player_t *player, const char* uri);
void zzub_player_add_plugin_alias(zzub_player_t *player, const char* name, const char* uri);
int zzub_player_initialize(zzub_player_t *player, int samplesPerSecond);
int zzub_player_load_bmx(zzub_player_t *player, const char* fileName);
int zzub_player_save_bmx(zzub_player_t *player, const char* fileName);
int zzub_player_load_ccm(zzub_player_t *player, const char* fileName);
int zzub_player_save_ccm(zzub_player_t *player, const char* fileName);
void zzub_player_set_state(zzub_player_t *player, int state);
int zzub_player_get_state(zzub_player_t *);
void zzub_player_set_position(zzub_player_t *player, int tick);
float zzub_player_get_bpm(zzub_player_t *player);
int zzub_player_get_tpb(zzub_player_t *player);
void zzub_player_set_bpm(zzub_player_t *player, float bpm);
void zzub_player_set_tpb(zzub_player_t *player, int tpb);

int zzub_player_get_pluginloader_count(zzub_player_t *player);
zzub_pluginloader_t *zzub_player_get_pluginloader(zzub_player_t *player, int index);
zzub_pluginloader_t *zzub_player_get_pluginloader_by_name(zzub_player_t *player, const char* name);

int zzub_player_get_plugin_count(zzub_player_t *player);
zzub_plugin_t *zzub_player_get_plugin(zzub_player_t *player, int index);
zzub_plugin_t *zzub_player_get_plugin_by_name(zzub_player_t *player, const char* name);

float** zzub_player_work_stereo(zzub_player_t *player, int* numSamples);
void zzub_player_clear(zzub_player_t *player);
int zzub_player_get_position(zzub_player_t *player);
void zzub_player_set_position(zzub_player_t *player, int pos);
zzub_sequencer_t *zzub_player_get_current_sequencer(zzub_player_t *player);
void zzub_player_set_current_sequencer(zzub_player_t *player, zzub_sequencer_t *sequencer);

int zzub_player_get_loop_start(zzub_player_t *player);
int zzub_player_get_loop_end(zzub_player_t *player);
int zzub_player_get_song_start(zzub_player_t *player);
void zzub_player_set_loop_start(zzub_player_t *player, int v);
void zzub_player_set_loop_end(zzub_player_t *player, int v);
void zzub_player_set_song_start(zzub_player_t *player, int v);
int zzub_player_get_song_end(zzub_player_t *player);
void zzub_player_set_song_end(zzub_player_t *player, int v);
void zzub_player_lock_tick(zzub_player_t *player);
void zzub_player_unlock_tick(zzub_player_t *player);
void zzub_player_lock(zzub_player_t *player);
void zzub_player_unlock(zzub_player_t *player);
void zzub_player_set_loop_enabled(zzub_player_t *player, int enable);
int zzub_player_get_loop_enabled(zzub_player_t *player);

zzub_sequence_t *zzub_player_get_sequence(zzub_player_t *player, int index);
int zzub_player_get_sequence_count(zzub_player_t *player);

int zzub_player_get_wave_count(zzub_player_t* player);
zzub_wave_t* zzub_player_get_wave(zzub_player_t* player, int index);

void zzub_player_play_wave(zzub_player_t* player, zzub_wave_t* wave, int level, int note);
void zzub_player_stop_wave(zzub_player_t* player);
void zzub_player_set_wave_amp(zzub_player_t *player, float amp);
float zzub_player_get_wave_amp(zzub_player_t *player);

void zzub_player_set_callback(zzub_player_t* player, ZzubCallback callback, void* tag);
void zzub_player_handle_events(zzub_player_t* player);

zzub_midimapping_t *zzub_player_add_midimapping(zzub_player_t* player, zzub_plugin_t *plugin, int group, int track, int param, int channel, int controller);
int zzub_player_remove_midimapping(zzub_player_t* player, zzub_plugin_t *plugin, int group, int track, int param);
zzub_midimapping_t *zzub_player_get_midimapping(zzub_player_t* player, int index);
int zzub_player_get_midimapping_count(zzub_player_t* player);

int zzub_player_get_automation(zzub_player_t* player);
void zzub_player_set_automation(zzub_player_t* player, int enable);

const char *zzub_player_get_infotext(zzub_player_t *player);
void zzub_player_set_infotext(zzub_player_t *player, const char *text);

void zzub_player_set_midi_plugin(zzub_player_t *player, zzub_plugin_t *plugin);
zzub_plugin_t *zzub_player_get_midi_plugin(zzub_player_t *player);

// midimapping functions

zzub_plugin_t *zzub_midimapping_get_plugin(zzub_midimapping_t *mapping);
int zzub_midimapping_get_group(zzub_midimapping_t *mapping);
int zzub_midimapping_get_track(zzub_midimapping_t *mapping);
int zzub_midimapping_get_column(zzub_midimapping_t *mapping);
int zzub_midimapping_get_channel(zzub_midimapping_t *mapping);
int zzub_midimapping_get_controller(zzub_midimapping_t *mapping);
	
// Plugin loading methods

const char *zzub_pluginloader_get_name(zzub_pluginloader_t* loader);
const char *zzub_pluginloader_get_short_name(zzub_pluginloader_t* loader);
int zzub_pluginloader_get_parameter_count(zzub_pluginloader_t* loader, int group);
const zzub_parameter_t *zzub_pluginloader_get_parameter(zzub_pluginloader_t* loader, int group, int index);
int zzub_pluginloader_get_attribute_count(zzub_pluginloader_t* loader);
const zzub_attribute_t *zzub_pluginloader_get_attribute(zzub_pluginloader_t* loader, int index);
const char *zzub_pluginloader_get_loader_name(zzub_pluginloader_t* loader);
int zzub_pluginloader_get_flags(zzub_pluginloader_t* loader);
const char *zzub_pluginloader_get_uri(zzub_pluginloader_t* loader);
const char *zzub_pluginloader_get_author(zzub_pluginloader_t* loader);

// parameter methods

int zzub_parameter_get_type(const zzub_parameter_t* param);
const char *zzub_parameter_get_name(const zzub_parameter_t* param);
const char *zzub_parameter_get_description(const zzub_parameter_t* param);
int zzub_parameter_get_value_min(const zzub_parameter_t* param);
int zzub_parameter_get_value_max(const zzub_parameter_t* param);
int zzub_parameter_get_value_none(const zzub_parameter_t* param);
int zzub_parameter_get_value_default(const zzub_parameter_t* param);
int zzub_parameter_get_flags(const zzub_parameter_t* param);

// attribute methods

const char *zzub_attribute_get_name(const zzub_attribute_t *attrib);
int zzub_attribute_get_value_min(const zzub_attribute_t *attrib);
int zzub_attribute_get_value_max(const zzub_attribute_t *attrib);
int zzub_attribute_get_value_default(const zzub_attribute_t *attrib);
	
// Machine methods

zzub_plugin_t *zzub_player_create_plugin(zzub_player_t *player, zzub_input_t *input, int dataSize, char* instanceName, zzub_pluginloader_t* loader);
int zzub_plugin_destroy(zzub_plugin_t *machine);
int zzub_plugin_set_name(zzub_plugin_t *machine, char* name);
int zzub_plugin_get_name(zzub_plugin_t *machine, char* name, int maxlen);
int zzub_plugin_get_commands(zzub_plugin_t *machine, char* commands, int maxlen);
int zzub_plugin_get_sub_commands(zzub_plugin_t *machine, int i, char* commands, int maxlen);
void zzub_plugin_command(zzub_plugin_t *machine, int i);
int zzub_plugin_get_flags(zzub_plugin_t *machine);
int zzub_plugin_get_output_channels(zzub_plugin_t *machine);
zzub_pluginloader_t *zzub_plugin_get_pluginloader(zzub_plugin_t *machine);
void zzub_plugin_add_pattern(zzub_plugin_t *machine, zzub_pattern_t *pattern);
void zzub_plugin_remove_pattern(zzub_plugin_t *machine, zzub_pattern_t *pattern);
void zzub_plugin_move_pattern(zzub_plugin_t *machine, int index, int newIndex);
zzub_pattern_t *zzub_plugin_get_pattern(zzub_plugin_t *machine, int index);
int zzub_plugin_get_pattern_index(zzub_plugin_t *machine, zzub_pattern_t *pattern);
zzub_pattern_t *zzub_plugin_get_pattern_by_name(zzub_plugin_t *machine, char* name);
int zzub_plugin_get_pattern_count(zzub_plugin_t *machine);
int zzub_plugin_get_parameter_value(zzub_plugin_t *machine, int group, int track, int column);
int zzub_plugin_describe_value(zzub_plugin_t *machine, int group, int column, int value, char* name, int maxlen);
void zzub_plugin_set_parameter_value(zzub_plugin_t *machine, int group, int track, int column, int value, int record);
void zzub_plugin_get_position(zzub_plugin_t *machine, float* x, float *y);
void zzub_plugin_set_position(zzub_plugin_t *machine, float x, float y);
int zzub_plugin_get_input_connection_count(zzub_plugin_t *machine);
zzub_connection_t *zzub_plugin_get_input_connection(zzub_plugin_t *machine, int index);
int zzub_plugin_get_output_connection_count(zzub_plugin_t *machine);
zzub_connection_t *zzub_plugin_get_output_connection(zzub_plugin_t *machine, int index);
void zzub_plugin_get_last_peak(zzub_plugin_t *machine, float *maxL, float *maxR);
zzub_connection_t *zzub_plugin_add_audio_input(zzub_plugin_t* machine, zzub_plugin_t* fromMachine, unsigned short amp, unsigned short pan);
zzub_connection_t *zzub_plugin_add_event_input(zzub_plugin_t* machine, zzub_plugin_t* fromMachine);
void zzub_plugin_delete_input(zzub_plugin_t* machine, zzub_plugin_t* fromMachine);
void zzub_plugin_set_input_channels(zzub_plugin_t* machine, zzub_plugin_t* fromMachine, int channels);
int zzub_plugin_get_track_count(zzub_plugin_t* machine);
void zzub_plugin_set_track_count(zzub_plugin_t* machine, int count);
int zzub_plugin_get_mute(zzub_plugin_t* machine);
void zzub_plugin_set_mute(zzub_plugin_t* machine, int muted);
int zzub_plugin_get_mixbuffer(zzub_plugin_t *machine, float *leftbuffer, float *rightbuffer, int *size, long long *samplepos);
// hd recording
int zzub_plugin_set_wave_file_path(zzub_plugin_t* machine, const char *path);
const char *zzub_plugin_get_wave_file_path(zzub_plugin_t* machine);
void zzub_plugin_set_write_wave(zzub_plugin_t* machine, int enable);
int zzub_plugin_get_write_wave(zzub_plugin_t* machine);
void zzub_plugin_set_start_write_position(zzub_plugin_t* machine, int position);
void zzub_plugin_set_end_write_position(zzub_plugin_t* machine, int position);
int zzub_plugin_get_start_write_position(zzub_plugin_t* machine);
int zzub_plugin_get_end_write_position(zzub_plugin_t* machine);
void zzub_plugin_set_auto_write(zzub_plugin_t* machine, int enable);
int zzub_plugin_get_auto_write(zzub_plugin_t* machine);
int zzub_plugin_get_ticks_written(zzub_plugin_t* machine);
void zzub_plugin_reset_ticks_written(zzub_plugin_t* machine);
// events
int zzub_plugin_invoke_event(zzub_plugin_t* machine, zzub_event_data_t *data, int immediate);
double zzub_plugin_get_last_worktime(zzub_plugin_t* machine);
void zzub_plugin_tick(zzub_plugin_t *machine);
int zzub_plugin_get_attribute_value(zzub_plugin_t *machine, int index);
void zzub_plugin_set_attribute_value(zzub_plugin_t *machine, int index, int value);
void zzub_plugin_get_new_pattern_name(zzub_plugin_t *machine, char* name, int maxLen);
zzub_postprocess_t *zzub_plugin_add_post_process(zzub_plugin_t *machine, ZzubMixCallback mixcallback, void *tag);
void zzub_plugin_remove_post_process(zzub_plugin_t *machine, zzub_postprocess_t *pp);
void zzub_plugin_play_midi_note(zzub_plugin_t *plugin, int note, int prevNote, int velocity);

// Connection methods

zzub_plugin_t *zzub_connection_get_input(zzub_connection_t *connection);
zzub_plugin_t *zzub_connection_get_output(zzub_connection_t *connection);
int zzub_connection_get_type(zzub_connection_t *connection);
zzub_audio_connection_t *zzub_connection_get_audio_connection(zzub_connection_t *connection);
zzub_event_connection_t *zzub_connection_get_event_connection(zzub_connection_t *connection);

// Audio connection methods

unsigned short zzub_audio_connection_get_amplitude(zzub_audio_connection_t *connection);
unsigned short zzub_audio_connection_get_panning(zzub_audio_connection_t *connection);
void zzub_audio_connection_set_amplitude(zzub_audio_connection_t *connection, unsigned short amp);
void zzub_audio_connection_set_panning(zzub_audio_connection_t *connection, unsigned short pan);

// Event connection methods
int zzub_event_connection_add_binding(zzub_event_connection_t *connection, int sourceparam, int targetgroup, int targettrack, int targetparam);
int zzub_event_connection_get_binding_count(zzub_event_connection_t *connection);
zzub_event_connection_binding_t *zzub_event_connection_get_binding(zzub_event_connection_t *connection, int index);
int zzub_event_connection_remove_binding(zzub_event_connection_t *connection, int index);

// event connection binding methods
int zzub_event_connection_binding_get_group(zzub_event_connection_binding_t *binding);
int zzub_event_connection_binding_get_track(zzub_event_connection_binding_t *binding);
int zzub_event_connection_binding_get_column(zzub_event_connection_binding_t *binding);
int zzub_event_connection_binding_get_controller(zzub_event_connection_binding_t *binding);

// Sequencer methods

void zzub_sequencer_destroy(zzub_sequencer_t *sequencer);
zzub_sequencer_t *zzub_sequencer_create_range(zzub_sequencer_t *sequencer, int fromRow, int fromTrack, int toRow, int toTrack);
int zzub_sequencer_get_track_count(zzub_sequencer_t *sequencer);
zzub_sequence_t *zzub_sequencer_get_track(zzub_sequencer_t *sequencer, int index);
void zzub_sequencer_move_track(zzub_sequencer_t *sequencer, int index, int newIndex);
zzub_sequence_t *zzub_sequencer_create_track(zzub_sequencer_t *sequencer, zzub_plugin_t *machine);
void zzub_sequencer_remove_track(zzub_sequencer_t *sequencer, int index);

// Sequence track methods

unsigned long zzub_sequence_get_value_at(zzub_sequence_t *track, unsigned long pos, int* exists);
void zzub_sequence_set_event(zzub_sequence_t *track, unsigned long pos, unsigned long value);
int zzub_sequence_get_event_count(zzub_sequence_t *track);
int zzub_sequence_get_event(zzub_sequence_t *track, int index, unsigned long* pos, unsigned long* value);
void zzub_sequence_move_events(zzub_sequence_t *track, unsigned long fromRow, unsigned long delta);
zzub_plugin_t *zzub_sequence_get_plugin(zzub_sequence_t *track);
void zzub_sequence_remove_event_at(zzub_sequence_t *track, unsigned long pos);
void zzub_sequence_remove_event_range(zzub_sequence_t *track, unsigned long from_pos, unsigned long to_pos);
void zzub_sequence_remove_event_value(zzub_sequence_t *track, unsigned long value);

//	Pattern methods

zzub_pattern_t *zzub_plugin_create_pattern(zzub_plugin_t *machine, int row);
void zzub_pattern_destroy(zzub_pattern_t *pattern);
void zzub_pattern_get_name(zzub_pattern_t *pattern, char* name, int maxLen);
void zzub_pattern_set_name(zzub_pattern_t *pattern, const char* name);
void zzub_pattern_set_row_count(zzub_pattern_t *pattern, int rows);
int zzub_pattern_get_row_count(zzub_pattern_t *pattern);
void zzub_pattern_insert_row(zzub_pattern_t *pattern, int group, int track, int column, int row);
void zzub_pattern_delete_row(zzub_pattern_t *pattern, int group, int track, int column, int row);
zzub_patterntrack_t *zzub_pattern_get_track(zzub_pattern_t *pattern, int index);
int zzub_pattern_get_track_count(zzub_pattern_t *pattern);
int zzub_pattern_get_value(zzub_pattern_t* pattern, int row, int group, int track, int column);
void zzub_pattern_set_value(zzub_pattern_t* pattern, int row, int group, int track, int column, int value);
void zzub_pattern_get_bandwidth_digest(zzub_pattern_t* pattern, float *digest, int digestsize);
//zzub_plugin_t* zzub_pattern_get_plugin(zzub_pattern_t* pattern);

// Wave table

int zzub_wave_get_level_count(zzub_wave_t* wave);
zzub_wavelevel_t *zzub_wave_get_level(zzub_wave_t* wave, int index);
const char *zzub_wave_get_name(zzub_wave_t* wave);
const char *zzub_wave_get_path(zzub_wave_t* wave);
int zzub_wave_get_flags(zzub_wave_t* wave);
float zzub_wave_get_volume(zzub_wave_t* wave);
void zzub_wave_clear(zzub_wave_t* wave);
int zzub_wave_load_sample(zzub_wave_t* wave, int level, const char *path);
int zzub_wave_save_sample(zzub_wave_t* wave, int level, const char *path);
void zzub_wave_set_volume(zzub_wave_t* wave, float volume);
void zzub_wave_set_path(zzub_wave_t* wave, const char *path);
void zzub_wave_set_name(zzub_wave_t* wave, const char *name);
void zzub_wave_set_flags(zzub_wave_t* wave, int flags);
int zzub_wave_get_envelope_count(zzub_wave_t* wave);
zzub_envelope_t *zzub_wave_get_envelope(zzub_wave_t* wave, int index);

int zzub_wavelevel_get_sample_count(zzub_wavelevel_t * level);
void *zzub_wavelevel_get_samples(zzub_wavelevel_t * level);
void zzub_wavelevel_get_samples_digest(zzub_wavelevel_t * level, int channel, int start, int end, float *mindigest, float *maxdigest, float *ampdigest, int digestsize);
int zzub_wavelevel_get_root_note(zzub_wavelevel_t * level);
int zzub_wavelevel_get_samples_per_second(zzub_wavelevel_t * level);
int zzub_wavelevel_get_loop_start(zzub_wavelevel_t * level);
int zzub_wavelevel_get_loop_end(zzub_wavelevel_t * level);

void zzub_wavelevel_set_root_note(zzub_wavelevel_t * level, int rootnote);
void zzub_wavelevel_set_samples_per_second(zzub_wavelevel_t * level, int samplespersecond);
void zzub_wavelevel_set_loop_start(zzub_wavelevel_t * level, int loopstart);
void zzub_wavelevel_set_loop_end(zzub_wavelevel_t * level, int loopend);

int zzub_wavelevel_silence_range(zzub_wavelevel_t * level, int start, int end);
int zzub_wavelevel_remove_range(zzub_wavelevel_t * level, int start, int end);
int zzub_wavelevel_stretch_range(zzub_wavelevel_t * level, int start, int end, int newsize);
int zzub_wavelevel_insert(zzub_wavelevel_t * level, int start, void* sampleData, int channels, int waveFormat, int numSamples);
	
int zzub_wavelevel_get_format(zzub_wavelevel_t * level);

// envelope

unsigned short zzub_envelope_get_attack(zzub_envelope_t *env);
unsigned short zzub_envelope_get_decay(zzub_envelope_t *env);
unsigned short zzub_envelope_get_sustain(zzub_envelope_t *env);
unsigned short zzub_envelope_get_release(zzub_envelope_t *env);
void zzub_envelope_set_attack(zzub_envelope_t *env, unsigned short attack);
void zzub_envelope_set_decay(zzub_envelope_t *env, unsigned short decay);
void zzub_envelope_set_sustain(zzub_envelope_t *env, unsigned short sustain);
void zzub_envelope_set_release(zzub_envelope_t *env, unsigned short release);
char zzub_envelope_get_subdivision(zzub_envelope_t *env);
void zzub_envelope_set_subdivision(zzub_envelope_t *env, char subdiv);
char zzub_envelope_get_flags(zzub_envelope_t *env);
void zzub_envelope_set_flags(zzub_envelope_t *env, char flags);
int zzub_envelope_is_enabled(zzub_envelope_t *env);
void zzub_envelope_enable(zzub_envelope_t *env, int enable);
int zzub_envelope_get_point_count(zzub_envelope_t *env);
void zzub_envelope_get_point(zzub_envelope_t *env, int index, unsigned short *x, unsigned short *y, unsigned char *flags);
void zzub_envelope_set_point(zzub_envelope_t *env, int index, unsigned short x, unsigned short y, unsigned char flags);
void zzub_envelope_insert_point(zzub_envelope_t *env, int index);
void zzub_envelope_delete_point(zzub_envelope_t *env, int index);

// recording functions
/*
zzub_recorder_t* zzub_file_recorder_create();
int zzub_file_recorder_set_wave_path(zzub_recorder_t*, char* wavefile);
void zzub_file_recorder_get_wave_path(zzub_recorder_t*, char* wavefile, int maxlen);

zzub_recorder_t* zzub_wavetable_recorder_create();
int zzub_wavetable_recorder_set_waveindex(zzub_recorder_t*, int index);
int zzub_wavetable_recorder_get_waveindex(zzub_recorder_t*);
void zzub_recorder_destroy(zzub_recorder_t*);
*/

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // __LIBZZUB_H
