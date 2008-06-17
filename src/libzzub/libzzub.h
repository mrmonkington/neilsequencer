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

/*! \mainpage libzzub C documentation

	\section intro Introduction
	
	See the documentation on <a href="libzzub_8h.html">libzzub.h</a> for an overview 
	of all C methods exposed by libzzub.

	This is a pure C library header for libzzub. If you wish
	to use classes directly, please include the according
	class headers.

 */

#if !defined(__LIBZZUB_H)
#define __LIBZZUB_H

#include "zzub/types.h"

#if defined(__cplusplus)
extern "C" {
#endif

/** @name Audio Driver Methods 
	Configure and create an audio driver instance.
*/
/*@{*/


/** \brief Create an audio driver that uses the PortAudio API. */
zzub_audiodriver_t* zzub_audiodriver_create_portaudio(zzub_player_t* player);

/** \brief Create an audio driver that uses the RtAudio API. */
zzub_audiodriver_t* zzub_audiodriver_create_rtaudio(zzub_player_t* player);

/** \brief Create a silent, non-processing audio driver that has one device with the specified properties. */
zzub_audiodriver_t* zzub_audiodriver_create_silent(zzub_player_t* player, const char* name, int out_channels, int in_channels, int* supported_rates, int num_rates);

/** \brief Creates the preferred audio driver. */
zzub_audiodriver_t* zzub_audiodriver_create(zzub_player_t* player);

/** \brief Get number of detected input and output audio devices */
int zzub_audiodriver_get_count(zzub_audiodriver_t* driver);

/** \brief Get name of specified audio device */
int zzub_audiodriver_get_name(zzub_audiodriver_t* driver, int index, char* name, int maxLen);

int zzub_audiodriver_get_supported_samplerates(zzub_audiodriver_t* driver, int index, int* result, int maxrates);
int zzub_audiodriver_get_supported_output_channels(zzub_audiodriver_t* driver, int index);
int zzub_audiodriver_get_supported_input_channels(zzub_audiodriver_t* driver, int index);

/** \brief Create specified audio device. */
int zzub_audiodriver_create_device(zzub_audiodriver_t* driver, int input_index, int output_index);
	
/** \brief Enable or disable current audio driver */
void zzub_audiodriver_enable(zzub_audiodriver_t* driver, int state);

/** \brief Returns whether current audio driver is enabled or disabled */
int zzub_audiodriver_get_enabled(zzub_audiodriver_t* driver);

/** \brief Disassociate audio driver and player */
void zzub_audiodriver_destroy(zzub_audiodriver_t* driver);

/** \brief De-allocate the current device. */
void zzub_audiodriver_destroy_device(zzub_audiodriver_t* driver);

/** \brief Set audio driver sample rate */
void zzub_audiodriver_set_samplerate(zzub_audiodriver_t* driver, unsigned int samplerate);

/** \brief Retreive audio driver sample rate */
unsigned int zzub_audiodriver_get_samplerate(zzub_audiodriver_t* driver);

void zzub_audiodriver_set_buffersize(zzub_audiodriver_t* driver, unsigned int buffersize);
unsigned int zzub_audiodriver_get_buffersize(zzub_audiodriver_t* driver);
double zzub_audiodriver_get_cpu_load(zzub_audiodriver_t* driver);
int zzub_audiodriver_is_output(zzub_audiodriver_t* driver, int index);
int zzub_audiodriver_is_input(zzub_audiodriver_t* driver, int index);
int zzub_audiodriver_get_master_channel(zzub_audiodriver_t* driver);
void zzub_audiodriver_set_master_channel(zzub_audiodriver_t* driver, int index);

/** @name MIDI Driver Methods 
	Open midi devices.
*/
/* midi driver */

int zzub_mididriver_get_count(zzub_player_t *player);
const char *zzub_mididriver_get_name(zzub_player_t *player, int index);
int zzub_mididriver_is_input(zzub_player_t *player, int index);
int zzub_mididriver_is_output(zzub_player_t *player, int index);
int zzub_mididriver_open(zzub_player_t *player, int index);
int zzub_mididriver_close_all(zzub_player_t *player);


/*@}*/
/** @name Plugin Collection Methods 
	For enumerating and configuring plugin collections.
*/
/*@{*/

zzub_plugincollection_t *zzub_player_get_plugincollection_by_uri(zzub_player_t *player, const char *uri);
void zzub_plugincollection_configure(zzub_plugincollection_t *collection, const char *key, const char *value);

/*@}*/
/** @name Player Methods */
/*@{*/

/** \brief Create a player instance. */
zzub_player_t *zzub_player_create();

/** \brief Destroy a player instance and all its resources. */
void zzub_player_destroy(zzub_player_t *player);

/** \brief Adds a directory that will be scanned for plugins upon initialization. The path *must* be terminated with an ending (back)slash.  */
void zzub_player_add_plugin_path(zzub_player_t *player, const char* path);

/** \brief Blacklist plugin. */
void zzub_player_blacklist_plugin(zzub_player_t *player, const char* uri);
//void zzub_player_add_plugin_alias(zzub_player_t *player, const char* name, const char* uri);

/** \brief Inititializes the player.
	zzub_player_initialize() must be called only after the audio driver, 
	plugin directories and optional blacklists are set up.
*/
int zzub_player_initialize(zzub_player_t *player, int samplesPerSecond);

/** \brief Loads a BMX from memory or file.

Load warnings and error messages are placed in the messages string.*/
int zzub_player_load_bmx(zzub_player_t *player, zzub_input_t* datastream, char* messages, int maxLen);

/** \brief Saves a BMX to memory or file. 

plugins is an array of ints containing the plugin ids to save in the song. If plugins is NULL,
everything in the running graph is saved. Optionally without waves, when save_waves is zero. */
int zzub_player_save_bmx(zzub_player_t *player, zzub_plugin_t** plugins, int num_plugins, int save_waves, zzub_output_t* datastream);

/** \brief Load a project in CCM file format from disk. */
int zzub_player_load_ccm(zzub_player_t *player, const char* fileName);

/** \brief Save current project in the CCM file format to disk. */
int zzub_player_save_ccm(zzub_player_t *player, const char* fileName);

/** \brief Returns one of the values in the #zzub_player_state enumeration. */
int zzub_player_get_state(zzub_player_t *);

/** \brief Set player state. Takes one of the values in the #zzub_player_state enumeration as parameter. */
void zzub_player_set_state(zzub_player_t *player, int state);

void zzub_player_set_position(zzub_player_t *player, int tick);
float zzub_player_get_bpm(zzub_player_t *player);
int zzub_player_get_tpb(zzub_player_t *player);
void zzub_player_set_bpm(zzub_player_t *player, float bpm);
void zzub_player_set_tpb(zzub_player_t *player, int tpb);

/** \brief Returns number of plugin loaders. */
int zzub_player_get_pluginloader_count(zzub_player_t *player);

/** \brief Returns a zzub_pluginloader_t handle by index. */
zzub_pluginloader_t *zzub_player_get_pluginloader(zzub_player_t *player, int index);

/** \brief Finds a zzub_pluginloader_t handle by uri. */
zzub_pluginloader_t *zzub_player_get_pluginloader_by_name(zzub_player_t *player, const char* name);

/** \brief Returns number of plugins in the current song. */
int zzub_player_get_plugin_count(zzub_player_t *player);

//zzub_plugin_t *zzub_player_get_plugin(zzub_player_t *player, int index);

/** \brief Returns the plugin object given the plugins name. */
zzub_plugin_t* zzub_player_get_plugin_by_name(zzub_player_t *player, const char* name);

/** \brief Returns the plugin object given the plugin id. See also zzub_plugin_get_id(). */
zzub_plugin_t* zzub_player_get_plugin_by_id(zzub_player_t *player, int id);

/** \brief Returns the plugin object given the plugins index in the graph. */
zzub_plugin_t* zzub_player_get_plugin(zzub_player_t* player, int index);

float** zzub_player_work_stereo(zzub_player_t *player, int* numSamples);
void zzub_player_clear(zzub_player_t *player);
int zzub_player_get_position(zzub_player_t *player);
void zzub_player_set_position(zzub_player_t *player, int pos);

int zzub_player_get_loop_start(zzub_player_t *player);
int zzub_player_get_loop_end(zzub_player_t *player);
int zzub_player_get_song_start(zzub_player_t *player);
void zzub_player_set_loop_start(zzub_player_t *player, int v);
void zzub_player_set_loop_end(zzub_player_t *player, int v);
void zzub_player_set_song_start(zzub_player_t *player, int v);
int zzub_player_get_song_end(zzub_player_t *player);
void zzub_player_set_song_end(zzub_player_t *player, int v);
//void zzub_player_lock_tick(zzub_player_t *player);
//void zzub_player_unlock_tick(zzub_player_t *player);
//void zzub_player_lock(zzub_player_t *player);
//void zzub_player_unlock(zzub_player_t *player);
void zzub_player_set_loop_enabled(zzub_player_t *player, int enable);
int zzub_player_get_loop_enabled(zzub_player_t *player);
int zzub_player_get_sequence_track_count(zzub_player_t *player);

/** \brief Retreive the currently playing pattern and row for a plugin. */
int zzub_player_get_currently_playing_pattern(zzub_plugin_t *plugin, int* pattern, int* row);

/** \brief Retreive the currently playing row for a plugin and a pattern. */
int zzub_player_get_currently_playing_pattern_row(zzub_plugin_t *plugin, int pattern, int* row);

int zzub_player_get_wave_count(zzub_player_t* player);

zzub_wave_t* zzub_player_get_wave(zzub_player_t* player, int index);

//void zzub_player_play_wave(zzub_player_t* player, zzub_wave_t* wave, int level, int note);
//void zzub_player_stop_wave(zzub_player_t* player);
//void zzub_player_set_wave_amp(zzub_player_t *player, float amp);
//float zzub_player_get_wave_amp(zzub_player_t *player);

/** \brief Sets a function that receives events. */
void zzub_player_set_callback(zzub_player_t* player, ZzubCallback callback, void* tag);

/** \brief Process player events. Intended to be called by the host in a timer
	or on idle processing to receive events about parameter changes etc. */
void zzub_player_handle_events(zzub_player_t* player);

zzub_midimapping_t *zzub_player_add_midimapping(zzub_plugin_t *plugin, int group, int track, int param, int channel, int controller);
int zzub_player_remove_midimapping(zzub_plugin_t *plugin, int group, int track, int param);
zzub_midimapping_t *zzub_player_get_midimapping(zzub_player_t* player, int index);
int zzub_player_get_midimapping_count(zzub_player_t* player);

int zzub_player_get_automation(zzub_player_t* player);
void zzub_player_set_automation(zzub_player_t* player, int enable);
int zzub_player_get_midi_transport(zzub_player_t* player);
void zzub_player_set_midi_transport(zzub_player_t* player, int enable);

const char *zzub_player_get_infotext(zzub_player_t *player);
void zzub_player_set_infotext(zzub_player_t *player, const char *text);

/** \brief Sets the plugin to receive MIDI data if the plugin's internal MIDI 
	channel is set to the special channel 17 ("Play if selected"). */
void zzub_player_set_midi_plugin(zzub_player_t *player, zzub_plugin_t *plugin);

zzub_plugin_t* zzub_player_get_midi_plugin(zzub_player_t *player);

/** \brief Generates a new plugin name that can be used in a call to zzub_player_create_plugin(). */
void zzub_player_get_new_plugin_name(zzub_player_t *player, const char* uri, char* name, int maxLen);

void zzub_player_reset_keyjazz(zzub_player_t *player);


/*@}*/
/** @name Undo/Redo Methods 

All operations in zzub go through a "back buffer", a copy of the song which is populated on demand.
Changes are made visible/audible by calling zzub_player_flush().

*/
/*@{*/

/** \brief Write changes made to the graph since zzub_player_begin().

When redo_event and/or undo_event are NULL, zzub will invoke the callback for every editing operation.
If a custom event is specified, the callback is invoked only once with either redo_event or undo_event
as its parameter.
*/
void zzub_player_flush(zzub_player_t *player, zzub_event_data_t* redo_event, zzub_event_data_t* undo_event);

/** \brief Rolls back all editing operations one step. Each step is defined with a call to zzub_player_history_commit(). */
void zzub_player_undo(zzub_player_t *player);

/** \brief Redoes all editing operations since last call to zzub_player_history_commit(). */
void zzub_player_redo(zzub_player_t *player);

/** \brief Commits the last operations to the undo buffer and marks a new undo step. */
void zzub_player_history_commit(zzub_player_t *player, const char* description);

/** \brief Causes the last operations to not appear in the undo buffer. */
void zzub_player_history_flush_last(zzub_player_t *player);

/** \brief Clears the undo buffer and frees all associated resources. */
void zzub_player_history_flush(zzub_player_t *player);

/** \brief Returns the size of the undo buffer. */
int zzub_player_history_get_size(zzub_player_t* player);

/** \brief Returns the current position in the undo buffer. */
int zzub_player_history_get_position(zzub_player_t* player);

/** \brief Returns the description of an operation in the undo buffer. */
const char* zzub_player_history_get_description(zzub_player_t* player, int position);

/*@}*/
/** @name MIDI Mapping Methods */
/*@{*/

int zzub_midimapping_get_plugin(zzub_midimapping_t *mapping);
int zzub_midimapping_get_group(zzub_midimapping_t *mapping);
int zzub_midimapping_get_track(zzub_midimapping_t *mapping);
int zzub_midimapping_get_column(zzub_midimapping_t *mapping);
int zzub_midimapping_get_channel(zzub_midimapping_t *mapping);
int zzub_midimapping_get_controller(zzub_midimapping_t *mapping);

/*@}*/
/** @name Plugin loading methods
	Retreive more details from zzub_pluginloader_t objects.*/
/*@{*/

const char *zzub_pluginloader_get_name(zzub_pluginloader_t* loader);
const char *zzub_pluginloader_get_short_name(zzub_pluginloader_t* loader);
int zzub_pluginloader_get_parameter_count(zzub_pluginloader_t* loader, int group);

/** \brief Returns the parameter for a group and column. See also zzub_plugin_get_parameter() which also returns parameters in group 0. */
const zzub_parameter_t *zzub_pluginloader_get_parameter(zzub_pluginloader_t* loader, int group, int index);

int zzub_pluginloader_get_attribute_count(zzub_pluginloader_t* loader);
const zzub_attribute_t *zzub_pluginloader_get_attribute(zzub_pluginloader_t* loader, int index);
const char *zzub_pluginloader_get_loader_name(zzub_pluginloader_t* loader);

/** \brief Returns the flags for this plugin loader. Combined by zero or more values in the #zzub_plugin_flag enumeration. */
int zzub_pluginloader_get_flags(zzub_pluginloader_t* loader);

const char *zzub_pluginloader_get_uri(zzub_pluginloader_t* loader);
const char *zzub_pluginloader_get_author(zzub_pluginloader_t* loader);

int zzub_pluginloader_get_instrument_list(zzub_pluginloader_t* loader, char* result, int maxbytes);

int zzub_pluginloader_get_tracks_min(zzub_pluginloader_t* loader);
int zzub_pluginloader_get_tracks_max(zzub_pluginloader_t* loader);

/** \brief Returns the number of supported stream formats. Used with plugins flagged #zzub_plugin_flag_stream. */
int zzub_pluginloader_get_stream_format_count(zzub_pluginloader_t* loader);

/** \brief Returns a supported stream file format extension stream. Used with plugins flagged #zzub_plugin_flag_stream. */
const char* zzub_pluginloader_get_stream_format_ext(zzub_pluginloader_t* loader, int index);

//int zzub_pluginloader_get_import_format_count(zzub_pluginloader_t* loader);
//const char* zzub_pluginloader_get_import_format_ext(zzub_pluginloader_t* loader, int index);

/*@}*/
/** @name Parameter methods
	Retreive more details from zzub_parameter_t objects.*/
/*@{*/

/** \brief Returns one of the values in the #zzub_parameter_type enumeration. */
int zzub_parameter_get_type(const zzub_parameter_t* param);

const char *zzub_parameter_get_name(const zzub_parameter_t* param);
const char *zzub_parameter_get_description(const zzub_parameter_t* param);
int zzub_parameter_get_value_min(const zzub_parameter_t* param);
int zzub_parameter_get_value_max(const zzub_parameter_t* param);
int zzub_parameter_get_value_none(const zzub_parameter_t* param);
int zzub_parameter_get_value_default(const zzub_parameter_t* param);

/** \brief A parameter flag is combined by zero or more values in the #zzub_parameter_flag enumeration. */
int zzub_parameter_get_flags(const zzub_parameter_t* param);

/*@}*/
/** @name Attribute methods
	Retreive more details from zzub_attribute_t objects.*/
/*@{*/

const char *zzub_attribute_get_name(const zzub_attribute_t *attrib);
int zzub_attribute_get_value_min(const zzub_attribute_t *attrib);
int zzub_attribute_get_value_max(const zzub_attribute_t *attrib);
int zzub_attribute_get_value_default(const zzub_attribute_t *attrib);
	
/*@}*/
/** @name Plugin methods
	Retreive more details about plugins.*/
/*@{*/

/** \brief Create a new plugin */
zzub_plugin_t* zzub_player_create_plugin(zzub_player_t *player, zzub_input_t *input, int dataSize, const char* instanceName, zzub_pluginloader_t* loader);

/** \brief Deletes a plugin */
int zzub_plugin_destroy(zzub_plugin_t *plugin);

/** \brief Load plugin state. */
int zzub_plugin_load(zzub_plugin_t *plugin, zzub_input_t *input);

/** \brief Save plugin state. */
int zzub_plugin_save(zzub_plugin_t *plugin, zzub_output_t *ouput);

/** \brief Renames a plugin. Should fail and return -1 if the name already exists. */
int zzub_plugin_set_name(zzub_plugin_t *plugin, char* name);

/** \brief Retreive the name of a plugin. */
int zzub_plugin_get_name(zzub_plugin_t *plugin, char* name, int maxlen);

/** \brief Retreive the unique per-session id of a plugin. See also zzub_player_get_plugin_by_id(). */
int zzub_plugin_get_id(zzub_plugin_t *plugin);

/** \brief Returns the screen position coordinates for the plugin. Values are expected to be in the range -1..1. */
void zzub_plugin_get_position(zzub_plugin_t *plugin, float* x, float *y);

/** \brief Sets the plugin screen position. Values are expected to be in the range -1..1. */
void zzub_plugin_set_position(zzub_plugin_t *plugin, float x, float y);

/** \brief Sets the plugin screen position. Values are expected to be in the range -1..1. This method is not undoable. */
void zzub_plugin_set_position_direct(zzub_plugin_t *plugin, float x, float y);

/** \brief Returns flags for this plugin. Shorthand for using zzub_pluginloader_get_flags(). Combined by zero or more values in the #zzub_plugin_flag enumeration. */
int zzub_plugin_get_flags(zzub_plugin_t *plugin);

/** \brief Returns the number of tracks. */
int zzub_plugin_get_track_count(zzub_plugin_t *plugin);

/** \brief Sets the number of tracks. Will call plugin::set_track_count() from the player thread. */
void zzub_plugin_set_track_count(zzub_plugin_t *plugin, int count);

/** \brief Returns 1 if plugin is muted, otherwise 0. */
int zzub_plugin_get_mute(zzub_plugin_t *plugin);

/** \brief Set whether plugin is muted. 1 for muted, 0 for normal. 
	A muted machine does not produce any sound. */
void zzub_plugin_set_mute(zzub_plugin_t *plugin, int muted);

/** \brief Returns 1 if plugin is bypassed, otherwise 0. */
int zzub_plugin_get_bypass(zzub_plugin_t *plugin);

/** \brief Set whether plugin is bypassed. 1 for bypass, 0 for normal. 
	Bypass causes no processing to occur in the given machine. */
void zzub_plugin_set_bypass(zzub_plugin_t *plugin, int muted);

/** \brief Returns a string of \\n-separated command strings */
int zzub_plugin_get_commands(zzub_plugin_t *plugin, char* commands, int maxlen);

/** \brief When a plugin command string starts with the char '\', it has subcommands.
	Unexpectedly, zzub_plugin_get_sub_commands returns a \\n-separated string (like get_commands).
	Some plugins need to be ticked before calling get_sub_commands. */
int zzub_plugin_get_sub_commands(zzub_plugin_t *plugin, int i, char* commands, int maxlen);

/** \brief Invoke a command on the plugin. */
void zzub_plugin_command(zzub_plugin_t *plugin, int i);

/** \brief Returns the pluginloader used to create this plugin. */
zzub_pluginloader_t *zzub_plugin_get_pluginloader(zzub_plugin_t *plugin);

int zzub_plugin_get_midi_output_device_count(zzub_plugin_t *plugin);
const char* zzub_plugin_get_midi_output_device(zzub_plugin_t *plugin, int index);

int zzub_plugin_get_envelope_count(zzub_plugin_t *plugin);
int zzub_plugin_get_envelope_flags(zzub_plugin_t *plugin, int index);
const char* zzub_plugin_get_envelope_name(zzub_plugin_t *plugin, int index);

void zzub_plugin_set_stream_source(zzub_plugin_t *plugin, const char* resource);
// 0.3: DEAD // const char* zzub_plugin_get_stream_source(zzub_plugin_t *plugin);

/** \brief Sets the plugin instrument (d'oh!) */
int zzub_plugin_set_instrument(zzub_plugin_t *plugin, const char *name);

/*@}*/
/** @name Plugin pattern methods
	Manipulate patterns belonging to a plugin in the graph directly.*/
/*@{*/

/** \brief Returns how many patterns are associated with the plugin. */
int zzub_plugin_get_pattern_count(zzub_plugin_t *plugin);

/** \brief Adds a pattern at the end of the plugins list of patterns */
void zzub_plugin_add_pattern(zzub_plugin_t *plugin, zzub_pattern_t *pattern);

/** \brief Remove the pattern from the plugin */
void zzub_plugin_remove_pattern(zzub_plugin_t *plugin, int pattern);

/** \brief Change the order of patterns */
void zzub_plugin_move_pattern(zzub_plugin_t *plugin, int index, int newIndex);

/** \brief Replaces pattern contents  */
void zzub_plugin_update_pattern(zzub_plugin_t *plugin, int index, zzub_pattern_t* pattern);

/** \brief Returns a copy of the requested pattern. Callers must destroy the pattern returned from get_pattern */
zzub_pattern_t *zzub_plugin_get_pattern(zzub_plugin_t *plugin, int index);

/** \brief Returns the index of the pattern with the given name */
int zzub_plugin_get_pattern_by_name(zzub_plugin_t *plugin, char* name);

/** \brief Returns the name of given pattern. */
const char* zzub_plugin_get_pattern_name(zzub_plugin_t *plugin, int index);

/** \brief Updates the name of the pattern. */
void zzub_plugin_set_pattern_name(zzub_plugin_t *plugin, int index, const char* name);

/** \brief Returns the length of the pattern. */
int zzub_plugin_get_pattern_length(zzub_plugin_t *plugin, int index);

/** \brief Updates the number of rows in the pattern. */
void zzub_plugin_set_pattern_length(zzub_plugin_t *plugin, int index, int rows);

/** \brief Returns a value from the requested pattern. */
int zzub_plugin_get_pattern_value(zzub_plugin_t *plugin, int pattern, int group, int track, int column, int row);

/** \brief Sets a value in a pattern. */
void zzub_plugin_set_pattern_value(zzub_plugin_t *plugin, int pattern, int group, int track, int column, int row, int value);

void zzub_plugin_get_new_pattern_name(zzub_plugin_t *plugin, char* name, int maxLen);
int zzub_plugin_linear_to_pattern(zzub_plugin_t *plugin, int index, int* group, int* track, int* column);
int zzub_plugin_pattern_to_linear(zzub_plugin_t *plugin, int group, int track, int column, int* index);
int zzub_plugin_get_pattern_column_count(zzub_plugin_t *plugin);

/** \brief Inserts rows in a pattern. column_indices has a total length of 3 * num_indices, where each index is a triple of group, track and column. */
void zzub_plugin_insert_pattern_rows(zzub_plugin_t *plugin, int pattern, int* column_indices, int num_indices, int start, int rows);

/** \brief Removes rows in a pattern. column_indices has a total length of 3 * num_indices, where each index is a triple of group, track and column. */
void zzub_plugin_remove_pattern_rows(zzub_plugin_t *plugin, int pattern, int* column_indices, int num_indices, int start, int rows);

/** \brief Copies columns from an offline pattern to a live pattern. Source and target columns are set up in
	the mappings array, which has 6 ints for each mapping: group, track and column for source and target 
	plugins.*/
// 0.3: DEAD // void zzub_plugin_set_pattern_values(zzub_player_t* player, int plugin, int pattern, int target_row, zzub_pattern_t* src_pattern, int* mappings, int mappings_count);

/*@}*/
/** @name Plugin parameter methods
	Manipulate plugin parameters.*/
/*@{*/

/** \brief Creates a textual description of the given value. The return value is the number of characters in the output string. */
int zzub_plugin_describe_value(zzub_plugin_t *plugin, int group, int column, int value, char* name, int maxlen);

/** \brief Returns the last written value of the requested parameter. */
int zzub_plugin_get_parameter_value(zzub_plugin_t *plugin, int group, int track, int column);

/** \brief Sets the value of a plugin parameter. The method will wait for the player thread to pick up the modified value and call process_events(). */
void zzub_plugin_set_parameter_value(zzub_plugin_t *plugin, int group, int track, int column, int value, int record);

/** \brief Sets the value of a plugin parameter. Unlike zzub_plugin_set_parameter_value(), this method returns immediately. The parameter will be changed later when the player thread notices the modified value. Is also not undoable. */
void zzub_plugin_set_parameter_value_direct(zzub_plugin_t *plugin, int group, int track, int column, int value, int record);

int zzub_plugin_get_parameter_count(zzub_plugin_t *plugin, int group, int track);

const zzub_parameter_t* zzub_plugin_get_parameter(zzub_plugin_t *plugin, int group, int track, int column);

/*@}*/
/** @name Plugin connection methods */
/*@{*/

/** \brief Returns the number of input connections for given plugin. */
int zzub_plugin_get_input_connection_count(zzub_plugin_t *plugin);

/** \brief Returns the input connection index for given plugin and connection type. */
int zzub_plugin_get_input_connection_by_type(zzub_plugin_t *to_plugin, zzub_plugin_t* from_plugin, int type);

/** \brief Returns the connection type for given plugin and connection index. */
int zzub_plugin_get_input_connection_type(zzub_plugin_t *plugin, int index);

/** \brief Returns the plugin index for given plugin and connection index. */
zzub_plugin_t* zzub_plugin_get_input_connection_plugin(zzub_plugin_t *plugin, int index);

/** \brief Returns the number of output connections for given plugin. */
int zzub_plugin_get_output_connection_count(zzub_plugin_t *plugin);

/** \brief Returns the output connection index for given plugin and connection type. */
int zzub_plugin_get_output_connection_by_type(zzub_plugin_t *to_plugin, zzub_plugin_t* from_plugin, int type);

/** \brief Returns the connection type for given plugin and connection index. */
int zzub_plugin_get_output_connection_type(zzub_plugin_t *plugin, int index);

/** \brief Returns the plugin index for given plugin and connection index. */
zzub_plugin_t* zzub_plugin_get_output_connection_plugin(zzub_plugin_t *plugin, int index);

/** \brief Connect two plugins */
int zzub_plugin_add_input(zzub_plugin_t *to_plugin, zzub_plugin_t* from_plugin, int type);

/** \brief Disconnect two plugins */
void zzub_plugin_delete_input(zzub_plugin_t *to_plugin, zzub_plugin_t* from_plugin, int type);


/*@}*/
/** @name Plugin statistics 
	Retreiving statistics is not exact unless the graph is locked.
*/
/*@{*/

/** \brief Copies the given plugins work buffer. */
int zzub_plugin_get_mixbuffer(zzub_plugin_t *plugin, float *leftbuffer, float *rightbuffer, int *size, long long *samplepos);

void zzub_plugin_get_last_peak(zzub_plugin_t *plugin, float *maxL, float *maxR);
double zzub_plugin_get_last_worktime(zzub_plugin_t *plugin);
double zzub_plugin_get_last_cpu_load(zzub_plugin_t *plugin);
int zzub_plugin_get_last_midi_result(zzub_plugin_t *plugin);
int zzub_plugin_get_last_audio_result(zzub_plugin_t *plugin);

/*@}*/
/** @name Other plugin methdos */
/*@{*/

int zzub_plugin_invoke_event(zzub_plugin_t *plugin, zzub_event_data_t *data, int immediate);
void zzub_plugin_tick(zzub_plugin_t *plugin);
int zzub_plugin_get_attribute_value(zzub_plugin_t *plugin, int index);
void zzub_plugin_set_attribute_value(zzub_plugin_t *plugin, int index, int value);
void zzub_plugin_play_midi_note(zzub_plugin_t *plugin, int note, int prevNote, int velocity);
void zzub_plugin_play_pattern_row_ref(zzub_plugin_t *plugin, int pattern, int row);
void zzub_plugin_play_pattern_row(zzub_plugin_t *plugin, zzub_pattern_t* pattern, int row);

// 0.3: DEAD // int zzub_plugin_is_non_song_plugin(zzub_plugin_t *plugin);
// 0.3: DEAD // void zzub_plugin_set_non_song_plugin(zzub_plugin_t *plugin, int state);

// Connection methods

/*@}*/
/** @name Midi connection methods */
/*@{*/

int zzub_plugin_set_midi_connection_device(zzub_plugin_t *to_plugin, zzub_plugin_t* from_plugin, const char* name);
// 0.3: DEAD // const char* zzub_plugin_get_midi_connection_device(zzub_plugin_t *plugin, int from_plugin);

/*@}*/
/** @name Event connection methods */
/*@{*/

void zzub_plugin_add_event_connection_binding(zzub_plugin_t *plugin, zzub_plugin_t* from_plugin, int sourceparam, int targetgroup, int targettrack, int targetparam);
// 0.3: DEAD // int zzub_plugin_get_event_connection_binding_count(zzub_plugin_t *plugin, int from_plugin);
// 0.3: DEAD // zzub_event_connection_binding_t *zzub_plugin_get_event_connection_binding(zzub_plugin_t *plugin, int from_plugin, int index);
// 0.3: DEAD // int zzub_plugin_remove_event_connection_binding(zzub_plugin_t *plugin, int from_plugin, int index);

/*@}*/
/** @name Event connection binding methods */
/*@{*/

// 0.3: DEAD // int zzub_event_connection_binding_get_group(zzub_event_connection_binding_t *binding);
// 0.3: DEAD // int zzub_event_connection_binding_get_track(zzub_event_connection_binding_t *binding);
// 0.3: DEAD // int zzub_event_connection_binding_get_column(zzub_event_connection_binding_t *binding);
// 0.3: DEAD // int zzub_event_connection_binding_get_controller(zzub_event_connection_binding_t *binding);

/*@}*/
/** @name Sequencer methods */
/*@{*/

void zzub_sequence_create_track(zzub_player_t *player, zzub_plugin_t* plugin);
void zzub_sequence_remove_track(zzub_player_t *player, int index);
void zzub_sequence_move_track(zzub_player_t *player, int index, int newIndex);

int zzub_sequence_insert_events(zzub_player_t *player, int* track_indices, int num_indices, int start, int ticks);
int zzub_sequence_remove_events(zzub_player_t *player, int* track_indices, int num_indices, int start, int ticks);

void zzub_sequence_set_event(zzub_player_t *player, int track, int pos, int value);
// 0.3: DEAD // void zzub_sequence_set_events(zzub_player_t *player, int* events, int num_events);

zzub_plugin_t* zzub_sequence_get_plugin(zzub_player_t *player, int track);
int zzub_sequence_get_event_at(zzub_player_t *player, int track, unsigned long pos);

int zzub_sequence_get_event_count(zzub_player_t *player);
int zzub_sequence_get_event_timestamp(zzub_player_t *player, int index);
int zzub_sequence_get_event_action_count(zzub_player_t *player, int index);
int zzub_sequence_get_event_action(zzub_player_t *player, int index, int action, int* track, int* pos, int* value);

/*@}*/
/** @name Offline pattern methods 
	These functions are meant to help editing patterns. Note you cannot 
	retreive a direct zzub_pattern_t object for a "live pattern". You can 
	however, use zzub_plugin_get_pattern to retreive copies of live patterns,
	and then call zzub_plugin_update_pattern to write the changed pattern back
	to the engine. 
	Alternately, zzub_plugin_get_pattern_value/zzub_plugin_set_pattern_value
	can also be used to edit single values in live patterns.
*/
/*@{*/

/** \brief Creates a pattern compatible with given plugin. The pattern becomes incompatible if the plugin has tracks or incoming connections added. */
zzub_pattern_t *zzub_plugin_create_pattern(zzub_plugin_t *plugin, int rows);

/** \brief Creates a non-playable pattern with given columns and rows in group 0, track 0. All values are set to 0 by default. */
zzub_pattern_t *zzub_plugin_create_range_pattern(zzub_player_t *player, int columns, int rows);

void zzub_pattern_destroy(zzub_pattern_t *pattern);
void zzub_pattern_get_name(zzub_pattern_t *pattern, char* name, int maxLen);
void zzub_pattern_set_name(zzub_pattern_t *pattern, const char* name);
int zzub_pattern_get_row_count(zzub_pattern_t *pattern);
int zzub_pattern_get_group_count(zzub_pattern_t *pattern);
int zzub_pattern_get_track_count(zzub_pattern_t *pattern, int group);
int zzub_pattern_get_column_count(zzub_pattern_t *pattern, int group, int track);
int zzub_pattern_get_value(zzub_pattern_t* pattern, int row, int group, int track, int column);
void zzub_pattern_set_value(zzub_pattern_t* pattern, int row, int group, int track, int column, int value);
void zzub_pattern_interpolate(zzub_pattern_t* pattern);
//void zzub_pattern_get_bandwidth_digest(zzub_pattern_t* pattern, float *digest, int digestsize);

/*@}*/
/** @name Wave table */
/*@{*/

int zzub_wave_get_index(zzub_wave_t* wave);
int zzub_wave_load_sample(zzub_wave_t* wave, int level, int offset, int clear, const char* path, zzub_input_t* datastream);
int zzub_wave_save_sample(zzub_wave_t* wave, int level, zzub_output_t* datastream);
int zzub_wave_save_sample_range(zzub_wave_t* wave, int level, zzub_output_t* datastream, int start, int end); 
int zzub_wave_clear(zzub_wave_t* wave);
const char* zzub_wave_get_name(zzub_wave_t* wave);
void zzub_wave_set_name(zzub_wave_t* wave, const char* name);
const char* zzub_wave_get_path(zzub_wave_t* wave);
void zzub_wave_set_path(zzub_wave_t* wave, const char* path);
int zzub_wave_get_flags(zzub_wave_t* wave);
void zzub_wave_set_flags(zzub_wave_t* wave, int flags);
float zzub_wave_get_volume(zzub_wave_t* wave);
void zzub_wave_set_volume(zzub_wave_t* wave, float volume);
int zzub_wave_get_envelope_count(zzub_wave_t* wave);
void zzub_wave_set_envelope_count(zzub_wave_t* wave, int count);
zzub_envelope_t* zzub_wave_get_envelope(zzub_wave_t* wave, int index);
void zzub_wave_set_envelope(zzub_wave_t* wave, int index, zzub_envelope_t* env);
int zzub_wave_get_level_count(zzub_wave_t* wave);
zzub_wavelevel_t* zzub_wave_get_level(zzub_wave_t* wave, int index);

// 0.3: DEAD // void zzub_wavetable_add_level(zzub_player_t* player, int wave);
// 0.3: DEAD // void zzub_wavetable_remove_level(zzub_player_t* player, int wave, int level);

int zzub_wavelevel_get_index(zzub_wavelevel_t* wave);
zzub_wave_t* zzub_wavelevel_get_wave(zzub_wavelevel_t* level);
int zzub_wavelevel_clear(zzub_wavelevel_t* level);
int zzub_wavelevel_get_sample_count(zzub_wavelevel_t* level);
void zzub_wavelevel_set_sample_count(zzub_wavelevel_t* level, int count);
int zzub_wavelevel_get_root_note(zzub_wavelevel_t* level);
void zzub_wavelevel_set_root_note(zzub_wavelevel_t* level, int note);
int zzub_wavelevel_get_samples_per_second(zzub_wavelevel_t* level);
void zzub_wavelevel_set_samples_per_second(zzub_wavelevel_t* level, int sps);
int zzub_wavelevel_get_loop_start(zzub_wavelevel_t* level);
void zzub_wavelevel_set_loop_start(zzub_wavelevel_t* level, int pos);
int zzub_wavelevel_get_loop_end(zzub_wavelevel_t* level);
void zzub_wavelevel_set_loop_end(zzub_wavelevel_t* level, int pos);
int zzub_wavelevel_get_format(zzub_wavelevel_t* level);
// 0.3: DEAD // void zzub_wavetable_set_format(zzub_player_t* player, int wave, int level, int format);

void zzub_wavelevel_remove_sample_range(zzub_wavelevel_t* level, int start, int end);
void zzub_wavelevel_insert_sample_range(zzub_wavelevel_t* level, int start, void* buffer, int channels, int format, int numsamples);
void zzub_wavelevel_get_samples_digest(zzub_wavelevel_t* level, int channel, int start, int end, float *mindigest, float *maxdigest, float *ampdigest, int digestsize);


/*@}*/
/** @name Envelopes */
/*@{*/

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

 /** @name Memory and file streams - load/save from/to file/clipboard
	Create file or memory data streams for use by e.g 
	zzub_wavetable_load_sample()/zzub_wavetable_save_sample() and 
	zzub_player_load_bmx()/zzub_player_save_bmx().

	In-memory streams are implemented via the zzub_archive_t object
	and destroyed via zzub_archive_destroy().
	File-streams are created with zzub_input_open_file and zzub_output_create_file()
	and closed/destroyed with zzub_input_destroy() and zzub_output_destroy().

*/
/*@{*/

/** \brief Create an in-memory archive of keyed input and output streams. */
zzub_archive_t* zzub_archive_create_memory();

/** \brief Returns an output stream object for writing. */
zzub_output_t* zzub_archive_get_output(zzub_archive_t*, const char* path);

/** \brief Returns an input stream object for reading. */
zzub_input_t* zzub_archive_get_input(zzub_archive_t*, const char* path);

void zzub_archive_destroy(zzub_archive_t*);

/** \brief Create an output stream that writes to a file. */
zzub_output_t* zzub_output_create_file(const char* filename);

/** \brief Closes an output stream created with zzub_create_output_XXX. */
void zzub_output_destroy(zzub_output_t* f);

/** \brief Create an input stream that reads from a file. */
zzub_input_t* zzub_input_open_file(const char* filename);

/** \brief Closes an input stream created with zzub_create_output_XXX. */
void zzub_input_destroy(zzub_input_t* f);

void zzub_input_read(zzub_input_t* f, void* buffer, int bytes);
int zzub_input_size(zzub_input_t* f);
int zzub_input_position(zzub_input_t* f);
void zzub_input_seek(zzub_input_t* f, int, int);

void zzub_output_write(zzub_output_t* f, void* buffer, int bytes);
int zzub_output_position(zzub_output_t* f);
void zzub_output_seek(zzub_output_t* f, int, int);

/*@}*/

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // __LIBZZUB_H
