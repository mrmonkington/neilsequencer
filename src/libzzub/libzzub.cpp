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

#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <sndfile.h>

#include "common.h"
#include "ccm.h"
#include "zzub/zzub.h"

#include "driver_rtaudio.h"

#include "driver_silent.h"

struct zzub_flatapi_player : zzub::player {
  //zzub::audiodriver_rtaudio driver;
  zzub::mididriver _midiDriver;
  zzub_callback_t callback;
  void *callbackTag;
	
  std::vector<zzub_event_data_t> event_queue;
  int read_event_queue;
  int write_event_queue;
	
  zzub_event_data_t *pop_event() {
    if (read_event_queue == write_event_queue)
      return NULL;
    zzub_event_data_t *result = &event_queue[read_event_queue];
    read_event_queue++;
    if (read_event_queue == event_queue.size())
      read_event_queue = 0;
    return result;
  }
	
  void push_event(zzub_event_data_t &data) {
    event_queue[write_event_queue] = data;
    write_event_queue++;
    if (write_event_queue == event_queue.size())
      write_event_queue = 0;
    if (write_event_queue == read_event_queue) {
      std::cout << "warning: event queue overflow. need more calls to zzub_player_get_next_event()!" << std::endl;
    }
  }
	
  zzub_flatapi_player() {
    callback = 0;
    callbackTag = 0;
    event_queue.resize(4096);
    read_event_queue = 0;
    write_event_queue = 0;
    //driver.initialize(this);
    _midiDriver.initialize(this);
  }

};

#include "bmxreader.h"
#include "bmxwriter.h"
#include "recorder.h"
#include "archive.h"
#include "tools.h"

using namespace zzub;
using namespace std;

extern "C"
{

  /***

      Player methods

  ***/

  struct zzub_player_callback_all_events : event_handler {
    metaplugin_proxy* proxy;
    zzub_flatapi_player* player;

    std::map<int, event_handler*> handlers;
	
    zzub_player_callback_all_events(zzub_flatapi_player* _player, metaplugin_proxy* _proxy) {
      player = _player;
      proxy = _proxy;
    }

    virtual bool invoke(zzub_event_data_t& data) {
      // the master plugin checks for create/delete plugin events and maintains an
      // array of handlers who forward all events to the player callback.
      if (proxy->id == 0 && data.type == zzub_event_type_new_plugin) {
	metaplugin_proxy* new_plugin = data.new_plugin.plugin;
	zzub_player_callback_all_events *ev = new zzub_player_callback_all_events(player, new_plugin);
	handlers[new_plugin->id] = ev;
	player->front.plugins[new_plugin->id]->event_handlers.push_back(ev);
      } else
	if (proxy->id == 0 && data.type == zzub_event_type_pre_delete_plugin) {
	  metaplugin_proxy* del_plugin = data.delete_plugin.plugin;
	  map<int, event_handler*>::iterator i = handlers.find(del_plugin->id);
	  if (i != handlers.end()) {
	    delete i->second;
	    handlers.erase(i);
	  }
	}

      if (player->callback) {
	//int plugin = player->front.plugins[plugin_id]->descriptor;
	int res = player->callback(player, proxy, &data, player->callbackTag);
	if (!res)
	  return true;
      } else {
	player->push_event(data);
      }
      return false;
    }
  };
	

  zzub_player_t *zzub_player_create() {
    zzub_player_t *player = new zzub_flatapi_player();
    return player;
  }

  void zzub_player_blacklist_plugin(zzub_player_t *player, const char* uri) {
    //player->blacklist.push_back(uri);
  }

  void zzub_player_add_plugin_alias(zzub_player_t *player, const char* alias, const char* uri) {
    //player->aliases.insert(std::pair<std::string,std::string>(alias,uri));
    //player->aliases.push_back(mpa);
  }

  void zzub_player_add_plugin_path(zzub_player_t *player, const char* path) {
    if (!path) return ;
    player->plugin_folders.push_back(path);
  }

  int zzub_player_initialize(zzub_player_t *player, int samplesPerSec) {
    if (!player->initialize())
      return -1;	

    // NOTE: 0 == master
    zzub_player_callback_all_events *ev = new zzub_player_callback_all_events(player, player->front.plugins[0]->proxy);
    player->front.plugins[0]->event_handlers.push_back(ev);
    player->reset();
    return 0;
  }

  /*void zzub_player_begin(zzub_player_t *player) {
    player->begin_operation();
    }

    void zzub_player_end(zzub_player_t *player, zzub_event_data_t* redo_event, zzub_event_data_t* undo_event) {
    player->commit_operation(redo_event, undo_event);
    }*/


  void zzub_player_flush(zzub_player_t *player, zzub_event_data_t* redo_event, zzub_event_data_t* undo_event) {
    player->flush_operations(redo_event, redo_event, undo_event);
  }

  void zzub_player_destroy(zzub_player_t *player) {
    player->set_state(zzub::player_state_muted);
    player->clear();
    std::vector<event_handler*>& handlers = player->front.plugins[0]->event_handlers;
    assert(handlers.size() == 1);
    delete handlers.front();
    handlers.clear();
    delete player;
  }


  void zzub_player_undo(zzub_player_t *player) {
    player->undo();
  }

  void zzub_player_redo(zzub_player_t *player) {
    player->redo();
  }

  void zzub_player_history_commit(zzub_player_t *player, const char* description) {
    player->flush_operations(0, 0, 0);
    player->commit_to_history(description);
  }

  int zzub_player_history_get_uncomitted_operations(zzub_player_t *player) {
    return player->backbuffer_operations.size();
  }

  void zzub_player_history_flush(zzub_player_t *player) {
    player->flush_operations(0, 0, 0);
    player->clear_history();
  }

  void zzub_player_history_flush_last(zzub_player_t *player) {
    player->flush_operations(0, 0, 0);
    player->flush_from_history();
  }

  int zzub_player_history_get_size(zzub_player_t* player) {
    return (int)player->history.size();
  }

  int zzub_player_history_get_position(zzub_player_t* player) {
    return (int)(player->history_position - player->history.begin());
  }

  const char* zzub_player_history_get_description(zzub_player_t* player, int position) {
    return player->history[position].description.c_str();
  }

  int zzub_player_get_pluginloader_count(zzub_player_t *player) {
    return (int)player->plugin_infos.size();
  }

  zzub_plugincollection_t *zzub_plugincollection_get_by_uri(zzub_player_t *player, const char *uri) {
    return 0;//player->getPluginlibByUri(uri);
  }

  void zzub_plugincollection_configure(zzub_plugincollection_t *collection, const char *key, const char *value) {
    collection->collection->configure(key, value);
  }

  zzub_pluginloader_t *zzub_player_get_pluginloader(zzub_player_t *player, int index) {
    return player->plugin_infos[index];
  }

  zzub_pluginloader_t *zzub_player_get_pluginloader_by_name(zzub_player_t *player, const char* name) {
    if (!name) return 0;
    return player->plugin_get_info(name);
  }

  const int bmx_flag_ignore_patterns = 1;
  const int bmx_flag_ignore_sequences = 2;
  const int bmx_flag_ignore_waves = 4;

  int zzub_player_load_bmx(zzub_player_t *player, zzub_input_t* datastream, char* messages, int maxLen, int flags, float x, float y) {
    BuzzReader f(datastream);

    if (flags & bmx_flag_ignore_patterns) f.ignorePatterns = true;
    if (flags & bmx_flag_ignore_sequences) f.ignoreSequences = true;
    if (flags & bmx_flag_ignore_waves) f.ignoreWaves = true;
    f.offsetX = x;
    f.offsetY = y;

    bool result = f.readPlayer(player);
		
    if (maxLen > 0) {
      string messageText = f.lastError + f.lastWarning;
      strncpy(messages, messageText.c_str(), maxLen - 1);
    }
    if (!result) {
      cerr << "Errors:" << endl << f.lastError << endl << endl;
      cerr << "Warnings:" << endl << f.lastWarning << endl;

      return -1;
    }

    return 0;
  }

  int zzub_player_save_bmx(zzub_player_t *player, const zzub_plugin_t** _plugins, int num_plugins, int save_waves, zzub_output_t* datastream) {

    // incoming plugins are plugin_id's. bmxwriter takes plugin_descriptors, so lets remap
    std::vector<zzub::plugin_descriptor> plugins(num_plugins);
    for (int i = 0; i < num_plugins; i++) {
      const zzub_plugin_t* plugin = _plugins[i];
      metaplugin& m = *player->front.plugins[plugin->id];
      plugins[i] = m.descriptor;
    }
    BuzzWriter f(datastream);
    if (!f.writePlayer(player, plugins, save_waves?true:false)) {
      return -1;
    }
    return 0;
  }

  int zzub_player_load_ccm(zzub_player_t *player, const char* fileName) {
    CcmReader f;
    if (!f.open(fileName, player)) return -1;
    zzub_player_set_position(player, 0);
    return 0;
  }

  int zzub_player_save_ccm(zzub_player_t *player, const char* fileName) {
    CcmWriter f;
    if (!f.save(fileName, player)) return -1;
    return 0;
  }

  int zzub_player_get_state(zzub_player_t *player) {
    return (int)player->front.state;
  }

  void zzub_player_set_state(zzub_player_t *player, int state) {
    player->set_state((zzub::player_state)state);
    player->flush_operations(0, 0, 0);
  }

  int zzub_player_get_plugin_count(zzub_player_t *player) {

    operation_copy_flags flags;
    flags.copy_graph = true;
    player->merge_backbuffer_flags(flags);

    return player->back.get_plugin_count();
  }

  void zzub_player_get_new_plugin_name(zzub_player_t *player, const char* uri, char* name, int maxLen) {

    operation_copy_flags flags;
    flags.copy_plugins = true;
    flags.copy_graph = true;
    player->merge_backbuffer_flags(flags);

    std::string newname = player->plugin_get_new_name(uri);
    strncpy(name, newname.c_str(), maxLen);
  }

  zzub_plugin_t* zzub_player_get_plugin_by_name(zzub_player_t *player, const char* name) {

    operation_copy_flags flags;
    flags.copy_plugins = true;
    flags.copy_graph = true;
    player->merge_backbuffer_flags(flags);

    plugin_descriptor plugindesc = player->back.get_plugin_descriptor(name);
    if (plugindesc == graph_traits<plugin_map>::null_vertex()) return 0;
    int id = player->back.graph[plugindesc].id;
    return zzub_player_get_plugin_by_id(player, id);
  }

  zzub_plugin_t* zzub_player_get_plugin_by_id(zzub_player_t *player, int id) {

    operation_copy_flags flags;
    flags.copy_plugins = true;
    player->merge_backbuffer_flags(flags);

    return player->back.plugins[id]->proxy;
  }

  zzub_plugin_t* zzub_player_get_plugin(zzub_player_t *player, int index) {

    operation_copy_flags flags;
    flags.copy_graph = true;
    flags.copy_plugins = true;
    player->merge_backbuffer_flags(flags);

    int id = player->back.graph[index].id;
    return player->back.plugins[id]->proxy;
  }

  int zzub_plugin_set_midi_connection_device(zzub_plugin_t *to_plugin, zzub_plugin_t* from_plugin, const char* name) {

    to_plugin->_player->plugin_set_midi_connection_device(to_plugin->id, from_plugin->id, name);
    return 0;
  }

  void zzub_plugin_add_event_connection_binding(zzub_plugin_t *to_plugin, zzub_plugin_t* from_plugin, int sourceparam, int targetgroup, int targettrack, int targetparam) {

    to_plugin->_player->plugin_add_event_connection_binding(to_plugin->id, from_plugin->id, sourceparam, targetgroup, targettrack, targetparam);
  }

  const float** zzub_player_work_stereo(zzub_player_t *player, int* numSamples) {
    player->work_stereo(*numSamples);
    static const float* workBuffer[] = { player->work_out_buffer[0], player->work_out_buffer[1] };
    return workBuffer;
  }

  void zzub_player_clear(zzub_player_t *player) {
    player->clear();
  }

  int zzub_player_get_position(zzub_player_t *player) {
    return player->front.song_position;
  }

  void zzub_player_set_position(zzub_player_t *player, int pos) {
    player->set_play_position(pos);
    player->flush_operations(0, 0, 0);
  }

  int zzub_player_get_loop_start(zzub_player_t *player) {
    return player->front.song_loop_begin;
  }

  int zzub_player_get_loop_end(zzub_player_t *player) {
    return player->front.song_loop_end;
  }

  void zzub_player_get_loop(zzub_player_t *player, int *begin, int *end) {
    if (begin)
      *begin = player->front.song_loop_begin;
    if (end)
      *end = player->front.song_loop_end;
  }

  void zzub_player_set_loop(zzub_player_t *player, int begin, int end) {
    player->front.song_loop_begin = begin;
    player->front.song_loop_end = end;
  }

  int zzub_player_get_song_start(zzub_player_t *player) {
    return player->front.song_begin;
  }

  void zzub_player_set_loop_start(zzub_player_t *player, int v) {
    player->front.song_loop_begin = v;
  }

  void zzub_player_set_loop_end(zzub_player_t *player, int v) {
    player->front.song_loop_end = v;
  }

  void zzub_player_set_song_start(zzub_player_t *player, int v) {
    player->front.song_begin = v;
  }

  int zzub_player_get_song_end(zzub_player_t *player) {
    return player->front.song_end;
  }

  void zzub_player_set_song_end(zzub_player_t *player, int v) {
    player->front.song_end = v;
  }

  void zzub_player_set_loop_enabled(zzub_player_t *player, int enable) {
    player->front.song_loop_enabled = enable?true:false;
  }

  int zzub_player_get_loop_enabled(zzub_player_t *player) {
    return player->front.song_loop_enabled?1:0;
  }

  int zzub_player_get_sequence_track_count(zzub_player_t *player) {

    operation_copy_flags flags;
    flags.copy_sequencer_tracks = true;
    flags.copy_plugins = true;
    player->merge_backbuffer_flags(flags);

    return (int)player->back.sequencer_tracks.size();
  }

  zzub_sequence_t* zzub_player_get_sequence(zzub_player_t *player, int index) {

    operation_copy_flags flags;
    flags.copy_sequencer_tracks = true;
    player->merge_backbuffer_flags(flags);

    return player->back.sequencer_tracks[index].proxy;
  }


  int zzub_player_get_currently_playing_pattern(zzub_plugin_t *plugin, int* pattern, int* row) {
    // retreive player statistic from the front buffer
    if (plugin->_player->front.get_currently_playing_pattern(plugin->id, *pattern, *row))
      return 0;
    return -1;
  }

  int zzub_player_get_currently_playing_pattern_row(zzub_plugin_t *plugin, int pattern, int* row) {
    // retreive player statistic from the front buffer
    if (plugin->_player->front.get_currently_playing_pattern_row(plugin->id, pattern, *row))
      return 0;
    return -1;
  }

  int zzub_player_get_wave_count(zzub_player_t* player) {

    operation_copy_flags flags;
    flags.copy_wavetable = true;
    player->merge_backbuffer_flags(flags);

    return (int)player->back.wavetable.waves.size();
  }

  zzub_wave_t* zzub_player_get_wave(zzub_player_t* player, int index) {

    operation_copy_flags flags;
    flags.copy_wavetable = true;
    player->merge_backbuffer_flags(flags);

    if (index == -1) // monitor wave
      return player->back.wavetable.monitorwave.proxy;
    else {
      assert(index >= 0 && index < player->back.wavetable.waves.size());
      return player->back.wavetable.waves[index]->proxy;
    }
  }

  zzub_event_data_t *zzub_player_get_next_event(zzub_player_t *player) {
    return player->pop_event();
  }

  void zzub_player_set_callback(zzub_player_t* player, zzub_callback_t callback, void* tag) {
    // order is important here
    // once we set the callback, threaded calls might use it
    // so the tag should be available.
    player->callbackTag = tag;
    player->callback = callback;
  }

  void zzub_player_handle_events(zzub_player_t* player) {
    player->process_user_event_queue();
    //	player->update_plugins_load_snapshot();
  }

  void zzub_player_set_event_queue_state(zzub_player_t* player, int enable) {
    player->set_event_queue_state(enable);
  }

  zzub_midimapping_t *zzub_player_add_midimapping(zzub_plugin_t *plugin, int group, int track, int param, int channel, int controller) {


    plugin->_player->add_midimapping(plugin->id, group, track, param, channel, controller);
    return &plugin->_player->back.midi_mappings.back();
  }

  int zzub_player_remove_midimapping(zzub_plugin_t *plugin, int group, int track, int param) {


    plugin->_player->remove_midimapping(plugin->id, group, track, param);
    return 0;
  }

  zzub_midimapping_t *zzub_player_get_midimapping(zzub_player_t* player, int index) {

    operation_copy_flags flags;
    flags.copy_midi_mappings = true;
    player->merge_backbuffer_flags(flags);

    return &player->back.midi_mappings[index];
  }

  int zzub_player_get_midimapping_count(zzub_player_t* player) {

    operation_copy_flags flags;
    flags.copy_midi_mappings = true;
    player->merge_backbuffer_flags(flags);

    return player->back.midi_mappings.size();
  }

  int zzub_player_get_automation(zzub_player_t* player) {
    return (int)player->front.is_recording_parameters;
  }

  void zzub_player_set_automation(zzub_player_t* player, int enable) {
    player->front.is_recording_parameters = enable!=0?true:false;
  }

  int zzub_player_get_midi_transport(zzub_player_t* player) {
    return player->front.is_syncing_midi_transport;
  }

  void zzub_player_set_midi_transport(zzub_player_t* player, int enable) {
    player->front.is_syncing_midi_transport = enable!=0?true:false;
  }

  void zzub_player_reset_keyjazz(zzub_player_t *player) {
    player->reset_keyjazz();
  }

  const char *zzub_player_get_infotext(zzub_player_t *player) {
    return player->front.song_comment.c_str();
  }

  void zzub_player_set_infotext(zzub_player_t *player, const char *text) {
    player->front.song_comment = text;
  }

  void zzub_player_set_seqstep(zzub_player_t *player, int step) {
    player->front.seqstep = step;
  }

  int zzub_player_get_seqstep(zzub_player_t *player) {
    return player->front.seqstep;
  }

  float zzub_player_get_bpm(zzub_player_t *player) {
    return (float)zzub_plugin_get_parameter_value(zzub_player_get_plugin_by_id(player, 0), 1, 0, 1);
  }

  int zzub_player_get_tpb(zzub_player_t *player) {
    return zzub_plugin_get_parameter_value(zzub_player_get_plugin_by_id(player, 0), 1, 0, 2);
  }

  void zzub_player_set_bpm(zzub_player_t *player, float bpm) {
    zzub_plugin_set_parameter_value(zzub_player_get_plugin_by_id(player, 0), 1, 0, 1, (int)bpm, false);
  }

  void zzub_player_set_tpb(zzub_player_t *player, int tpb) {
    zzub_plugin_set_parameter_value(zzub_player_get_plugin_by_id(player, 0), 1, 0, 2, tpb, false);
  }

  void zzub_player_set_midi_plugin(zzub_player_t *player, zzub_plugin_t* plugin) {
    if (plugin == 0) {
      player->front.midi_plugin = -1;
    } else {
      if (plugin->id >= player->front.plugins.size() || player->front.plugins[plugin->id] == 0) return ;
      player->front.midi_plugin = plugin->id;
    }
  }

  zzub_plugin_t* zzub_player_get_midi_plugin(zzub_player_t *player) {
    if (player->front.midi_plugin == -1) return 0;
    return zzub_player_get_plugin_by_id(player, player->front.midi_plugin);
  }

  void zzub_player_set_host_info(zzub_player_t *player, int id, int version, void *host_ptr) {
    player->hostinfo.id = id;
    player->hostinfo.version = version;
    player->hostinfo.host_ptr = host_ptr;
  }

  // midimapping functions

  int zzub_midimapping_get_plugin(zzub_midimapping_t *mapping) {
    return (int)mapping->plugin_id;
  }

  int zzub_midimapping_get_group(zzub_midimapping_t *mapping) {
    return (int)mapping->group;
  }

  int zzub_midimapping_get_track(zzub_midimapping_t *mapping) {
    return (int)mapping->track;
  }

  int zzub_midimapping_get_column(zzub_midimapping_t *mapping) {
    return (int)mapping->column;
  }

  int zzub_midimapping_get_channel(zzub_midimapping_t *mapping) {
    return (int)mapping->channel;
  }

  int zzub_midimapping_get_controller(zzub_midimapping_t *mapping) {
    return (int)mapping->controller;
  }

  // PLuginloader methods

  const char *zzub_pluginloader_get_loader_name(zzub_pluginloader_t* loader)
  {
    return loader->uri.c_str();
  }

  const char *zzub_pluginloader_get_name(zzub_pluginloader_t* loader) {
    return loader->name.c_str();
  }

  const char *zzub_pluginloader_get_short_name(zzub_pluginloader_t* loader) {
    return loader->short_name.c_str();
  }

  int zzub_pluginloader_get_parameter_count(zzub_pluginloader_t* loader, int group) {
    switch (group) {
    case 0: // input connections
      return 0;
    case 1: // globals
      return (int)loader->global_parameters.size();
    case 2: // track params
      return (int)loader->track_parameters.size();
    case 3: // controller params
      return (int)loader->controller_parameters.size();
    default:
      return 0;
    }
  }

  zzub_parameter_t *zzub_pluginloader_get_parameter(zzub_pluginloader_t* loader, int group, int index) {
    switch (group) {
    case 0: // input connections
      return 0;//connectionParameters[index];
    case 1: // globals
      return (zzub_parameter_t*)loader->global_parameters[index];
    case 2: // track params
      return (zzub_parameter_t*)loader->track_parameters[index];
    case 3: // controller params
      return (zzub_parameter_t*)loader->controller_parameters[index];
    default:
      return 0;
    }
  }

  int zzub_pluginloader_get_flags(zzub_pluginloader_t* loader) {
    return loader->flags;
  }

  const char *zzub_pluginloader_get_uri(zzub_pluginloader_t* loader) {
    return loader->uri.c_str();
  }

  const char *zzub_pluginloader_get_author(zzub_pluginloader_t* loader) {
    return loader->author.c_str();
  }

  int zzub_pluginloader_get_attribute_count(zzub_pluginloader_t* loader) {
    return (int)loader->attributes.size();
  }

  zzub_attribute_t *zzub_pluginloader_get_attribute(zzub_pluginloader_t* loader, int index) {
    return (zzub_attribute_t *)loader->attributes[index];
  }

  int zzub_pluginloader_get_instrument_list(zzub_pluginloader_t* loader, char* result, int maxbytes) {
    if (loader->plugin_lib == 0) return 0;

    vector<char> outputBytes;
    zzub::mem_outstream outf(outputBytes);
    loader->plugin_lib->get_instrument_list(&outf);
    int size = outputBytes.size();
    if (size > 0)
      {
	if (size > maxbytes) size = maxbytes;
	memcpy(result, &outputBytes.front(), size);
      }
    result[size] = 0;
    return size;
  }

  int zzub_pluginloader_get_tracks_min(zzub_pluginloader_t* loader) {
    return loader->min_tracks;
  }

  int zzub_pluginloader_get_tracks_max(zzub_pluginloader_t* loader) {
    return loader->max_tracks;
  }

  int zzub_pluginloader_get_stream_format_count(zzub_pluginloader_t* loader) {
    return (int)loader->supported_stream_extensions.size();
  }

  const char* zzub_pluginloader_get_stream_format_ext(zzub_pluginloader_t* loader, int index) {
    return loader->supported_stream_extensions[index].c_str();
  }


  // parameter methods

  int zzub_parameter_get_type(zzub_parameter_t* param) {
    return param->type;
  }

  const char *zzub_parameter_get_name(zzub_parameter_t* param) {
    return param->name;
  }

  const char *zzub_parameter_get_description(zzub_parameter_t* param) {
    return param->description;
  }

  int zzub_parameter_get_value_min(zzub_parameter_t* param) {
    return param->value_min;
  }

  int zzub_parameter_get_value_max(zzub_parameter_t* param) {
    return param->value_max;
  }

  int zzub_parameter_get_value_none(zzub_parameter_t* param) {
    return param->value_none;
  }

  int zzub_parameter_get_value_default(zzub_parameter_t* param) {
    return param->value_default;
  }

  int zzub_parameter_get_flags(zzub_parameter_t* param) {
    return param->flags;
  }

  // attribute methods

  const char *zzub_attribute_get_name(zzub_attribute_t *attrib) {
    return attrib->name;
  }

  int zzub_attribute_get_value_min(zzub_attribute_t *attrib) {
    return attrib->value_min;
  }

  int zzub_attribute_get_value_max(zzub_attribute_t *attrib) {
    return attrib->value_max;
  }

  int zzub_attribute_get_value_default(zzub_attribute_t *attrib) {
    return attrib->value_default;
  }

  /***

      Machine methods

  ***/

  zzub_plugin_t* zzub_player_create_plugin(zzub_player_t *player, zzub_input_t *input, int dataSize, const char* name, zzub_pluginloader_t* loader, int flags) {
    std::vector<char> bytes(dataSize);
    if (dataSize > 0) input->read(&bytes.front(), dataSize);
    int plugin_id = player->create_plugin(bytes, name, const_cast<zzub::info*>(loader), flags);
    return player->back.plugins[plugin_id]->proxy;
  }

  int zzub_plugin_destroy(zzub_plugin_t *plugin) {

    plugin->_player->plugin_destroy(plugin->id);
    return 0;
  }


  /** \brief Load plugin state. */
  int zzub_plugin_load(zzub_plugin_t *plugin, zzub_input_t *input) {
    operation_copy_flags flags;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);

    mem_archive* arc = new mem_archive();
    outstream* outs = arc->get_outstream("");

    vector<char> bytes(input->size());
    input->read(&bytes.front(), (int)bytes.size());
    outs->write(&bytes.front(), (int)bytes.size());

    plugin->_player->back.plugins[plugin->id]->plugin->load(arc);

    delete arc;

    return -1;
  }

  /** \brief Save plugin state. */
  int zzub_plugin_save(zzub_plugin_t *plugin, zzub_output_t *ouput) {
    operation_copy_flags flags;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);

    mem_archive* arc = new mem_archive();
    plugin->_player->back.plugins[plugin->id]->plugin->save(arc);

    instream* ins = arc->get_instream("");
    if (ins && ins->size()) {
      vector<char> bytes(ins->size());
      ins->read(&bytes.front(), (int)bytes.size());
      ouput->write(&bytes.front(), (int)bytes.size());
    }

    delete arc;
    return -1;
  }


  void zzub_plugin_command(zzub_plugin_t *plugin, int i) {

    operation_copy_flags flags;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);

    plugin->_player->back.plugins[plugin->id]->plugin->command(i);
  }

  int zzub_plugin_set_name(zzub_plugin_t *plugin, const char* name) {

    plugin->_player->plugin_set_name(plugin->id, name);
    return 0;
  }

  int zzub_plugin_get_name(zzub_plugin_t *plugin, char* name, int maxlen) {
    operation_copy_flags flags;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);

    std::string s = plugin->_player->back.plugins[plugin->id]->name;
    strncpy(name, s.c_str(), maxlen);
    return (int)strlen(name);
  }

  void zzub_plugin_configure(zzub_plugin_t *plugin, const char *key, const char *value) {
    operation_copy_flags flags;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);

    plugin->_player->back.plugins[plugin->id]->plugin->configure(key, value);
  }

  int zzub_plugin_get_id(zzub_plugin_t* plugin) {
    return plugin->id;
  }

  int zzub_plugin_get_commands(zzub_plugin_t *plugin, char* commands, int maxlen) {

    operation_copy_flags flags;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);

    strncpy(commands, plugin->_player->back.plugins[plugin->id]->info->commands.c_str(), maxlen);
    return strlen(commands);
  }

  int zzub_plugin_get_sub_commands(zzub_plugin_t *plugin, int i, char* commands, int maxlen) {
    // if a command string starts with the char '\', it has subcommands
    // unexpectedly, this returns a \n-separated string (like getCommands())
    // some machines need to be ticked before calling getSubCommands (not yet supported)

    operation_copy_flags flags;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);

    vector<char> bytes;
    mem_outstream outm(bytes);
    outstream* outf = &outm;

    plugin->_player->back.plugins[plugin->id]->plugin->get_sub_menu(i, outf);
    outf->write((char)0);	// terminate array

    // create a new \n-separated string and return it instead, means both getCommands() and getSubCommands() return similar formatted strings
    const char* firstp = &bytes.front();
    string ret = "";

    while (*firstp) {
      if (ret.length() > 0)
	ret += "\n";
      ret += firstp;
      firstp += strlen(firstp)+1;
    }

    if (commands != strncpy(commands, ret.c_str(), maxlen)) {
      // too many bytes, clear string pls
      strcat(commands, "");
    }
    return strlen(commands);
  }

  int zzub_plugin_get_midi_output_device_count(zzub_plugin_t *plugin) {

    operation_copy_flags flags;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);

    static _midiouts midiouts;
    midiouts.clear();
    plugin->_player->back.plugins[plugin->id]->plugin->get_midi_output_names(&midiouts);
    return midiouts.names.size();
  }

  const char* zzub_plugin_get_midi_output_device(zzub_plugin_t *plugin, int index) {

    operation_copy_flags flags;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);

    static _midiouts midiouts;
    midiouts.clear();
    plugin->_player->back.plugins[plugin->id]->plugin->get_midi_output_names(&midiouts);
    return midiouts.names[index].c_str();
  }

  int zzub_plugin_get_envelope_count(zzub_plugin_t *plugin) {

    operation_copy_flags flags;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);

    const zzub::envelope_info** infos = plugin->_player->back.plugins[plugin->id]->plugin->get_envelope_infos();
    if (!infos) return 0;

    int count = 0;
    while (*infos) { count++; infos++; }
    return count;
  }

  const zzub::envelope_info* get_envelope_info(zzub_plugin_t *plugin, int index) {

    operation_copy_flags flags;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);

    const zzub::envelope_info** infos = plugin->_player->back.plugins[plugin->id]->plugin->get_envelope_infos();
    if (!infos) return 0;

    int count = 0;
    while (*infos && count < index) { count++; infos++; }
    return *infos;
  }

  int zzub_plugin_get_envelope_flags(zzub_plugin_t *plugin, int index) {
    const zzub::envelope_info* info = get_envelope_info(plugin, index);
    return info->flags;
  }

  const char* zzub_plugin_get_envelope_name(zzub_plugin_t *plugin, int index) {
    const zzub::envelope_info* info = get_envelope_info(plugin, index);
    return info->name;
  }

  void zzub_plugin_set_stream_source(zzub_plugin_t *plugin, const char* resource) {

    plugin->_player->plugin_set_stream_source(plugin->id, resource);
  }

  int zzub_plugin_get_flags(zzub_plugin_t *plugin) {

    operation_copy_flags flags;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);
    return plugin->_player->back.plugins[plugin->id]->flags;
  }

  zzub_pluginloader_t *zzub_plugin_get_pluginloader(zzub_plugin_t *plugin) {

    operation_copy_flags flags;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);
    return plugin->_player->back.plugins[plugin->id]->info;
  }

  void zzub_plugin_add_pattern(zzub_plugin_t *plugin, zzub_pattern_t *pattern) {


    plugin->_player->plugin_add_pattern(plugin->id, *pattern);
  }

  void zzub_plugin_remove_pattern(zzub_plugin_t *plugin, int pattern) {

    plugin->_player->plugin_remove_pattern(plugin->id, pattern);
  }

  void zzub_plugin_move_pattern(zzub_plugin_t *plugin, int index, int newIndex) {
    assert(false);
  }

  void zzub_plugin_update_pattern(zzub_plugin_t *plugin, int index, zzub_pattern_t* pattern) {
    plugin->_player->plugin_update_pattern(plugin->id, index, *pattern);
  }


  zzub_pattern_t *zzub_plugin_get_pattern(zzub_plugin_t *plugin, int index) {
    operation_copy_flags flags;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);
    return new zzub::pattern(*plugin->_player->back.plugins[plugin->id]->patterns[index]);
  }

  int zzub_plugin_get_pattern_index(zzub_plugin_t *plugin, zzub_pattern_t *pattern) {

    operation_copy_flags flags;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);

    for (size_t i = 0; i < plugin->_player->back.plugins[plugin->id]->patterns.size(); i++)
      if (plugin->_player->back.plugins[plugin->id]->patterns[i] == pattern) return (int)i;
    return -1;
  }


  int zzub_plugin_get_pattern_by_name(zzub_plugin_t *plugin, const char* name) {
    // copy necessary fields to the back buffer (if neccessary) and fetch the value from there
    // (note to self: separating read/write flags seems appropriate now)

    operation_copy_flags flags;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);
    for (size_t i = 0; i < plugin->_player->back.plugins[plugin->id]->patterns.size(); i++)
      if (plugin->_player->back.plugins[plugin->id]->patterns[i]->name == name) return (int)i;
    return -1;
  }

  int zzub_plugin_get_pattern_count(zzub_plugin_t *plugin) {

    operation_copy_flags flags;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);
    return (int)plugin->_player->back.plugins[plugin->id]->patterns.size();
  }

  const char* zzub_plugin_get_pattern_name(zzub_plugin_t *plugin, int index) {

    operation_copy_flags flags;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);
    return plugin->_player->back.plugins[plugin->id]->patterns[index]->name.c_str();
  }

  void zzub_plugin_set_pattern_name(zzub_plugin_t *plugin, int index, const char* name) {

    plugin->_player->plugin_set_pattern_name(plugin->id, index, name);
  }

  int zzub_plugin_get_pattern_length(zzub_plugin_t *plugin, int index) {

    operation_copy_flags flags;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);

    return plugin->_player->back.plugins[plugin->id]->patterns[index]->rows;
  }

  void zzub_plugin_set_pattern_length(zzub_plugin_t *plugin, int index, int rows) {

    plugin->_player->plugin_set_pattern_length(plugin->id, index, rows);
  }

  int zzub_plugin_get_pattern_value(zzub_plugin_t *plugin, int pattern, int group, int track, int column, int row) {

    operation_copy_flags flags;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);

    return plugin->_player->back.plugins[plugin->id]->patterns[pattern]->groups[group][track][column][row];
  }

  int zzub_plugin_get_parameter_count(zzub_plugin_t *plugin, int group, int track) {

    operation_copy_flags flags;
    flags.copy_graph = true;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);

    return plugin->_player->back.plugin_get_parameter_count(plugin->id, group, track);
  }

  zzub_parameter_t* zzub_plugin_get_parameter(zzub_plugin_t *plugin, int group, int track, int column) {

    operation_copy_flags flags;
    flags.copy_graph = true;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);

    return (zzub_parameter_t*)plugin->_player->back.plugin_get_parameter_info(plugin->id, group, track, column);
  }

  void zzub_plugin_set_pattern_value(zzub_plugin_t *plugin, int pattern, int group, int track, int column, int row, int value) {
    const zzub_parameter_t* param = zzub_plugin_get_parameter(plugin, group, track, column);
    assert((value >= param->value_min && value <= param->value_max) || value == param->value_none || (param->type == zzub::parameter_type_note && value == zzub::note_value_off));


    plugin->_player->plugin_set_pattern_value(plugin->id, pattern, group, track, column, row, value);
  }

  void zzub_plugin_insert_pattern_rows(zzub_plugin_t *plugin, int pattern, const int* column_indices, int num_indices, int start, int rows) {

    plugin->_player->plugin_insert_pattern_rows(plugin->id, pattern, (int*)column_indices, num_indices, start, rows);
  }

  void zzub_plugin_remove_pattern_rows(zzub_plugin_t *plugin, int pattern, const int* column_indices, int num_indices, int start, int rows) {

    plugin->_player->plugin_remove_pattern_rows(plugin->id, pattern, (int*)column_indices, num_indices, start, rows);
  }

  /*void zzub_plugin_set_pattern_values(zzub_player_t* player, int plugin, int pattern, int target_row, zzub_pattern_t* src_pattern, int* mappings, int mappings_count) {
    player->begin_operation();
    for (int i = 0; i < mappings_count; i++) {
    int source_group = mappings[i * 6 + 0];
    int source_track = mappings[i * 6 + 1];
    int source_column = mappings[i * 6 + 2];
    int target_group = mappings[i * 6 + 3];
    int target_track = mappings[i * 6 + 4];
    int target_column = mappings[i * 6 + 5];
    const zzub_parameter_t* target_param = zzub_plugin_get_parameter(player, plugin, target_group, target_track, target_column);
    for (int j = 0; j < zzub_pattern_get_row_count(src_pattern); j++) {
    if (target_row + j < 0) continue;
    if (target_row + j >= zzub_plugin_get_pattern_length(player, plugin, pattern)) break;
    int v = src_pattern->groups[source_group][source_track][source_column][j];
    if (v == -1) v = target_param->value_none;

    // value must be within the valid range
    if (v != target_param->value_none && v < target_param->value_min) v = target_param->value_min;
    if (v != target_param->value_none && v > target_param->value_max) v = target_param->value_max;

    player->plugin_set_pattern_value(plugin, pattern, target_group, target_track, target_column, target_row + j, v);
    }
    }
    player->commit_operation();
    }*/


  int zzub_plugin_get_parameter_value(zzub_plugin_t *plugin, int group, int track, int column) {

    operation_copy_flags flags;
    flags.copy_graph = true;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);

    return plugin->_player->back.plugin_get_parameter(plugin->id, group, track, column);
  }

  void zzub_plugin_set_parameter_value(zzub_plugin_t *plugin, int group, int track, int column, int value, int record) {
    // NOTE: users of zzub have no way to set a parameter with undo
    plugin->_player->plugin_set_parameter(plugin->id, group, track, column, value, record?true:false, false, false);
  }

  void zzub_plugin_set_parameter_value_direct(zzub_plugin_t *plugin, int group, int track, int column, int value, int record) {
    plugin->_player->plugin_set_parameter(plugin->id, group, track, column, value, record?true:false, true, false);
  }

  void zzub_plugin_get_position(zzub_plugin_t *plugin, float* x, float *y) {

    operation_copy_flags flags;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);

    *x = plugin->_player->back.plugins[plugin->id]->x;
    *y = plugin->_player->back.plugins[plugin->id]->y;
  }

  void zzub_plugin_set_position(zzub_plugin_t *plugin, float x, float y) {

    plugin->_player->plugin_set_position(plugin->id, x, y);
  }

  void zzub_plugin_set_position_direct(zzub_plugin_t *plugin, float x, float y) {

    operation_copy_flags flags;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);

    plugin->_player->back.plugins[plugin->id]->x = x;
    plugin->_player->back.plugins[plugin->id]->y = y;
  }

  int zzub_plugin_get_input_connection_count(zzub_plugin_t *plugin) {

    operation_copy_flags flags;
    flags.copy_graph = true;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);
	
    return plugin->_player->back.plugin_get_input_connection_count(plugin->id);
  }

  int zzub_plugin_get_input_connection_by_type(zzub_plugin_t *to_plugin, zzub_plugin_t* from_plugin, int type) {

    operation_copy_flags flags;
    flags.copy_graph = true;
    flags.copy_plugins = true;
    to_plugin->_player->merge_backbuffer_flags(flags);

    return to_plugin->_player->back.plugin_get_input_connection_index(to_plugin->id, from_plugin->id, (connection_type)type);
  }

  int zzub_plugin_get_input_connection_type(zzub_plugin_t *plugin, int index) {

    operation_copy_flags flags;
    flags.copy_graph = true;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);

    return plugin->_player->back.plugin_get_input_connection_type(plugin->id, index);
  }

  zzub_plugin_t* zzub_plugin_get_input_connection_plugin(zzub_plugin_t *plugin, int index) {

    operation_copy_flags flags;
    flags.copy_graph = true;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);

    int id = plugin->_player->back.plugin_get_input_connection_plugin(plugin->id, index);
    return plugin->_player->back.plugins[id]->proxy;
  }

  int zzub_plugin_get_output_connection_count(zzub_plugin_t *plugin) {

    operation_copy_flags flags;
    flags.copy_graph = true;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);

    return plugin->_player->back.plugin_get_output_connection_count(plugin->id);
  }

  int zzub_plugin_get_output_connection_by_type(zzub_plugin_t *to_plugin, zzub_plugin_t* from_plugin, int type) {

    operation_copy_flags flags;
    flags.copy_graph = true;
    flags.copy_plugins = true;
    to_plugin->_player->merge_backbuffer_flags(flags);

    return to_plugin->_player->back.plugin_get_output_connection_index(to_plugin->id, from_plugin->id, (connection_type)type);
  }

  int zzub_plugin_get_output_connection_type(zzub_plugin_t *plugin, int index) {

    operation_copy_flags flags;
    flags.copy_graph = true;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);

    return plugin->_player->back.plugin_get_output_connection_type(plugin->id, index);
  }

  zzub_plugin_t* zzub_plugin_get_output_connection_plugin(zzub_plugin_t *plugin, int index) {

    operation_copy_flags flags;
    flags.copy_graph = true;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);

    int id = plugin->_player->back.plugin_get_output_connection_plugin(plugin->id, index);
    return plugin->_player->back.plugins[id]->proxy;
  }

  int zzub_connection_get_parameter_count(zzub_player_t *player, int plugin, int from_plugin) {
    assert(false);
    return 0;
  }

  const zzub_parameter_t* zzub_connection_get_parameter(zzub_player_t *player, int plugin, int from_plugin, int index) {
    assert(false);
    return 0;
  }

  void zzub_plugin_get_last_peak(zzub_plugin_t *plugin, float *maxL, float *maxR) {
    *maxL = plugin->_player->front.plugins[plugin->id]->last_work_max_left;
    *maxR = plugin->_player->front.plugins[plugin->id]->last_work_max_right;
  }

  int zzub_plugin_add_input(zzub_plugin_t *to_plugin, zzub_plugin_t* from_plugin, int type) {

    bool result = to_plugin->_player->plugin_add_input(to_plugin->id, from_plugin->id, (zzub::connection_type)type);
    return result ? 0 : -1;
  }

  void zzub_plugin_delete_input(zzub_plugin_t *to_plugin, zzub_plugin_t* from_plugin, int type) {

    to_plugin->_player->plugin_delete_input(to_plugin->id, from_plugin->id, (zzub::connection_type)type);
  }

  int zzub_plugin_get_track_count(zzub_plugin_t *plugin) {

    operation_copy_flags flags;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);
    return plugin->_player->back.plugins[plugin->id]->tracks;
  }

  void zzub_plugin_set_track_count(zzub_plugin_t *plugin, int tracks) {

    plugin->_player->plugin_set_track_count(plugin->id, tracks);
  }

  int zzub_plugin_get_group_track_count(zzub_plugin_t *plugin, int group) {
    switch (group) {
    case zzub_parameter_group_connection: return zzub_plugin_get_input_connection_count(plugin);
    case zzub_parameter_group_global: return 1;
    case zzub_parameter_group_track: return zzub_plugin_get_track_count(plugin);
    default: return 0;
    }
    return 0;
  }

  int zzub_plugin_pattern_to_linear_no_connections(zzub_plugin_t *plugin, int group, int track, int column, int* index) {


    operation_copy_flags flags;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);

    const zzub::info* info = plugin->_player->back.plugins[plugin->id]->info;

    switch (group) {
    case 0:
      // should have been found already
      assert(false);
      return 0;
    case 1:
      *index = column;
      return 1;
    case 2:
      *index = info->global_parameters.size() + track * info->track_parameters.size() + column;
      return 1;
    case 3:
      return 0;
    default:
      assert(false);
      return 0;
    }
    //return player->plugin_pattern_to_linear(plugin, group, track, column, *index);
  }

  int zzub_plugin_describe_value(zzub_plugin_t *plugin, int group, int column, int value, char* name, int maxlen) {
    if (group == 0) {
      if (column == 0) {
	// buzz writes this as "-X.Y dB (Z%)"
	float dB = linear_to_dB((float)value / 0x4000) ;
	std::stringstream strm;
	if (dB > -100) 
	  strm << std::setprecision(2) << std::fixed << dB << " dB"; else
	  strm << "-inf dB";
	strm << " (" << (int)(((float)value / 0x4000) * 100) << "%)";
	strncpy(name, strm.str().c_str(), maxlen);
	return strlen(name);
      } else {
	std::stringstream strm;
	if (value == 0) 
	  strm << "Left"; else
	  if (value == 0x4000)
	    strm << "Center"; else
	    if (value == 0x8000)
	      strm << "Right"; else
	      strm << value - 0x4000;
	strncpy(name, strm.str().c_str(), maxlen);
	return strlen(name);
      }
    }
    if (group == 3) {
      strcpy(name, "");
      return 0;
    }


    operation_copy_flags flags;
    flags.copy_plugins = true;
    flags.copy_graph = true;
    plugin->_player->merge_backbuffer_flags(flags);

    int index = -1;
    zzub_plugin_pattern_to_linear_no_connections(plugin, group, 0, column, &index);

    const zzub::parameter* para = plugin->_player->back.plugin_get_parameter_info(plugin->id, group, 0, column);
    if (index != -1) {
      if (value != getNoValue(para)) {	// infector crashen when trying to describe novalues (and out-of-range-values)
			
	const char* str = plugin->_player->back.plugins[plugin->id]->plugin->describe_value(index, value);
	if (str != 0) {
	  strncpy(name, str, maxlen);
	  return strlen(str);
	}
      }
    }
    strcpy(name, "");
    return 0;
  }

  int zzub_plugin_get_mute(zzub_plugin_t *plugin) {
    if (plugin->id >= plugin->_player->front.plugins.size() || plugin->_player->front.plugins[plugin->id] == 0) return 0;
    return plugin->_player->front.plugins[plugin->id]->is_muted?1:0;
  }

  void zzub_plugin_set_mute(zzub_plugin_t *plugin, int muted) {
    if (plugin->id >= plugin->_player->front.plugins.size() || plugin->_player->front.plugins[plugin->id] == 0) return ;
    plugin->_player->front.plugins[plugin->id]->is_muted = muted?true:false;
  }

  int zzub_plugin_get_bypass(zzub_plugin_t *plugin) {
    if (plugin->id >= plugin->_player->front.plugins.size() || plugin->_player->front.plugins[plugin->id] == 0) return 0;
    return plugin->_player->front.plugins[plugin->id]->is_bypassed?1:0;
  }

  void zzub_plugin_set_bypass(zzub_plugin_t *plugin, int muted) {
    if (plugin->id >= plugin->_player->front.plugins.size() || plugin->_player->front.plugins[plugin->id] == 0) return ;
    plugin->_player->front.plugins[plugin->id]->is_bypassed = muted?true:false;
  }

  int zzub_plugin_invoke_event(zzub_plugin_t *plugin, zzub_event_data_t *data, int immediate) {
    assert(plugin->id < plugin->_player->front.plugins.size() && plugin->_player->front.plugins[plugin->id] != 0);
    return plugin->_player->front.plugin_invoke_event(plugin->id, *data, immediate?true:false)?0:-1;
  }

  double zzub_plugin_get_last_worktime(zzub_plugin_t *plugin) {
    if (plugin->id >= plugin->_player->front.plugins.size() || plugin->_player->front.plugins[plugin->id] == 0) return 0.0f;
    return plugin->_player->front.plugins[plugin->id]->last_work_time;
  }

  double zzub_plugin_get_last_cpu_load(zzub_plugin_t *plugin) {
    if (plugin->id >= plugin->_player->front.plugins.size() || plugin->_player->front.plugins[plugin->id] == 0) return 0.0f;
    return plugin->_player->front.plugins[plugin->id]->cpu_load;
  }

  int zzub_plugin_get_last_audio_result(zzub_plugin_t *plugin) {
    if (plugin->id >= plugin->_player->front.plugins.size() || plugin->_player->front.plugins[plugin->id] == 0) return 0;
    return plugin->_player->front.plugins[plugin->id]->last_work_audio_result?1:0;
  }

  int zzub_plugin_get_last_midi_result(zzub_plugin_t *plugin) {
    if (plugin->id >= plugin->_player->front.plugins.size() || plugin->_player->front.plugins[plugin->id] == 0) return 0;
    return plugin->_player->front.plugins[plugin->id]->last_work_midi_result?1:0;
  }

  void zzub_plugin_tick(zzub_plugin_t *plugin) {
    operation_copy_flags flags;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);

    op_plugin_process_events o(plugin->id);
    plugin->_player->backbuffer_operations.push_back(&o);
    o.prepare(plugin->_player->back);
    plugin->_player->flush_operations(0, 0, 0);
  }

  int zzub_plugin_get_attribute_value(zzub_plugin_t *plugin, int index) {

    operation_copy_flags flags;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);

    return plugin->_player->back.plugins[plugin->id]->plugin->attributes[index];//machine->getAttributeValue((size_t)index);
  }

  void zzub_plugin_set_attribute_value(zzub_plugin_t *plugin, int index, int value) {

    operation_copy_flags flags;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);

    metaplugin& m = *plugin->_player->back.plugins[plugin->id];
    m.plugin->attributes[index] = value;
    m.plugin->attributes_changed();
  }

  int zzub_plugin_get_mixbuffer(zzub_plugin_t *plugin, float *leftbuffer, float *rightbuffer, int *size, long long *samplepos) {
    assert(false);
    return -1;
  }

  void zzub_plugin_play_midi_note(zzub_plugin_t *plugin, int note, int prevNote, int velocity) {
    plugin->_player->play_plugin_note(plugin->id, note, prevNote, velocity);
  }

  void zzub_plugin_play_pattern_row_ref(zzub_plugin_t *plugin, int pattern, int row) {
    zzub_plugin_play_pattern_row(plugin, plugin->_player->front.plugins[plugin->id]->patterns[pattern], row);
  }

  void zzub_plugin_play_pattern_row(zzub_plugin_t *plugin, zzub_pattern_t* pattern, int row) {
    if (pattern->rows == 0) return ;

    operation_copy_flags flags;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);

    op_plugin_set_parameters_and_tick o(plugin->id, *pattern, row, false);
    plugin->_player->backbuffer_operations.push_back(&o);
    o.prepare(plugin->_player->back);
    plugin->_player->flush_operations(0, 0, 0);
    //player->execute_single_operation(&o);
  }


  int zzub_plugin_linear_to_pattern(zzub_plugin_t *plugin, int index, int* group, int* track, int* column) {
    operation_copy_flags flags;
    flags.copy_plugins = true;
    flags.copy_graph = true;
    plugin->_player->merge_backbuffer_flags(flags);

    const zzub::info* info = plugin->_player->back.plugins[plugin->id]->info;

    int numconnparams = 0;
    for (int i = 0; i < plugin->_player->back.plugin_get_input_connection_count(plugin->id); i++) {
      int numconnparamtrack = zzub_plugin_get_parameter_count(plugin, 0, i);
      if (index < numconnparams + numconnparamtrack) {
	*group = 0;
	*track = i;
	*column = index - numconnparams;
	return 1;
      }
      numconnparams += numconnparamtrack;
    }

    index -= numconnparams;

    if (index < (int)info->global_parameters.size()) {
      *group = 1;
      *track = 0;
      *column = index;
      return 1;
    }

    index -= info->global_parameters.size();
	
    if (!info->track_parameters.size()) return 0;

    int t = index / info->track_parameters.size();
    if (t >= plugin->_player->back.plugins[plugin->id]->tracks) return 0;

    *group = 2;
    *track = t;
    *column = index % info->track_parameters.size();
    return 1;
  }

  int zzub_plugin_pattern_to_linear(zzub_plugin_t *plugin, int group, int track, int column, int* index) {
    operation_copy_flags flags;
    flags.copy_plugins = true;
    flags.copy_graph = true;
    plugin->_player->merge_backbuffer_flags(flags);

    const zzub::info* info = plugin->_player->back.plugins[plugin->id]->info;

    int numconnparams = 0;
    for (int i = 0; i < plugin->_player->back.plugin_get_input_connection_count(plugin->id); i++) {
      int numconnparamtrack = zzub_plugin_get_parameter_count(plugin, 0, i);

      if (group == 0 && track == i) {
	*index = numconnparams + column;
	return 1;
      }

      numconnparams += numconnparamtrack;
    }

    switch (group) {
    case 0:
      // should have been found already
      return 0;
    case 1:
      *index = numconnparams + column;
      return 1;
    case 2:
      *index = numconnparams + info->global_parameters.size() + track * info->track_parameters.size() + column;
      return 1;
    case 3:
      return 0;
    default:
      assert(false);
      return 0;
    }
  }


  int zzub_plugin_get_pattern_column_count(zzub_plugin_t *plugin) {

    operation_copy_flags flags;
    flags.copy_graph = true;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);

    const zzub::info* info = plugin->_player->back.plugins[plugin->id]->info;

    int numconnparams = 0;
    for (int i = 0; i < plugin->_player->back.plugin_get_input_connection_count(plugin->id); i++)
      numconnparams += zzub_plugin_get_parameter_count(plugin, 0, i);

    return numconnparams + info->global_parameters.size() + info->track_parameters.size() * plugin->_player->back.plugins[plugin->id]->tracks;
  }

  int zzub_plugin_set_instrument(zzub_plugin_t *plugin, const char *name) {

    operation_copy_flags flags;
    flags.copy_plugins = true;
    plugin->_player->merge_backbuffer_flags(flags);

    return plugin->_player->back.plugins[plugin->id]->plugin->set_instrument(name) ? 0 : -1;
  }

  /***

      Sequencer methods

  ***/

  int zzub_sequence_get_event_at(zzub_sequence_t* sequence, int pos) {
    int song_index = 0;
    int timestamp, value;
    int event_count = zzub_sequence_get_event_count(sequence);
    while (event_count > 0 && song_index < event_count) 
      {
	zzub_sequence_get_event(sequence, song_index, &timestamp, &value);
	if (pos <= timestamp) break;
	song_index++;
      }

    if (song_index >= event_count || pos != timestamp) return -1;

    return value;
  }

  void zzub_sequence_set_event(zzub_sequence_t* sequence, int timestamp, int value) {

    sequence->_player->sequencer_set_event(sequence->track, timestamp, value);
  }

  int zzub_sequence_get_event_count(zzub_sequence_t* sequence) {

    operation_copy_flags flags;
    flags.copy_sequencer_tracks = true;
    sequence->_player->merge_backbuffer_flags(flags);

    return (int)sequence->_player->back.sequencer_tracks[sequence->track].events.size();
  }

  int zzub_sequence_get_event(zzub_sequence_t* sequence, int index, int* pos, int* value) {
    operation_copy_flags flags;
    flags.copy_sequencer_tracks = true;
    sequence->_player->merge_backbuffer_flags(flags);
	
    sequence_event& ev = sequence->_player->back.sequencer_tracks[sequence->track].events[index];
    *pos = ev.time;
    *value = ev.pattern_event.value;
    return 0;
  }

  zzub_plugin_t* zzub_sequence_get_plugin(zzub_sequence_t* sequence) {

    operation_copy_flags flags;
    flags.copy_sequencer_tracks = true;
    flags.copy_plugins = true;
    sequence->_player->merge_backbuffer_flags(flags);

    int id = sequence->_player->back.sequencer_tracks[sequence->track].plugin_id;
    return sequence->_player->back.plugins[id]->proxy;
  }

  zzub_sequence_t* zzub_player_create_sequence(zzub_player_t *player, zzub_plugin_t* plugin, int type) {
    player->sequencer_add_track(plugin->id, (sequence_type)type);
    return player->back.sequencer_tracks.back().proxy;
  }

  int zzub_sequence_get_type(zzub_sequence_t* sequence) {
    operation_copy_flags flags;
    flags.copy_sequencer_tracks = true;
    sequence->_player->merge_backbuffer_flags(flags);

    return (int)sequence->_player->back.sequencer_tracks[sequence->track].type;
  }

  void zzub_sequence_destroy(zzub_sequence_t* sequence) {

    sequence->_player->sequencer_remove_track(sequence->track);
  }

  void zzub_sequence_move(zzub_sequence_t* sequence, int newIndex) {

    sequence->_player->sequencer_move_track(sequence->track, newIndex);
  }

  int zzub_sequence_insert_events(zzub_sequence_t* sequence, int start, int ticks) {

    sequence->_player->sequencer_insert_events(sequence->track, start, ticks);
    return 0;
  }

  int zzub_sequence_remove_events(zzub_sequence_t* sequence, int start, int ticks) {

    sequence->_player->sequencer_remove_events(sequence->track, start, ticks);
    return 0;
  }

  //	Pattern methods

  zzub_pattern_t *zzub_plugin_create_pattern(zzub_plugin_t *plugin, int rows) {
    // copy necessary fields to back buffer and create pattern there
    operation_copy_flags flags;
    flags.copy_graph = true;
    flags.copy_plugins = true;

    operation_copy_plugin_flags plugin_flags;
    plugin_flags.copy_plugin = true;
    plugin_flags.plugin_id = plugin->id;
    flags.plugin_flags.push_back(plugin_flags);
    plugin->_player->merge_backbuffer_flags(flags);

    zzub::pattern* p = new zzub::pattern();
    plugin->_player->back.create_pattern(*p, plugin->id, rows);
    return p;
  }

  zzub_pattern_t *zzub_plugin_create_range_pattern(zzub_player_t *player, int columns, int rows) {
    zzub::pattern* p = new zzub::pattern();
    p->groups.resize(1);
    p->groups[0].resize(1);
    p->groups[0][0].resize(columns);
    for (size_t i = 0; i < p->groups[0][0].size(); i++) {
      p->groups[0][0][i].resize(rows);
    }
    p->rows = rows;
    return p;

  }

  void zzub_pattern_destroy(zzub_pattern_t *pattern) {
    delete pattern;
  }

  void zzub_plugin_get_new_pattern_name(zzub_plugin_t *plugin, char* name, int maxLen) {
    std::stringstream strm;
    for (int i = zzub_plugin_get_pattern_count(plugin); i < 9999; i++) {
      if (i < 100)
	strm << std::setw(2) << std::setfill('0') << i; else
	if (i < 1000)
	  strm << std::setw(3) << std::setfill('0') << i; else
	  if (i < 10000)
	    strm << std::setw(4) << std::setfill('0') << i; else
	    if (i < 100000)
	      strm << std::setw(5) << std::setfill('0') << i;

      int p = zzub_plugin_get_pattern_by_name(plugin, const_cast<char*>(strm.str().c_str()));
      if (p != -1) {
	strm.str("");
	continue;
      } else {
	break;
      }
    }
    strncpy(name, strm.str().c_str(), maxLen);
  }

  void zzub_pattern_get_name(zzub_pattern_t *pattern, char* name, int maxLen) {
    string str = pattern->name;
    strncpy(name, str.c_str(), maxLen);
  }

  void zzub_pattern_set_name(zzub_pattern_t *pattern, const char* name) {
    pattern->name = name;
  }

  int zzub_pattern_get_row_count(zzub_pattern_t *pattern) {
    return pattern->rows;
  }

  int zzub_pattern_get_group_count(zzub_pattern_t *pattern) {
    return (int)pattern->groups.size();
  }

  int zzub_pattern_get_track_count(zzub_pattern_t *pattern, int group) {
    return (int)pattern->groups[group].size();
  }

  int zzub_pattern_get_column_count(zzub_pattern_t *pattern, int group, int track) {
    return (int)pattern->groups[group][track].size();
  }

  int zzub_pattern_get_value(zzub_pattern_t* pattern, int row, int group, int track, int column) {
    return pattern->groups[group][track][column][row];
  }

  void zzub_pattern_set_value(zzub_pattern_t* pattern, int row, int group, int track, int column, int value) {
    pattern->groups[group][track][column][row] = value;
  }

  void zzub_pattern_interpolate(zzub_pattern_t* pattern) {
    int num_rows = zzub_pattern_get_row_count(pattern);
    int num_cols = zzub_pattern_get_column_count(pattern, 0, 0);
    int tr, tv, br, bv;
    int i, j;
    float increment;

    for (i = 0; i < num_cols; i++) {
      tr = tv = br = bv = -1;

      // Find top-most row and value
      for (j = 0; j < num_rows; j++) {
	tv = zzub_pattern_get_value(pattern, j, 0, 0, i);
	if (tv != -1) {
	  tr = j;
	  break;
	}
      }
		
      if (tr == -1)
	continue;

      // Find bottom-most row and value
      for (j = num_rows - 1; j > tr + 1; j--) {
	bv = zzub_pattern_get_value(pattern, j, 0, 0, i);
	if (bv != -1) {
	  br = j;
	  break;
	}
      }
      if (br == -1)
	continue;

      increment = bv - tv;
      increment /= (br - tr);

      // Interpolate this column
      if(increment) {
	for(float value = tv, j = tr + 1; j < br; j++) {
	  value += increment;
	  zzub_pattern_set_value(pattern, j, 0, 0, i, int(value));
	}
      }
    }
  }

  void zzub_pattern_get_bandwidth_digest(zzub_pattern_t* pattern, float *digest, int digestsize) {
    /*
      float row = 0;
      int rowcount = zzub_pattern_get_row_count(pattern);
      // rows per digest sample
      float rps = (float)rowcount / (float)digestsize;
      for (int i = 0; i < digestsize; ++i) {
      int total = 0;
      int count = 0;
      float rowend = std::min(row + rps, (float)rowcount);
      for (int r = (int)row; r < (int)rowend; r++) {
      size_t trackcount = pattern->getPatternTracks();
      for (size_t t = 0; t < trackcount; ++t) {
      patterntrack* track = pattern->getPatternTrack(t);
      size_t paramcount = track->getParams();
      for (size_t p = 0; p < paramcount; ++p) {
      total += 1;
      int value = track->getValue(r, p);
      const parameter *param = track->getParam(p);
      if (value != param->value_none)
      count += 1;
      }				
      }
      }
      if (total)
      digest[i] = float(count) / float(total);
      else
      digest[i] = 0.0f;
      row = rowend;
      }
    */
  }

#ifndef TRUE
#define TRUE 0
#endif

#ifndef FALSE
#define FALSE 0
#endif

  using namespace std;

  /***

      Audio Driver methods

  ***/


  /** \brief Create an audio driver that uses the PortAudio API. */
  zzub_audiodriver_t* zzub_audiodriver_create_portaudio(zzub_player_t* player) {
    return 0;
  }

  /** \brief Create an audio driver that uses the RtAudio API. */
  zzub_audiodriver_t* zzub_audiodriver_create_rtaudio(zzub_player_t* player) {
    audiodriver_rtaudio* driver = new audiodriver_rtaudio();
    driver->initialize(player);
    return driver;
  }

  /** \brief Create a silent, non-processing audio driver that has one device with the specified properties. */
  zzub_audiodriver_t* zzub_audiodriver_create_silent(zzub_player_t* player, const char* name, int out_channels, int in_channels, int* supported_rates, int num_rates) {
    audiodriver_silent* driver = new audiodriver_silent();
    driver->device.name = name;
    driver->device.out_channels = out_channels;
    driver->device.in_channels = in_channels;
    driver->device.rates.assign(supported_rates, supported_rates + num_rates);
    driver->initialize(player);
    return driver;
  }

  /** \brief Creates the preferred audio driver. */
  zzub_audiodriver_t* zzub_audiodriver_create(zzub_player_t* player) {
    {
      zzub_audiodriver_t* d = zzub_audiodriver_create_rtaudio(player);
      if (d) return d;
    }
    {
      zzub_audiodriver_t* d = zzub_audiodriver_create_portaudio(player);
      if (d) return d;
    }
    return 0;
  }

  int zzub_audiodriver_get_count(zzub_audiodriver_t* driver) {
    return driver->getDeviceCount();
  }

  int zzub_audiodriver_get_name(zzub_audiodriver_t* driver, int index, char* name, int maxLen) {
    audiodevice* device = driver->getDeviceInfo(index);
    if (!device) {
      strcpy(name, "");
    } else {
      strncpy(name, device->name.c_str(), maxLen);
    }
    return (int)strlen(name);
  }

  int zzub_audiodriver_create_device(zzub_audiodriver_t* driver, int input_index, int output_index) {
    return driver->createDevice(output_index, input_index)?0:-1;
  }

  void zzub_audiodriver_enable(zzub_audiodriver_t* driver, int state) {
    driver->enable(state?true:false);
  }

  int zzub_audiodriver_get_enabled(zzub_audiodriver_t* driver) {
    return driver->worker->work_started?1:0;
  }

  void zzub_audiodriver_destroy(zzub_audiodriver_t* driver) {
    driver->destroyDevice();
    delete driver;
  }

  /** \brief De-allocate the current device. */
  void zzub_audiodriver_destroy_device(zzub_audiodriver_t* driver) {
    driver->destroyDevice();
  }

  void zzub_audiodriver_set_samplerate(zzub_audiodriver_t* driver, unsigned int samplerate) {
    driver->samplerate = samplerate;
  }

  unsigned int zzub_audiodriver_get_samplerate(zzub_audiodriver_t* driver) {
    return driver->samplerate;
  }

  void zzub_audiodriver_set_buffersize(zzub_audiodriver_t* driver, unsigned int buffersize) {
    driver->buffersize = buffersize;
  }

  unsigned int zzub_audiodriver_get_buffersize(zzub_audiodriver_t* driver) {
    return driver->buffersize;
  }

  int zzub_audiodriver_get_master_channel(zzub_audiodriver_t* driver) {
    return driver->master_channel;
  }

  void zzub_audiodriver_set_master_channel(zzub_audiodriver_t* driver, int index) {
    driver->master_channel = index;
  }

  double zzub_audiodriver_get_cpu_load(zzub_audiodriver_t* driver) {
    return driver->getCpuLoad();
  }

  int zzub_audiodriver_is_output(zzub_audiodriver_t* driver, int index) {
    audiodevice* device = driver->getDeviceInfo(index);
    return device && device->out_channels>0;
  }

  int zzub_audiodriver_is_input(zzub_audiodriver_t* driver, int index) {
    audiodevice* device = driver->getDeviceInfo(index);
    return device && device->in_channels>0;
  }

  int zzub_audiodriver_get_supported_samplerates(zzub_audiodriver_t* driver, int index, int* result, int maxrates) {
    audiodevice* device = driver->getDeviceInfo(index);
    if (maxrates > device->rates.size()) maxrates = device->rates.size();
    std::copy(device->rates.begin(), device->rates.begin() + maxrates, result);
    return device->rates.size();
  }

  int zzub_audiodriver_get_supported_output_channels(zzub_audiodriver_t* driver, int index) {
    audiodevice* device = driver->getDeviceInfo(index);
    return device->out_channels;
  }

  int zzub_audiodriver_get_supported_input_channels(zzub_audiodriver_t* driver, int index) {
    audiodevice* device = driver->getDeviceInfo(index);
    return device->in_channels;
  }

  // midi driver

  int zzub_mididriver_get_count(zzub_player_t *player) {
    return (int)player->_midiDriver.getDevices();
  }

  const char *zzub_mididriver_get_name(zzub_player_t *player, int index) {
    return player->_midiDriver.getDeviceName(index);
  }

  int zzub_mididriver_is_input(zzub_player_t *player, int index) {
    return player->_midiDriver.isInput(index)?1:0;
  }

  int zzub_mididriver_is_output(zzub_player_t *player, int index) {
    return player->_midiDriver.isOutput(index)?1:0;
  }
  int zzub_mididriver_open(zzub_player_t *player, int index) {
    return player->_midiDriver.openDevice(index)?0:-1;
  }

  int zzub_mididriver_close_all(zzub_player_t *player) {
    return player->_midiDriver.closeAllDevices()?0:-1;
  }

  // Wave table

  int zzub_wave_get_index(zzub_wave_t* wave) {
    return wave->wave;
  }

  const char* zzub_wave_get_name(zzub_wave_t* wave) {
    operation_copy_flags flags;
    flags.copy_wavetable = true;
    wave->_player->merge_backbuffer_flags(flags);
    return wave->_player->back.wavetable.waves[wave->wave]->name.c_str();
  }

  void zzub_wave_set_name(zzub_wave_t* wave, const char* name) {
    wave->_player->wave_set_name(wave->wave, name);
  }

  const char* zzub_wave_get_path(zzub_wave_t* wave) {
    operation_copy_flags flags;
    flags.copy_wavetable = true;
    wave->_player->merge_backbuffer_flags(flags);
    return wave->_player->back.wavetable.waves[wave->wave]->fileName.c_str();
  }

  void zzub_wave_set_path(zzub_wave_t* wave, const char* path) {
    wave->_player->wave_set_path(wave->wave, path);
  }

  int zzub_wave_get_flags(zzub_wave_t* wave) {
    operation_copy_flags flags;
    flags.copy_wavetable = true;
    wave->_player->merge_backbuffer_flags(flags);
    return wave->_player->back.wavetable.waves[wave->wave]->flags;
  }

  void zzub_wave_set_flags(zzub_wave_t* wave, int flags) {
    wave->_player->wave_set_flags(wave->wave, flags);
  }

  float zzub_wave_get_volume(zzub_wave_t* wave) {
    operation_copy_flags flags;
    flags.copy_wavetable = true;
    wave->_player->merge_backbuffer_flags(flags);
    return wave->_player->back.wavetable.waves[wave->wave]->volume;
  }

  void zzub_wave_set_volume(zzub_wave_t* wave, float volume) {
    wave->_player->wave_set_volume(wave->wave, volume);
  }

  int zzub_wave_load_sample(zzub_wave_t* wave, int level, int offset, 
			    int clear, const char* path, 
			    zzub_input_t* datastream) {
    bool result = true;
    int loaded_samples = wave->_player->wave_load_sample(wave->wave, level, offset, clear != 0, path, datastream);
    return loaded_samples;
  }

  void zzub_wavelevel_remove_sample_range(zzub_wavelevel_t* level, int start, int end) {
    level->_player->wave_remove_samples(level->wave, level->level, start, end - start + 1);
  }

  void zzub_wave_insert_sample_range(zzub_player_t* player, int wave, int level, int start, void* buffer, int channels, int format, int numsamples) {
    assert(false);
  }

  int zzub_wavelevel_clear(zzub_wavelevel_t* level) {
    level->_player->wave_clear_level(level->wave, level->level);
    return 0;
  }

  zzub_wave_t* zzub_wavelevel_get_wave(zzub_wavelevel_t* level) {
    operation_copy_flags flags;
    flags.copy_wavetable = true;
    level->_player->merge_backbuffer_flags(flags);

    return level->_player->back.wavetable.waves[level->wave]->proxy;
  }

  int zzub_wave_clear(zzub_wave_t* wave) {
    wave->_player->wave_clear(wave->wave);
    return 0;
  }

  int zzub_wave_get_level_count(zzub_wave_t* wave) {

    operation_copy_flags flags;
    flags.copy_wavetable = true;
    wave->_player->merge_backbuffer_flags(flags);

    return wave->_player->back.wavetable.waves[wave->wave]->levels.size();
  }

  zzub_wavelevel_t* zzub_wave_get_level(zzub_wave_t* wave, int index) {

    operation_copy_flags flags;
    flags.copy_wavetable = true;
    wave->_player->merge_backbuffer_flags(flags);

    return wave->_player->back.wavetable.waves[wave->wave]->levels[index].proxy;
  }

  int zzub_wave_get_envelope_count(zzub_wave_t* wave) {

    operation_copy_flags flags;
    flags.copy_wavetable = true;
    wave->_player->merge_backbuffer_flags(flags);

    return wave->_player->back.wavetable.waves[wave->wave]->envelopes.size();
  }

  void zzub_wave_set_envelope_count(zzub_wave_t* wave, int count) {

    operation_copy_flags flags;
    flags.copy_wavetable = true;
    wave->_player->merge_backbuffer_flags(flags);

    vector<envelope_entry> envelopes = wave->_player->back.wavetable.waves[wave->wave]->envelopes;
    if (count >= envelopes.size()) {
      for (int i = envelopes.size(); i < count; i++) {
	envelopes.push_back(envelope_entry());
      }
    } else
      if (count < envelopes.size())
	envelopes.erase(envelopes.begin() + count, envelopes.end());

    wave->_player->wave_set_envelopes(wave->wave, envelopes);

  }

  zzub_envelope_t* zzub_wave_get_envelope(zzub_wave_t* wave, int index) {

    operation_copy_flags flags;
    flags.copy_wavetable = true;
    wave->_player->merge_backbuffer_flags(flags);

    assert(index >= 0 && index < wave->_player->back.wavetable.waves[wave->wave]->envelopes.size());
    return &wave->_player->back.wavetable.waves[wave->wave]->envelopes[index];
  }

  void zzub_wave_set_envelope(zzub_wave_t* wave, int index, zzub_envelope_t* env) {

    operation_copy_flags flags;
    flags.copy_wavetable = true;
    wave->_player->merge_backbuffer_flags(flags);

    vector<envelope_entry> envelopes = wave->_player->back.wavetable.waves[wave->wave]->envelopes;
    assert(index >= 0 && index < envelopes.size());
    envelopes[index] = *env;
    wave->_player->wave_set_envelopes(wave->wave, envelopes);

  }

  int zzub_wavelevel_get_sample_count(zzub_wavelevel_t* level) {

    operation_copy_flags flags;
    flags.copy_wavetable = true;
    level->_player->merge_backbuffer_flags(flags);

    return level->_player->back.wavetable.waves[level->wave]->levels[level->level].sample_count;
  }

  void zzub_wavelevel_set_sample_count(zzub_wavelevel_t* level, int count) {
    assert(false);
  }

  int zzub_wavelevel_get_root_note(zzub_wavelevel_t* level) {
    operation_copy_flags flags;
    flags.copy_wavetable = true;
    level->_player->merge_backbuffer_flags(flags);
    return level->_player->back.wavetable.waves[level->wave]->levels[level->level].root_note;
  }

  void zzub_wavelevel_set_root_note(zzub_wavelevel_t* level, int note) {
    level->_player->wave_set_root_note(level->wave, level->level, note);
  }

  int zzub_wavelevel_get_samples_per_second(zzub_wavelevel_t* level) {
    operation_copy_flags flags;
    flags.copy_wavetable = true;
    level->_player->merge_backbuffer_flags(flags);
    return level->_player->back.wavetable.waves[level->wave]->levels[level->level].samples_per_second;
  }

  void zzub_wavelevel_set_samples_per_second(zzub_wavelevel_t* level, int sps) {
    level->_player->wave_set_samples_per_second(level->wave, level->level, sps);
  }

  int zzub_wavelevel_get_loop_start(zzub_wavelevel_t* level) {
    operation_copy_flags flags;
    flags.copy_wavetable = true;
    level->_player->merge_backbuffer_flags(flags);
    return level->_player->back.wavetable.waves[level->wave]->levels[level->level].loop_start;
  }

  void zzub_wavelevel_set_loop_start(zzub_wavelevel_t* level, int pos) {
    level->_player->wave_set_loop_begin(level->wave, level->level, pos);
  }

  int zzub_wavelevel_get_loop_end(zzub_wavelevel_t* level) {
    operation_copy_flags flags;
    flags.copy_wavetable = true;
    level->_player->merge_backbuffer_flags(flags);
    return level->_player->back.wavetable.waves[level->wave]->levels[level->level].loop_end;
  }

  void zzub_wavelevel_set_loop_end(zzub_wavelevel_t* level, int pos) {
    level->_player->wave_set_loop_end(level->wave, level->level, pos);
  }

  int zzub_wavelevel_get_format(zzub_wavelevel_t* level) {
    operation_copy_flags flags;
    flags.copy_wavetable = true;
    level->_player->merge_backbuffer_flags(flags);
    return level->_player->back.wavetable.waves[level->wave]->levels[level->level].format;
  }

  static sf_count_t outstream_filelen (void *user_data) {
    zzub::outstream* strm = (zzub::outstream*)user_data ;
    int pos = strm->position();
    strm->seek(0, SEEK_END);
    int size = strm->position();
    strm->seek(pos, SEEK_SET);
    return size;
  }

  static sf_count_t outstream_seek (sf_count_t offset, int whence, void *user_data) {
    zzub::outstream* strm = (zzub::outstream*)user_data ;
    strm->seek(offset, whence);
    return strm->position();
  }

  static sf_count_t outstream_read (void *ptr, sf_count_t count, void *user_data){
    zzub::outstream* strm = (zzub::outstream*)user_data ;
    assert(false);
    return 0;
  }

  static sf_count_t outstream_write (const void *ptr, sf_count_t count, void *user_data) {
    zzub::outstream* strm = (zzub::outstream*)user_data ;
    return strm->write((void*)ptr, count);
  }

  static sf_count_t outstream_tell (void *user_data){
    zzub::outstream* strm = (zzub::outstream*)user_data ;
    return strm->position();
  }

  int zzub_wave_save_sample_range(zzub_wave_t* wave, int level, zzub_output_t* datastream, int start, int end) {
    operation_copy_flags flags;
    flags.copy_wavetable = true;
    wave->_player->merge_backbuffer_flags(flags);

    wave_info_ex& w = *wave->_player->back.wavetable.waves[wave->wave];
    if (level < 0 || level >= w.levels.size()) return -1;

    wave_level_ex& l = w.levels[level];

    int channels = (w.flags & wave_flag_stereo) ?2:1;
    int result = -1;
    SF_INFO sfinfo;
    memset(&sfinfo, 0, sizeof(sfinfo));
    sfinfo.samplerate = l.samples_per_second;
    sfinfo.channels = channels;
    sfinfo.format = SF_FORMAT_WAV;
    switch (l.format) {
    case wave_buffer_type_si16:
      sfinfo.format |= SF_FORMAT_PCM_16;
      break;
    case wave_buffer_type_si24:
      sfinfo.format |= SF_FORMAT_PCM_24;
      break;
    case wave_buffer_type_si32:
      sfinfo.format |= SF_FORMAT_PCM_32;
      break;
    case wave_buffer_type_f32:
      sfinfo.format |= SF_FORMAT_FLOAT;
      break;
    default:
      return -1;
    }

    SF_VIRTUAL_IO vio;
    vio.get_filelen = outstream_filelen ;
    vio.seek = outstream_seek;
    vio.read = outstream_read;
    vio.write = outstream_write;
    vio.tell = outstream_tell;
    SNDFILE* sf = sf_open_virtual(&vio, SFM_WRITE, &sfinfo, datastream);

    if (!sf)
      return -1;
    //	sf_writef_short(sf, (short*)w.get_sample_ptr(level, start), end - start);
    int bytes_per_sample = l.get_bytes_per_sample();
    char* sample_ptr = (char*)l.samples;
    sample_ptr += start * channels * bytes_per_sample;
    sf_write_raw(sf, sample_ptr, (end - start) * channels * bytes_per_sample);
    sf_close(sf); // so close it
    return 0;
  }

  int zzub_wave_save_sample(zzub_wave_t* wave, int level, zzub_output_t* datastream) {

    operation_copy_flags flags;
    flags.copy_wavetable = true;
    wave->_player->merge_backbuffer_flags(flags);

    wave_info_ex& w = *wave->_player->back.wavetable.waves[wave->wave];
    return zzub_wave_save_sample_range(wave, level, datastream, 0, w.get_sample_count(level));
  }

  void zzub_wavelevel_xfade(zzub_wavelevel_t* level, int start, int end) {
    assert((end - start) < start);
    wave_info_ex &w = *level->_player->back.wavetable.waves[level->wave];
    wave_level_ex &l = level->_player->back.wavetable.waves[level->wave]->levels[level->level];
    unsigned char *samples = (unsigned char *)l.samples;
    int channels = w.get_stereo() ? 2 : 1;
    int bps = l.get_bytes_per_sample();
    int bitsps = bps * 8;
    wave_buffer_type format = (wave_buffer_type)l.format;
    // Find the peak in the samples buffer.
    int isample1 = 0;
    int isample2 = 0;
    float dfade = 1.0 / (end - start);
    for (int channel = 0; channel < channels; channel++) {
      float fade1 = 0.0;
      float fade2 = 1.0;
      for (int i = start; i < end; i++) {
	int offset1 = ((i - end + start) * channels + channel) * bps;
	int offset2 = (i * channels + channel) * bps;
	switch (bps) {
	case 1: 
	  isample1 = *(char *)&samples[offset1];
	  isample2 = *(char *)&samples[offset2]; 
	  *(char *)&samples[offset2] = isample1 * fade1 + isample2 * fade2;
	  break;
	case 2: 
	  isample1 = *(short *)&samples[offset1]; 
	  isample2 = *(short *)&samples[offset2];
	  *(short *)&samples[offset2] = isample1 * fade1 + isample2 * fade2;
	  break;
	case 4: 
	  isample1 = *(int *)&samples[offset1];
	  isample2 = *(int *)&samples[offset2];
	  *(int *)&samples[offset2] = isample1 * fade1 + isample2 * fade2;
	  break;
	}
	fade1 += dfade;
	fade2 -= dfade;
      }
    }
    l.loop_start = start;
    l.loop_end = end;
    w.flags = w.flags | zzub_wave_flag_loop;
  }


  void zzub_wavelevel_normalize(zzub_wavelevel_t* level) {
    wave_info_ex &w = *level->_player->back.wavetable.waves[level->wave];
    wave_level_ex &l = level->_player->back.wavetable.waves[level->wave]->levels[level->level];
    unsigned char *samples = (unsigned char *)l.samples;
    int channels = w.get_stereo() ? 2 : 1;
    int bps = l.get_bytes_per_sample();
    int bitsps = bps * 8;
    wave_buffer_type format = (wave_buffer_type)l.format;
    float scaler = 1.0 / (1 << (bitsps - 1));
    // Find the peak in the samples buffer.
    int isample = 0;
    int max_sample = 0;
    float normalizer = 1.0;
    for (int j = 0; j < 2; j++) {
      for (int channel = 0; channel < channels; channel++) {
	for (int i = 0; i < l.sample_count; i++) {
	  int offset = (i * channels + channel) * bps;
	  switch (bps) {
	  case 1: 
	    isample = *(char *)&samples[offset]; 
	    *(char *)&samples[offset] = isample * normalizer;
	    break;
	  case 2: 
	    isample = *(short *)&samples[offset]; 
	    *(short *)&samples[offset] = isample * normalizer;
	    break;
	  case 4: 
	    isample = *(int *)&samples[offset];
	    *(int *)&samples[offset] = isample * normalizer;
	    break;
	  }
	  if (isample > abs(max_sample)) {
	    max_sample = abs(isample);
	  }
	}
	normalizer = (0.5 * pow(2.0, bitsps)) / float(max_sample);
      }
    }
  }

  void zzub_wavelevel_get_samples_digest(zzub_wavelevel_t* level, int channel, int start, int end, float *mindigest, float *maxdigest, float *ampdigest, int digestsize) {

    operation_copy_flags flags;
    flags.copy_wavetable = true;
    level->_player->merge_backbuffer_flags(flags);

    wave_info_ex& w = *level->_player->back.wavetable.waves[level->wave];
    wave_level_ex& l = level->_player->back.wavetable.waves[level->wave]->levels[level->level];

    unsigned char *samples = (unsigned char*)l.samples;
    int bitsps = l.get_bytes_per_sample() * 8;
    int bps = l.get_bytes_per_sample();
    wave_buffer_type format = (wave_buffer_type)l.format;
    float scaler = 1.0f / (1<<(bitsps-1));
    int samplecount = l.sample_count;
    int samplerange = end - start;
    assert((start >= 0) && (start < samplecount));
    assert((end > start) && (end <= samplecount));
    assert(samplerange > 0);
    int channels = w.get_stereo()?2:1;
    float sps = (float)samplerange / (float)digestsize; // samples per sample
    float blockstart = (float)start;
    if (sps > 1)
      {
	for (int i = 0; i < digestsize; ++i) {
	  float blockend = std::min(blockstart + sps, (float)end);
	  float minsample = 1.0f;
	  float maxsample = -1.0f;
	  float amp = 0.0f;
	  for (int s = (int)blockstart; s < (int)blockend; ++s) {
	    int offset = (s*channels + channel)*bps;
	    int isample = 0;
	    switch (bps) {
	    case 1: isample = *(char*)&samples[offset]; break;
	    case 2: isample = *(short*)&samples[offset]; break;
	    case 3: 
	      isample = *(int*)&samples[offset] & 0x00ffffff; 
	      if (isample & 0x00800000) isample = isample | 0xFF000000;
	      break; // TODO
	    case 4: 
	      switch (format) {
	      case wave_buffer_type_si32:
		isample = *(int*)&samples[offset];
		break;
	      case wave_buffer_type_f32:
		isample = (int)((*(float*)&samples[offset]) / scaler);
		break;
	      }
	      break;
	    }
	    float sample = (float)(isample) * scaler;
	    minsample = std::min(minsample, sample);
	    maxsample = std::max(maxsample, sample);
	    amp += sample*sample;
	  }
	  if (mindigest)
	    mindigest[i] = minsample;
	  if (maxdigest)
	    maxdigest[i] = maxsample;
	  if (ampdigest)
	    ampdigest[i] = sqrtf(amp / (blockend - blockstart));
	  blockstart = blockend;
	}
      }
    else
      {
	for (int i = 0; i < digestsize; ++i) {
	  int s = (int)(blockstart + i * sps /* + 0.5f */);
	  int offset = (s*channels + channel)*bps;
	  int isample = 0;
	  switch (bps) {
	  case 1: isample = *(char*)&samples[offset]; break;
	  case 2: isample = *(short*)&samples[offset]; break;
	  case 3: isample = *(int*)&samples[offset]; break; // TODO
	  case 4: isample = *(int*)&samples[offset]; break;
	  }
	  float sample = (float)(isample) * scaler;
	  if (mindigest)
	    mindigest[i] = sample;
	  if (maxdigest)
	    maxdigest[i] = sample;
	  if (ampdigest)
	    ampdigest[i] = std::abs(sample);
	}
      }
  }

  /*int zzub_wavelevel_get_sample_count(zzub_wavelevel_t * level) {
    return level->wave->get_sample_count(level->level); 
    }

    void *zzub_wavelevel_get_samples(zzub_wavelevel_t * level) {
    return level->wave->get_sample_ptr(level->level);
    }

    int zzub_wavelevel_get_root_note(zzub_wavelevel_t * level) {
    return level->wave->get_root_note(level->level); 
    }

    int zzub_wavelevel_get_samples_per_second(zzub_wavelevel_t * level) {
    return level->wave->get_samples_per_sec(level->level); 
    }

    int zzub_wavelevel_get_loop_start(zzub_wavelevel_t * level) {
    return level->wave->get_loop_start(level->level);
    }

    int zzub_wavelevel_get_loop_end(zzub_wavelevel_t * level) {
    return level->wave->get_loop_end(level->level);
    }

    void zzub_wave_set_path(zzub_wave_t* wave, const char *path) {
    wave->fileName = path;
    }

    void zzub_wave_set_name(zzub_wave_t* wave, const char *name) {
    wave->name = name;
    }
    void zzub_wave_set_flags(zzub_wave_t* wave, int flags) {
    wave->flags = flags;
    }

    void zzub_wave_set_volume(zzub_wave_t* wave, float volume) {
    wave->volume = volume;
    }

    int zzub_wave_get_envelope_count(zzub_wave_t* wave) {
    return wave->envelopes.size();
    }

    zzub_envelope_t *zzub_wave_get_envelope(zzub_wave_t* wave, int index) {
    // create envelopes that do not exist!
    if (index >= wave->envelopes.size()) {
    int numNewEnvs = index - wave->envelopes.size() + 1;
    for (int i = 0; i < numNewEnvs; i++) {
    wave->envelopes.push_back(envelope_entry());
    }
    }
    return &wave->envelopes[index];
    }

    void zzub_wavelevel_set_root_note(zzub_wavelevel_t * level, int rootnote) {
    level->wave->set_root_note(level->level, rootnote);
    }
    void zzub_wavelevel_set_samples_per_second(zzub_wavelevel_t * level, int samplespersecond) {
    level->wave->set_samples_per_sec(level->level, samplespersecond);
    }
    void zzub_wavelevel_set_loop_start(zzub_wavelevel_t * level, int loopstart) {
    level->wave->set_loop_start(level->level,loopstart);
    }
    void zzub_wavelevel_set_loop_end(zzub_wavelevel_t * level, int loopend) {
    level->wave->set_loop_end(level->level, loopend);
    }

    int zzub_wavelevel_silence_range(zzub_wavelevel_t * level, int start, int end) {
    return level->wave->silence_wave_range(level->level, (size_t)start, (size_t)(end - start))?0:-1;
    }

    int zzub_wavelevel_remove_range(zzub_wavelevel_t * level, int start, int end) {
    return level->wave->remove_wave_range(level->level, (size_t)start, (size_t)(end - start))?0:-1;
    }

    int zzub_wavelevel_stretch_range(zzub_wavelevel_t * level, int start, int end, int newsize) {
    return level->wave->stretch_wave_range(level->level, (size_t)start, (size_t)(end - start), (size_t)newsize)?0:-1;
    }

    int zzub_wavelevel_insert(zzub_wavelevel_t * level, int start, void* sampleData, int channels, int waveFormat, int numSamples) {
    return level->wave->insert_wave_at(level->level, (size_t)start, sampleData, (size_t)channels, waveFormat, (size_t)numSamples)?0:-1;
    }

    int zzub_wavelevel_get_format(zzub_wavelevel_t * level) {
    return level->wave->get_wave_format(level->level);
    }

    int zzub_wavelevel_get_slice_count(zzub_wavelevel_t *level) {
    return level->slices.size();
    }
    int zzub_wavelevel_get_slice_value(zzub_wavelevel_t *level, int index) {
    if ((index < 0) || ((size_t)index >= level->slices.size()))
    return -1;
    return level->slices[index];
    }
    int zzub_wavelevel_clear_slices(zzub_wavelevel_t *level) {
    level->slices.clear();
    return 0;
    }
    int zzub_wavelevel_add_slice(zzub_wavelevel_t *level, int value) {
    level->slices.push_back(value);
    return 0;
    }*/

  // envelopes

  unsigned short zzub_envelope_get_attack(zzub_envelope_t *env) {
    return env->attack;
  }

  unsigned short zzub_envelope_get_decay(zzub_envelope_t *env) {
    return env->decay;
  }

  unsigned short zzub_envelope_get_sustain(zzub_envelope_t *env) {
    return env->sustain;
  }

  unsigned short zzub_envelope_get_release(zzub_envelope_t *env) {
    return env->release;
  }

  void zzub_envelope_set_attack(zzub_envelope_t *env, unsigned short attack) {
    env->attack = attack;
  }

  void zzub_envelope_set_decay(zzub_envelope_t *env, unsigned short decay) {
    env->decay = decay;
  }

  void zzub_envelope_set_sustain(zzub_envelope_t *env, unsigned short sustain) {
    env->sustain = sustain;
  }

  void zzub_envelope_set_release(zzub_envelope_t *env, unsigned short release) {
    env->release = release;
  }

  char zzub_envelope_get_subdivision(zzub_envelope_t *env) {
    return env->subDivide;
  }

  void zzub_envelope_set_subdivision(zzub_envelope_t *env, char subdiv) {
    env->subDivide = subdiv;
  }

  char zzub_envelope_get_flags(zzub_envelope_t *env) {
    return env->flags;
  }

  void zzub_envelope_set_flags(zzub_envelope_t *env, char flags) {
    env->flags = flags;
  }

  int zzub_envelope_is_enabled(zzub_envelope_t *env) {
    return (!env->disabled)?1:0;
  }

  void zzub_envelope_enable(zzub_envelope_t *env, int enable) {
    env->disabled = !enable;
  }

  int zzub_envelope_get_point_count(zzub_envelope_t *env) {
    return env->points.size();
  }

  void zzub_envelope_get_point(zzub_envelope_t *env, int index, unsigned short *x, unsigned short *y, char *flags) {
    envelope_point *pt = &env->points[index];
    if (x)
      *x = pt->x;
    if (y)
      *y = pt->y;
    if (flags)
      *flags = pt->flags;
  }

  void zzub_envelope_set_point(zzub_envelope_t *env, int index, unsigned short x, unsigned short y, char flags) {
    envelope_point *pt = &env->points[index];
    if (index == 0)
      {
	pt->x = 0;
      }
    else if (index == (env->points.size()-1))
      {
	pt->x = 65535;
      }
    else
      {
	envelope_point *ptprev = &env->points[index-1];
	envelope_point *ptnext = &env->points[index+1];
	pt->x = std::min(std::max(x,(unsigned short)(ptprev->x+1)), (unsigned short)(ptnext->x-1));
      }
    pt->y = y;
    pt->flags = flags;
  }

  void zzub_envelope_insert_point(zzub_envelope_t *env, int index) {
    index = std::max(std::min(index, (int)(env->points.size()-1)), 1); // must never insert before the first or after the last pt
    std::vector<envelope_point>::iterator pos = env->points.begin() + index;
    envelope_point pt = *pos;
    pt.flags = 0;
    env->points.insert(pos, pt);
  }

  void zzub_envelope_delete_point(zzub_envelope_t *env, int index) {
    index = std::max(std::min(index, (int)(env->points.size()-1)), 1); // must never remove before the first or after the last pt
    std::vector<envelope_point>::iterator pos = env->points.begin() + index;
    env->points.erase(pos);
  }

  zzub_archive_t* zzub_archive_create_memory() {
    return new mem_archive();
  }

  zzub_output_t* zzub_archive_get_output(zzub_archive_t* a, const char* path) {
    return a->get_outstream(path);
  }

  zzub_input_t* zzub_archive_get_input(zzub_archive_t* a, const char* path) {
    return a->get_instream(path);
  }

  void zzub_archive_destroy(zzub_archive_t* a) {
    delete a;
  }

  zzub_output_t* zzub_output_create_file(const char* filename) {
    zzub::file_outstream* fout = new zzub::file_outstream();
    if (!fout->create(filename)) {
      delete fout;
      return 0;
    }
    return fout;
  }

  void zzub_output_destroy(zzub_output_t* fout) {
    // NOTE: unchecked cast!
    zzub::file_outstream* fstrm = (zzub::file_outstream*)fout;
    fstrm->close();
    delete fout;
  }

  zzub_input_t* zzub_input_open_file(const char* filename) {
    zzub::file_instream* fout = new zzub::file_instream();
    if (!fout->open(filename)) {
      delete fout;
      return 0;
    }
    return fout;
  }

  void zzub_input_destroy(zzub_input_t* inf) {
    // NOTE: unchecked cast!
    zzub::file_instream* fstrm = (zzub::file_instream*)inf;
    fstrm->close();
    delete inf;
  }

  void zzub_input_read(zzub_input_t* f, char* buffer, int bytes) {
    f->read(buffer, bytes);
  }

  int zzub_input_size(zzub_input_t* f) {
    return f->size();
  }

  int zzub_input_position(zzub_input_t* f) {
    return f->position();
  }

  void zzub_input_seek(zzub_input_t* f, int a, int b) {
    f->seek(a, b);
  }

  void zzub_output_write(zzub_output_t* f, const char* buffer, int bytes) {
    f->write((void*)buffer, bytes);
  }

  int zzub_output_position(zzub_output_t* f) {
    return f->position();
  }

  void zzub_output_seek(zzub_output_t* f, int a, int b) {
    f->seek(a, b);
  }

} // extern "C"
