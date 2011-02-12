/*
  Copyright (C) 2003-2008 Anders Ervik <calvin@countzero.no>

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

#include "common.h"
#include <functional>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <sstream>
#include "dummy.h"
#include "archive.h"
#include "tools.h"

#include <sndfile.h>

#include "import.h"

#include <dirent.h>
#include <sys/stat.h>

#include "sseoptimization.h"

using namespace std;

extern size_t sizeFromWaveFormat(int waveFormat);

struct find_info_by_uri : public std::unary_function<const zzub::info*, bool> {
  std::string uri;
  find_info_by_uri(std::string u) {
    uri = u;
  }
  bool operator()(const zzub::info* info) {
    return strcmpi(uri.c_str(), info->uri.c_str()) == 0;
  }
};

namespace zzub {
  /***

      waveimporter

  ***/
  static sf_count_t instream_filelen (void *user_data) {
    zzub::instream* strm = (zzub::instream*)user_data ;
    return strm->size();
  }

  static sf_count_t instream_seek (sf_count_t offset, int whence, void *user_data) {
    zzub::instream* strm = (zzub::instream*)user_data ;
    strm->seek((long)offset, (int)whence);
    return strm->position();
  }

  static sf_count_t instream_read (void *ptr, sf_count_t count, void *user_data){
    zzub::instream* strm = (zzub::instream*)user_data ;
    return strm->read(ptr, count);
  }

  static sf_count_t instream_write (const void *ptr, sf_count_t count, void *user_data) {
    zzub::instream* strm = (zzub::instream*)user_data ;
    assert(false);
    _unused(strm);
    return 0;
  }

  static sf_count_t instream_tell (void *user_data){
    zzub::instream* strm = (zzub::instream*)user_data ;
    return strm->position();
  }


  import_sndfile::import_sndfile() {
    sf = 0;
    memset(&sfinfo, 0, sizeof(sfinfo));
  }

  bool import_sndfile::open(zzub::instream* strm) {
    memset(&sfinfo, 0, sizeof(sfinfo));
    SF_VIRTUAL_IO vio;
    vio.get_filelen = instream_filelen ;
    vio.seek = instream_seek;
    vio.read = instream_read;
    vio.write = instream_write;
    vio.tell = instream_tell;
    sf = sf_open_virtual(&vio, SFM_READ, &sfinfo, strm);

    if (!sf || !sfinfo.frames) {
      sf_close(sf);
      sf = 0;
      return false;
    }

    return true;
  }

  int import_sndfile::get_wave_count() {
    if (sf != 0) return 1;
    return 0;
  }

  int import_sndfile::get_wave_level_count(int i) {
    assert(i == 0);
    if (sf != 0) return 1;
    return 0;
  }

  bool import_sndfile::get_wave_level_info(int i, int level, importwave_info& info) {
    assert(i == 0);
    assert(level == 0);
    if (i != 0 && level != 0) return false;
    info.channels = sfinfo.channels;

    switch (sfinfo.format & SF_FORMAT_SUBMASK) {
    case SF_FORMAT_PCM_U8: // convert anything 8 bit to 16-bit
    case SF_FORMAT_PCM_S8:
    case SF_FORMAT_PCM_16:
      info.format = wave_buffer_type_si16;
      break;
    case SF_FORMAT_PCM_24:
      info.format = wave_buffer_type_si24;
      break;
    case SF_FORMAT_PCM_32:
      info.format = wave_buffer_type_si32;
      break;
    case SF_FORMAT_FLOAT:
      info.format = wave_buffer_type_f32;
      break;
    default:
      return false;
    }
    info.sample_count = (int)sfinfo.frames;
    info.samples_per_second = sfinfo.samplerate;
    return true;
  }

  void import_sndfile::read_wave_level_samples(int i, int level, void* buffer) {
    assert(i == 0);
    assert(level == 0);
    importwave_info iwi;
    if (!get_wave_level_info(i, level, iwi)) return ;

    // TODO: this could use a larger buffer
    for (int i = 0; i < iwi.sample_count; i++) {
      float f[2];
      sf_read_float(sf, f, iwi.channels);
      CopySamples(&f, buffer, 1, wave_buffer_type_f32, iwi.format, 1, 1, 0, i * iwi.channels);
      if (iwi.channels == 2) 
	CopySamples(&f, buffer, 1, wave_buffer_type_f32, iwi.format, 1, 1, 1, (i * iwi.channels) + 1);
    }
  }

  void import_sndfile::close() {
    assert(sf != 0);
    sf_close(sf);
  }


  waveimporter::waveimporter() {
    imp = 0;
    plugins.push_back(new import_sndfile());
  }

  waveimporter::~waveimporter() {
    for (size_t i = 0; i < plugins.size(); i++)
      delete plugins[i];
    plugins.clear();
  }

  importplugin* waveimporter::get_importer(std::string filename) {
    size_t dp = filename.find_last_of('.');
    if (dp == std::string::npos) return 0;
    std::string ext = filename.substr(dp + 1);
    transform(ext.begin(), ext.end(), ext.begin(), (int(*)(int))std::tolower);
    std::vector<importplugin*>::iterator i;
    for (i = plugins.begin(); i != plugins.end(); ++i) {
      std::vector<std::string> exts = (*i)->get_extensions();
      std::vector<std::string>::iterator j = find(exts.begin(), exts.end(), ext);
      if (j != exts.end()) return *i;
    }

    return 0;
  }

  bool waveimporter::open(std::string filename, zzub::instream* inf) {
    imp = get_importer(filename);
    if (!imp) return false;
    return imp->open(inf);
  }

  int waveimporter::get_wave_count() {
    assert(imp);
    return imp->get_wave_count();
  }

  int waveimporter::get_wave_level_count(int i) {
    return imp->get_wave_level_count(i);
  }

  bool waveimporter::get_wave_level_info(int i, int level, importwave_info& info) {
    return imp->get_wave_level_info(i, level, info);
  }

  void waveimporter::read_wave_level_samples(int i, int level, void* buffer) {
    imp->read_wave_level_samples(i, level, buffer);
  }

  void waveimporter::close() {
    imp->close();
    imp = 0;
  }


  /***

      player

  ***/

  player::player() {
    swap_operations_commit = false;

    history_position = history.begin();

  }

  player::~player(void) {
    if (front.plugins[0] != 0) {
      front.plugins[0]->plugin->destroy();
      delete front.plugins[0]->callbacks;
      delete front.plugins[0];
      front.plugins[0] = 0;
    }
    front.plugins.clear();

    for (size_t i = 0; i < plugin_libraries.size(); i++) {
      delete plugin_libraries[i];
    }
    plugin_libraries.clear();
  }

  bool player::initialize() {
#if defined(__SSE__)
    std::cout << "SSE optimization is enabled." << std::endl;
#else
    std::cout << "SSE optimization is not enabled. Expect your CPU to choke once in a while." << std::endl;
#endif

    user_thread_id = thread_id::get();

    front.work_position = 0;
    front.master_info.tick_position = 0;
    front.master_info.samples_per_second = work_rate;
    memset(&hostinfo, 0, sizeof(host_info));

    std::vector<char> bytes;
    create_plugin(bytes, "Master", &front.master_plugininfo, 0);
    flush_operations(0, 0, 0);
    flush_from_history();

    initialize_plugin_libraries();

    front.wavetable.waves.resize(200);
    for (size_t i = 0; i < front.wavetable.waves.size(); i++) {
      wave_info_ex* wave = new wave_info_ex();
      wave->proxy = new wave_proxy(this, i);
      front.wavetable.waves[i] = wave;
    }

    front.state = player_state_stopped;
    return true;
  }


  void player::load_plugin_library(const std::string &fullpath) {
    int dpos=(int)fullpath.find_last_of('.');
    string fileExtension = fullpath.substr(dpos);
    if (fileExtension == ".so") {
      // machine loaders will be registered by lib through registerMachineLoader,
      // now and during loading of songs
      pluginlib* l = new pluginlib(fullpath, *this);
      if (l->collection != 0)
	plugin_libraries.push_back(l); 
      else 
	delete l;
    }
  }

  void player::initialize_plugin_directory(std::string folder) {
    using namespace std;

    struct dirent **namelist;
    struct stat statinfo;
    int n;
	
    string searchPath=folder;
	
    n = scandir(searchPath.c_str(), &namelist, 0, alphasort);
    if (n < 0)
      perror("scandir");
    else {
      while(n--) {
	string fullFilePath=folder + namelist[n]->d_name;
	printf("enumerating %s\n", fullFilePath.c_str());
	if (!stat(fullFilePath.c_str(), &statinfo))
	  {
	    if (!S_ISDIR(statinfo.st_mode))
	      {
		load_plugin_library(fullFilePath);
	      }
	  }
	free(namelist[n]);
      }
      free(namelist);
    }

  }

  void player::initialize_plugin_libraries() {
    // add input collection
    plugin_libraries.push_back(new pluginlib("input", *this, &inputPluginCollection));
    // add output collection
    plugin_libraries.push_back(new pluginlib("output", *this, &outputPluginCollection));
    // add recorder collection
    plugin_libraries.push_back(new pluginlib("recorder", *this, &recorderPluginCollection));

    // initialize rest like usual
    for (size_t i = 0; i < plugin_folders.size(); i++) {
      initialize_plugin_directory(plugin_folders[i]);
    }
  }

  void player::set_state(player_state newstate) {
    op_state_change* o = new op_state_change(newstate);
    backbuffer_operations.push_back(o);
    o->prepare(front);
    // TODO: fix leak!

    // NOTE: the state is now queued until flushed:
    // player::set_state() is called from zzub_player_set_state() (apps) and
    // host::set_state() (plugins).
    // zzub_player_set_state() handles flushing for clients, and 
    // host::set_state() will always (usually?) be a part of a larger operation
    // when it is called, so flushing presumably takes place later. when 
    // host::set_state() is called from the audio thread, it will invoke 
    // set_state_direct() instead, not requiring a flush. (phew!)
  }

  void player::set_state_direct(player_state newstate) {
    op_state_change o(newstate);
    o.prepare(front);
    o.operate(front);
    o.finish(front, false);
    front.plugin_invoke_event(0, o.event_data, false);
    //execute_single_operation(&o);
  }

  void player::set_play_position(int pos) {
    op_player_song_position* o = new op_player_song_position(pos);
    backbuffer_operations.push_back(o);
    o->prepare(front);
    // TODO: fix leak!

    // NOTE: also see note for player::set_state(). the same stuff goes on here too.
  }

  /*	\brief Clears all data associated with current song from the player.
   */
  void player::clear() {
    using namespace std;
    set_state(player_state_muted);
    // make sure we flushed since we are manipulating the front buffer directly
    flush_operations(0, 0, 0);
    front.is_recording_parameters = false;
    front.midi_mappings.clear();
    front.keyjazz.clear();
    front.sequencer_tracks.clear();
    front.midi_plugin = -1;
    //front.user_event_queue_read = 0;
    //front.user_event_queue_write = 0;
    operation_copy_flags flags;
    flags.copy_plugins = true;
    flags.copy_wavetable = true;
    merge_backbuffer_flags(flags);
    // release sample data on all wave levels
    for (int i = 0; i < (int)back.wavetable.waves.size(); i++)
      wave_clear(i);
    for (int i = 0; i < (int)back.plugins.size(); i++) {
      // skip NULL plugins
      if (back.plugins[i] == 0) continue;
      // reset the master plugin
      if (back.plugins[i]->info->flags & zzub::plugin_flag_is_root) {
	clear_plugin(0);
	back.process_plugin_events(0);
	continue;
      }
      // destroy all other plugins
      plugin_destroy(i);
    }
    flush_operations(0, 0, 0);
    clear_history();
    front.song_comment = "";
    front.song_begin = 0;
    front.song_end = 16;
    front.song_loop_begin = 0;
    front.song_loop_end = 16;
    front.song_loop_enabled = true;
    // there is most likely a bunch of NULL-plugins at the end of plugins, so we trim them off here
    // we assume only master is left at plugin index 0
    if (front.plugins.size() > 1)
      front.plugins.erase(front.plugins.begin() + 1, front.plugins.end());
    set_state(player_state_stopped);
  }

  void player::clear_plugin(int id) {
    operation_copy_flags flags;
    flags.copy_plugins = true;
    flags.copy_graph = true;
    flags.copy_sequencer_tracks = true;
    merge_backbuffer_flags(flags);
    // Delete all the incomming connections.
    while (back.plugin_get_input_connection_count(id) != 0) {
      int from_id = back.plugin_get_input_connection_plugin(id, 0);
      connection_type type = back.plugin_get_input_connection_type(id, 0);
      plugin_delete_input(id, from_id, type);
    }
    // Delete all the outgoing connections.
    while (back.plugin_get_output_connection_count(id) != 0) {
      int to_id = back.plugin_get_output_connection_plugin(id, 0);
      connection_type type = back.plugin_get_output_connection_type(id, 0);
      plugin_delete_input(to_id, id, type);
    }
    for (int j = 0; j < (int)back.sequencer_tracks.size(); ) {
      if (back.sequencer_tracks[j].plugin_id == id) {
	sequencer_remove_track(j);
      } else
	j++;
    }
    for (int j = 0; j < (int)back.plugins[id]->patterns.size(); ) {
      plugin_remove_pattern(id, j);
    }
    plugin_set_position(id, 0, 0);
    // set default params
    for (int group = 1; group <= 2; group++) {
      for (int track = 0; track < back.plugin_get_track_count(id, group); track++) {
	for (int j = 0; j < back.plugin_get_parameter_count(id, group, 0); j++) {
	  const parameter* param = back.plugin_get_parameter_info(id, group, track, j);
	  plugin_set_parameter(id, group, track, j, param->value_default, false, false, true);
	}
      }
    }
    const zzub::info* i = back.plugins[id]->info;
    plugin_set_track_count(id, i->min_tracks);
  }

  void player::audio_enabled() {
    // tell undo manager to wait for audio thread to perform swapping
    swap_mode = true;
  }

  void player::audio_disabled() {
    // tell undo manager we should swap directly on the user thread
    swap_mode = false;
  }

  void player::samplerate_changed() {
    front.master_info.samples_per_second = work_rate;
  }

  void player::work_stereo(int sample_count) {
    using namespace std;
    work_buffer_position = 0;
    int remaining_samples = sample_count;
    while (remaining_samples > 0) {
      // handle serialized editing
      poll_operations();
      swap_lock.lock();
      // handle MIDI input
      if (midiDriver) 
	midiDriver->poll();
      for (int i = 0; i < work_out_channel_count; i++) {
	front.outputBuffer[i] = &work_out_buffer[i][work_buffer_position];
      }
      for (int i = 0; i < work_in_channel_count; i++) {
	if (work_in_device)
	  front.inputBuffer[i] = &work_in_buffer[i][work_buffer_position]; else
	  front.inputBuffer[i] = 0;
      }
      int chunk_size = front.generate_audio(remaining_samples);
      // the master plugins work_buffer has the final output
      // users can add Audio Output-plugins to send output to channels > 2
      metaplugin& masterplugin = front.get_plugin(0);
      memcpy(&work_out_buffer[work_master_channel*2+0][work_buffer_position], &masterplugin.work_buffer[0].front(), chunk_size * sizeof(float));
      memcpy(&work_out_buffer[work_master_channel*2+1][work_buffer_position], &masterplugin.work_buffer[1].front(), chunk_size * sizeof(float));
      work_buffer_position += chunk_size;
      remaining_samples -= chunk_size;
      swap_lock.unlock();
    }
    // update cpu_load per plugin
    for (unsigned int i = 0; i < front.plugins.size(); i++) {
      if (front.plugins[i] == 0) continue;
      metaplugin& m = *front.plugins[i];
      double load;		
      if (m.cpu_load_buffersize > 0)
	load = (m.cpu_load_time * double(front.master_info.samples_per_second)) / double(m.cpu_load_buffersize); 
      else
	load = 0;
      m.cpu_load += 0.1 * (load - m.cpu_load);
      m.cpu_load_time = 0;
      m.cpu_load_buffersize = 0;
    }
  }

  // ---------------------------------------------------------------------------
  //
  // MIDI stuff and keyboard note handling
  //
  // ---------------------------------------------------------------------------

  void player::midiEvent(unsigned short status, unsigned char data1, unsigned char data2) {
    // midi sync
    if (front.is_syncing_midi_transport) {
      if (status == 0xf2) {
	// set song position pointer
	int spp = data1 | (data2 << 7);
	front.song_position = front.song_loop_begin + spp;
      } else if (status == 0xfa) {
	// midi start
	front.song_position = 0;	
	set_state_direct(player_state_playing);
      } else if (status == 0xfb) {
	// midi continue
	set_state_direct(player_state_playing);
      } else if (status == 0xfc) {
	// midi stop
	set_state_direct(player_state_stopped);
      }
    }
    // send midi event to the mixer class for handling per machine
    front.midi_event(status, data1, data2);
  }


  void player::play_plugin_note(int plugin_id, int note, int prevNote, int _velocity) {
    op_plugin_play_note o(plugin_id, note, prevNote, _velocity);
    merge_backbuffer_flags(o.copy_flags);
    backbuffer_operations.push_back(&o);
    o.prepare(back);
    flush_operations(0, 0, 0);
  }

  void player::reset_keyjazz() {
    if (front.keyjazz.size() == 0) 
      return ;
    // send note off for all currently playing notes
    vector<op_plugin_play_note*> ops;
    for (vector<keyjazz_note>::iterator i = front.keyjazz.begin(); i != front.keyjazz.end(); ++i) {
      if (i->delay_off) 
	continue;
      op_plugin_play_note* o = new op_plugin_play_note(i->plugin_id, note_value_off, i->note, 0);
      backbuffer_operations.push_back(o);
      o->prepare(back);
      ops.push_back(o);
    }
    if (ops.size() == 0) 
      return;
    flush_operations(0, 0, 0);
    for (vector<op_plugin_play_note*>::iterator i = ops.begin(); i != ops.end(); ++i) {
      delete *i;
    }
  }


  std::string player::plugin_get_new_name(std::string uri) {
    operation_copy_flags flags;
    flags.copy_graph = true;
    merge_backbuffer_flags(flags);
    using namespace std;
    string baseName;
    std::vector<const zzub::info*>::iterator info = find_if(plugin_infos.begin(), plugin_infos.end(), find_info_by_uri(uri));
    if (info == plugin_infos.end()) 
      baseName = uri; else
      baseName = (*info)->short_name;
    for (int i = 0; i < 9999; i++) {
      std::stringstream strm;
      if (i == 0) {
	strm << baseName;
      }  else {
	strm << baseName << (i+1);
      }
      zzub::plugin_descriptor m = back.get_plugin_descriptor(strm.str());
      if (m == graph_traits<plugin_map>::null_vertex()) return strm.str();
    }
    assert(false);
    return baseName;	// error
  }

  const zzub::info* player::plugin_get_info(std::string uri) {
    std::vector<const zzub::info*>::iterator i = find_if(plugin_infos.begin(), plugin_infos.end(), find_info_by_uri(uri));
    if (i == plugin_infos.end()) 
      return 0;
    else
      return *i;
  }

  void player::begin_plugin_operation(int plugin_id) {
    operation_copy_flags flags;
    flags.copy_plugins = true;
    merge_backbuffer_flags(flags);

    int pflags = back.plugins[plugin_id]->flags;
    if (pflags & zzub_plugin_flag_no_undo) {
      no_undo_stack.push(ignore_undo);
      ignore_undo = true;
    }
  }

  void player::end_plugin_operation(int plugin_id) {
    int flags = back.plugins[plugin_id]->flags;
    if (flags & zzub_plugin_flag_no_undo) {
      ignore_undo = no_undo_stack.top();
      no_undo_stack.pop();
    }
  }


  // ---------------------------------------------------------------------------
  //
  // Idle GUI messages
  //
  // ---------------------------------------------------------------------------

  /*! \brief Process GUI messages. 

    Whenever the player invokes an event that may reach the GUI, it is
    queued until the host invokes handleMessages() on its thread.

  */
  void player::process_user_event_queue() {
    while (front.user_event_queue_read != front.user_event_queue_write) {
      event_message& message = front.user_event_queue[front.user_event_queue_read];
      if (message.event != 0) message.event->invoke(message.data);
      if (front.user_event_queue_read == front.user_event_queue.size() - 1)
	front.user_event_queue_read = 0; else
	front.user_event_queue_read++;
    }
  }

  void player::set_event_queue_state(int enable) {
    front.enable_event_queue = enable;
  }

  /***

      User methods for writing to the graph

      The following methods set up redo/undo operations and executes the redo-operation

  ***/


  void player::plugin_set_parameter(int plugin_id, int group, int track, int column, int value, bool record, bool immediate, bool undoable) {
    if (immediate) {
      zzub::pattern state;
      front.create_pattern(state, plugin_id, 1);
      state.groups[group][track][column][0] = value;
      front.transfer_plugin_parameter_row(plugin_id, group, state, front.plugins[plugin_id]->state_write, 0, 0, false);
      if (record)
	front.transfer_plugin_parameter_row(plugin_id, group, state, front.plugins[plugin_id]->state_automation, 0, 0, false);
    } else {
      // her kan vi merge flagg
      operation_copy_flags flags;
      flags.copy_plugins = true;
      flags.copy_graph = true;
      merge_backbuffer_flags(flags);

      op_plugin_set_parameter* redo = new op_plugin_set_parameter(plugin_id, group, track, column, value, record);

      if (undoable) {
	int oldval = back.plugin_get_parameter(plugin_id, group, track, column);
	op_plugin_set_parameter* undo = new op_plugin_set_parameter(plugin_id, group, track, column, oldval, record);

	prepare_operation_redo(redo);
	prepare_operation_undo(undo);
      } else {
	// these two lines are identical to prepare_operation_redo() except we dont add it to the undo buffer:
	redo->prepare(back);
	backbuffer_operations.push_back(redo);
	// TODO: dont leak the redo object here!
      }
    }
  }

  void player::add_midimapping(int plugin_id, int group, int track, int param, int channel, int controller) {
    zzub::midimapping mapping;
    mapping.plugin_id = plugin_id;
    mapping.group = group;
    mapping.track = track;
    mapping.column = param;
    mapping.channel = channel;
    mapping.controller = controller;
    op_midimapping_insert* redo = new op_midimapping_insert(mapping);
    prepare_operation_redo(redo);
    op_midimapping_remove* undo = new op_midimapping_remove(back.midi_mappings.size() - 1);
    prepare_operation_undo(undo);
  }

  void player::remove_midimapping(int plugin_id, int group, int track, int param) {
    operation_copy_flags flags;
    flags.copy_midi_mappings = true;
    merge_backbuffer_flags(flags);
    int found = 0;
    for (int i = 0; i < (int)back.midi_mappings.size(); i++) {
      midimapping& mm = back.midi_mappings[i];
      if (mm.plugin_id == plugin_id && mm.group == group && mm.track == track && mm.column == param) {
	op_midimapping_remove* redo = new op_midimapping_remove(i - found);
	op_midimapping_insert* undo = new op_midimapping_insert(mm);
	prepare_operation_redo(redo);
	prepare_operation_undo(undo);
      }
    }
  }

  int player::create_plugin(std::vector<char>& bytes, string name, const zzub::info* loader, int pflags) {
    operation_copy_flags flags;
    flags.copy_plugins = true;
    flags.copy_wavetable = true;
    merge_backbuffer_flags(flags);
    int next_id = (int)back.plugins.size();
    pflags |= loader->flags;
    if (pflags & zzub_plugin_flag_no_undo) {
      no_undo_stack.push(ignore_undo);
      ignore_undo = true;
    }
    op_plugin_create* redo = new op_plugin_create(this, next_id, name, bytes, loader, pflags);
    if (!prepare_operation_redo(redo)) 
      return -1;
    op_plugin_delete* undo = new op_plugin_delete(this, next_id);
    prepare_operation_undo(undo);
    if (pflags & zzub_plugin_flag_no_undo) {
      ignore_undo = no_undo_stack.top();
      no_undo_stack.pop();
    }
    return next_id;
  }

  void player::plugin_destroy(int id) {
    op_plugin_delete* redo = new op_plugin_delete(this, id);
    merge_backbuffer_flags(redo->copy_flags);
    assert(id >= 0 && id < back.plugins.size());
    assert(back.plugins[id] != 0);
    metaplugin& m = *back.plugins[id];
    begin_plugin_operation(id);
    clear_plugin(id);
    // NOTE: undo operations are added in opposite order (!!!!1)
    int tracks = m.tracks;
    op_plugin_set_track_count* undo_tracks = new op_plugin_set_track_count(id, tracks);
    prepare_operation_undo(undo_tracks);
    // create undo for minimize, mute, xy-position, midi-channel, etc
    metaplugin pos_plugin = m;
    op_plugin_replace* undo_pos = new op_plugin_replace(id, pos_plugin);
    prepare_operation_undo(undo_pos);
    std::vector<char> bytes;
    op_plugin_create* undo = new op_plugin_create(this, id, m.name, bytes, m.info, m.flags);
    prepare_operation_undo(undo);
    prepare_operation_redo(redo);
    end_plugin_operation(id);
  }

  void player::plugin_set_name(int id, std::string name) {
    zzub::metaplugin m;
    op_plugin_replace* redo = new op_plugin_replace(id, m);
    merge_backbuffer_flags(redo->copy_flags);
    redo->plugin = *back.plugins[id];
    redo->plugin.name = name;
    begin_plugin_operation(id);
    prepare_operation_redo(redo);
    op_plugin_replace* undo = new op_plugin_replace(id, *back.plugins[id]);
    prepare_operation_undo(undo);
    end_plugin_operation(id);
  }

  void player::plugin_set_position(int id, float x, float y) {
    zzub::metaplugin m;
    op_plugin_replace* redo = new op_plugin_replace(id, m);
    merge_backbuffer_flags(redo->copy_flags);
    redo->plugin = *back.plugins[id];
    redo->plugin.x = x;
    redo->plugin.y = y;
    begin_plugin_operation(id);
    prepare_operation_redo(redo);
    op_plugin_replace* undo = new op_plugin_replace(id, *back.plugins[id]);
    prepare_operation_undo(undo);
    end_plugin_operation(id);
  }

  void player::plugin_set_track_count(int id, int tracks) {
    op_plugin_set_track_count* redo = new op_plugin_set_track_count(id, tracks);
    merge_backbuffer_flags(redo->copy_flags);
    metaplugin& m = *back.plugins[id];
    begin_plugin_operation(id);
    // add undo actions for removed pattern data
    for (int i = 0; i < (int)m.patterns.size(); i++) {
      op_pattern_replace* undo_p = new op_pattern_replace(id, i, *m.patterns[i]);
      prepare_operation_undo(undo_p);
    }
    op_plugin_set_track_count* undo = new op_plugin_set_track_count(id, m.tracks);
    prepare_operation_undo(undo);
    // prepare the redo operation after the undo operations are set up
    prepare_operation_redo(redo);
    end_plugin_operation(id);
  }

  void player::plugin_add_pattern(int id, const zzub::pattern& pattern) {
    begin_plugin_operation(id);
    prepare_operation_redo(new op_pattern_insert(id, -1, pattern));
    prepare_operation_undo(new op_pattern_remove(id, -1));
    end_plugin_operation(id);
  }

  void player::plugin_remove_pattern(int id, int pattern) {
    op_pattern_remove* redo = new op_pattern_remove(id, pattern);
    merge_backbuffer_flags(redo->copy_flags);
    //plugin_descriptor plugin = back.plugins[id]->descriptor;
    begin_plugin_operation(id);
    // remove all sequence events for this pattern in an undoable manner
    std::vector<std::pair<int, int> > remove_events;
    for (size_t i = 0; i < back.sequencer_tracks.size(); i++) {
      if (back.sequencer_tracks[i].plugin_id == id) {
	for (size_t j = 0; j < back.sequencer_tracks[i].events.size(); j++) {
	  sequence_event& ta = back.sequencer_tracks[i].events[j];
	  if (ta.pattern_event.value >= 0x10) {
	    if (pattern == ta.pattern_event.value - 0x10) {
	      remove_events.push_back(std::pair<int, int>(i, ta.time));
	    }
	  }
	}
      }
    }
    for (size_t i = 0; i < remove_events.size(); i++) {
      sequencer_set_event(remove_events[i].first, remove_events[i].second, -1);
    }
    op_pattern_move* undo_move = new op_pattern_move(id, -1, pattern);
    prepare_operation_undo(undo_move);
    op_pattern_insert* undo = new op_pattern_insert(id, -1, *back.plugins[id]->patterns[pattern]);
    prepare_operation_undo(undo);
    prepare_operation_redo(redo);
    end_plugin_operation(id);
  }

  void player::plugin_move_pattern(int id, int pattern, int newindex) {
    assert(false);
  }

  void player::plugin_update_pattern(int id, int index, const zzub::pattern& pattern) {
    zzub::pattern newpattern;
    op_pattern_replace* redo = new op_pattern_replace(id, index, newpattern);
    merge_backbuffer_flags(redo->copy_flags);
    redo->pattern = pattern;
    begin_plugin_operation(id);
    op_pattern_replace* undo = new op_pattern_replace(id, index, *back.plugins[id]->patterns[index]);
    prepare_operation_undo(undo);
    prepare_operation_redo(redo);
    end_plugin_operation(id);
  }

  void player::plugin_set_pattern_name(int id, int index, std::string name) {
    zzub::pattern newpattern;
    op_pattern_replace* redo = new op_pattern_replace(id, index, newpattern);
    merge_backbuffer_flags(redo->copy_flags);
    redo->pattern = *back.plugins[id]->patterns[index];
    redo->pattern.name = name;
    begin_plugin_operation(id);
    op_pattern_replace* undo = new op_pattern_replace(id, index, *back.plugins[id]->patterns[index]);
    prepare_operation_undo(undo);
    prepare_operation_redo(redo);
    end_plugin_operation(id);
  }

  void player::plugin_set_pattern_length(int id, int index, int rows) {
    zzub::pattern newpattern;// = *get_plugin(plugin).patterns[index];
    op_pattern_replace* redo = new op_pattern_replace(id, index, newpattern);
    merge_backbuffer_flags(redo->copy_flags);
    begin_plugin_operation(id);
    op_pattern_replace* undo = new op_pattern_replace(id, index, *back.plugins[id]->patterns[index]);
    prepare_operation_undo(undo);
    redo->pattern = *back.plugins[id]->patterns[index];
    back.set_pattern_length(id, redo->pattern, rows);
    prepare_operation_redo(redo);
    end_plugin_operation(id);
  }

  void player::plugin_set_pattern_value(int id, int pattern, int group, int track, int column, int row, int value) {
    op_pattern_edit* redo = new op_pattern_edit(id, pattern, group, track, column, row, value);
    merge_backbuffer_flags(redo->copy_flags);
    begin_plugin_operation(id);
    int prevvalue = back.plugins[id]->patterns[pattern]->groups[group][track][column][row];
    op_pattern_edit* undo = new op_pattern_edit(id, pattern, group, track, column, row, prevvalue);
    prepare_operation_undo(undo);
    prepare_operation_redo(redo);
    end_plugin_operation(id);
  }

  void player::plugin_insert_pattern_rows(int plugin_id, int pattern, int* column_indices, int num_indices, int start, int rows) {
    operation_copy_flags flags;
    flags.copy_plugins = true;
    merge_backbuffer_flags(flags);
    operation_copy_pattern_flags patternflags;
    patternflags.plugin_id = plugin_id;
    patternflags.index = pattern;
    flags.pattern_flags.push_back(patternflags);
    metaplugin& m = *back.plugins[plugin_id];
    zzub::pattern& p = *m.patterns[pattern];
    begin_plugin_operation(plugin_id);
    for (int i = 0; i < num_indices; i++) {
      int group = column_indices[i * 3 + 0];
      int track = column_indices[i * 3 + 1];
      int column = column_indices[i * 3 + 2];
      int first_overflow_row = p.rows - rows;
      for (int j = 0; j < rows; j++) {
	int v = p.groups[group][track][column][first_overflow_row + j];
	op_pattern_edit* undo_edit = new op_pattern_edit(plugin_id, pattern, group, track, column, first_overflow_row + j, v);
	prepare_operation_undo(undo_edit);
      }
    }
    std::vector<int> columns;
    columns.insert(columns.begin(), column_indices, column_indices + (num_indices * 3));
    op_pattern_insert_rows* redo = new op_pattern_insert_rows(plugin_id, pattern, start, columns, rows);
    op_pattern_remove_rows* undo = new op_pattern_remove_rows(plugin_id, pattern, start, columns, rows);
    prepare_operation_redo(redo);
    prepare_operation_undo(undo);
    end_plugin_operation(plugin_id);
  }

  void player::plugin_remove_pattern_rows(int plugin_id, int pattern, int* column_indices, int num_indices, int start, int rows) {
    operation_copy_flags flags;
    flags.copy_plugins = true;
    operation_copy_pattern_flags patternflags;
    patternflags.plugin_id = plugin_id;
    patternflags.index = pattern;
    flags.pattern_flags.push_back(patternflags);
    merge_backbuffer_flags(flags);
    metaplugin& m = *back.plugins[plugin_id];
    zzub::pattern& p = *m.patterns[pattern];
    begin_plugin_operation(plugin_id);
    for (int i = 0; i < num_indices; i++) {
      int group = column_indices[i * 3 + 0];
      int track = column_indices[i * 3 + 1];
      int column = column_indices[i * 3 + 2];
      for (int j = 0; j < rows; j++) {
	int v = p.groups[group][track][column][start + j];
	op_pattern_edit* undo_edit = new op_pattern_edit(plugin_id, pattern, group, track, column, start + j, v);
	prepare_operation_undo(undo_edit);
      }
    }
    std::vector<int> columns;
    columns.insert(columns.begin(), column_indices, column_indices + (num_indices * 3));
    op_pattern_remove_rows* redo = new op_pattern_remove_rows(plugin_id, pattern, start, columns, rows);
    op_pattern_insert_rows* undo = new op_pattern_insert_rows(plugin_id, pattern, start, columns, rows);
    prepare_operation_redo(redo);
    prepare_operation_undo(undo);
    end_plugin_operation(plugin_id);
  }

  bool player::plugin_add_input(int to_id, int from_id, connection_type type) {
    begin_plugin_operation(to_id);
    begin_plugin_operation(from_id);

    op_plugin_connect* redo = new op_plugin_connect(from_id, to_id, type);
    if (!prepare_operation_redo(redo)) {
      delete redo;
      return false;
    }

    op_plugin_disconnect* undo = new op_plugin_disconnect(from_id, to_id, type);
    prepare_operation_undo(undo);

    end_plugin_operation(from_id);
    end_plugin_operation(to_id);
    return true;
  }

  void player::plugin_delete_input(int to_id, int from_id, connection_type type) {

    op_plugin_disconnect* redo = new op_plugin_disconnect(from_id, to_id, type);
    merge_backbuffer_flags(redo->copy_flags);

    //plugin_descriptor from_plugin = back.plugins[from_id]->descriptor;
    //plugin_descriptor to_plugin = back.plugins[to_id]->descriptor;

    begin_plugin_operation(to_id);
    begin_plugin_operation(from_id);

    op_plugin_connect* undo = new op_plugin_connect(from_id, to_id, type);

    int track = back.plugin_get_input_connection_index(to_id, from_id, type);
    assert(track != -1);
    connection* conn = back.plugin_get_input_connection(to_id, track);
    for (int i = 0; i < (int)conn->connection_parameters.size(); i++) {
      undo->values.push_back(back.plugin_get_parameter(to_id, 0, track, i));
    }

    switch (type) {
    case zzub::connection_type_audio:
      break;
    case zzub::connection_type_midi:
      undo->midi_device = ((midi_connection*)conn)->device_name;
      break;
    case zzub::connection_type_event:
      undo->bindings = ((event_connection*)conn)->bindings;
      break;
    }
    prepare_operation_redo(redo);
    prepare_operation_undo(undo);

    end_plugin_operation(from_id);
    end_plugin_operation(to_id);
  }

  void player::plugin_set_midi_connection_device(int to_id, int from_id, std::string name) {
    operation_copy_flags flags;
    flags.copy_graph = true;
    flags.copy_plugins = true;
    merge_backbuffer_flags(flags);

    begin_plugin_operation(to_id);
    begin_plugin_operation(from_id);

    op_plugin_set_midi_connection_device* redo = new op_plugin_set_midi_connection_device(to_id, from_id, name);
    int midiconn = back.plugin_get_input_connection_index(to_id, from_id, connection_type_midi);
    assert(midiconn != -1);
	
    midi_connection* conn = (midi_connection*)back.plugin_get_input_connection(to_id, midiconn);

    op_plugin_set_midi_connection_device* undo = new op_plugin_set_midi_connection_device(to_id, from_id, conn->device_name);

    prepare_operation_redo(redo);
    prepare_operation_undo(undo);

    end_plugin_operation(from_id);
    end_plugin_operation(to_id);
  }

  void player::plugin_add_event_connection_binding(int to_id, int from_id, int sourceparam, int targetgroup, int targettrack, int targetparam) {
    event_connection_binding binding;
    binding.source_param_index = sourceparam;
    binding.target_group_index = targetgroup;
    binding.target_track_index = targettrack;
    binding.target_param_index = targetparam;

    begin_plugin_operation(to_id);
    begin_plugin_operation(from_id);

    op_plugin_add_event_connection_binding* redo = new op_plugin_add_event_connection_binding(to_id, from_id, binding);

    op_plugin_remove_event_connection_binding* undo = new op_plugin_remove_event_connection_binding(to_id, from_id, -1);
    prepare_operation_redo(redo);
    prepare_operation_undo(undo);

    end_plugin_operation(from_id);
    end_plugin_operation(to_id);
  }

  void player::plugin_remove_event_connection_binding(int to_id, int from_id, int index) {
    assert(false);
  }

  void player::plugin_set_stream_source(int plugin_id, std::string data_url) {
    operation_copy_flags flags;
    flags.copy_plugins = true;
    operation_copy_plugin_flags pluginflags;
    pluginflags.plugin_id = plugin_id;
    pluginflags.copy_plugin = true;
    flags.plugin_flags.push_back(pluginflags);
    merge_backbuffer_flags(flags);

    std::string prev_data_url = back.plugins[plugin_id]->stream_source;

    begin_plugin_operation(plugin_id);

    op_plugin_set_stream_source* redo = new op_plugin_set_stream_source(plugin_id, data_url);
    prepare_operation_redo(redo);

    op_plugin_set_stream_source* undo = new op_plugin_set_stream_source(plugin_id, prev_data_url);
    prepare_operation_undo(undo);

    end_plugin_operation(plugin_id);
  }


  void player::sequencer_add_track(int id, sequence_type type) {

    // TODO: disallow sequencer tracks for no_undo-plugins?
    // adding a track, and then adding more tracks with non-no_undo plugins will mess up the 
    // undo buffer for sure when the no_undo-plugin is deleted.. correctional surgery on the history 
    // would be messy and a first

    // TODO: disallow wave_tracks on plugins that has no plugin_plays_wave flag

    op_sequencer_create_track* redo = new op_sequencer_create_track(this, id, type);
    prepare_operation_redo(redo);

    op_sequencer_remove_track* undo = new op_sequencer_remove_track(-1);
    prepare_operation_undo(undo);
  }

  void player::sequencer_remove_track(int index) {
    operation_copy_flags flags;
    flags.copy_graph = true;
    flags.copy_plugins = true;
    flags.copy_sequencer_tracks = true;

    merge_backbuffer_flags(flags);

    int plugin_id = (int)back.sequencer_tracks[index].plugin_id;
    sequence_type type = back.sequencer_tracks[index].type;

    // remove the events in this track in an undoable fashion
    std::vector<std::pair<int, int> > remove_events;
    for (size_t j = 0; j < back.sequencer_tracks[index].events.size(); j++) {
      sequence_event& ev = back.sequencer_tracks[index].events[j];
      remove_events.push_back(std::pair<int, int>(index, ev.time));
    }

    for (size_t i = 0; i < remove_events.size(); i++) {
      sequencer_set_event(remove_events[i].first, remove_events[i].second, -1);
    }

    op_sequencer_remove_track* redo = new op_sequencer_remove_track(index);
    prepare_operation_redo(redo);

    // NOTE: adding undo operations in reverse order
    op_sequencer_move_track* undo_pos = new op_sequencer_move_track(-1, index);
    prepare_operation_undo(undo_pos);

    op_sequencer_create_track* undo = new op_sequencer_create_track(this, plugin_id, type);
    prepare_operation_undo(undo);

  }

  void player::sequencer_move_track(int index, int newindex) {
    op_sequencer_move_track* redo = new op_sequencer_move_track(index, newindex);
    merge_backbuffer_flags(redo->copy_flags);

    prepare_operation_redo(redo);

    op_sequencer_move_track* undo = new op_sequencer_move_track(newindex, index);
    prepare_operation_undo(undo);
  }

  void player::sequencer_set_event(int track, int pos, int value) {
    op_sequencer_set_event* redo = new op_sequencer_set_event(pos, track, value);
    merge_backbuffer_flags(redo->copy_flags);

    int prevvalue = back.sequencer_get_event_at(track, pos);
    prepare_operation_undo(new op_sequencer_set_event(pos, track, prevvalue));
    prepare_operation_redo(redo);
  }

  void player::sequencer_insert_events(int track, int start, int ticks) {
    operation_copy_flags flags;
    flags.copy_sequencer_tracks = true;
    merge_backbuffer_flags(flags);

    op_sequencer_replace* undo = new op_sequencer_replace(back.sequencer_tracks);

    // set up temporary operation-objects to perform the sequencer editing on the back buffer
    std::vector<op_sequencer_set_event> remove_ops;
    std::vector<op_sequencer_set_event> insert_ops;
    for (size_t i = 0; i < back.sequencer_tracks[track].events.size(); i++) {
      sequence_event& ev =  back.sequencer_tracks[track].events[i];
      if (ev.time >= start) {
	remove_ops.push_back(op_sequencer_set_event(ev.time, track, -1));
	insert_ops.push_back(op_sequencer_set_event(ev.time + ticks, track, ev.pattern_event.value));
      }
    }

    // do the reading and writing, not calling finish() to avoid sending any events
    std::vector<op_sequencer_set_event>::iterator op;
    for (op = remove_ops.begin(); op != remove_ops.end(); ++op) {
      op->prepare(back);
      op->operate(back);
    }
    for (op = insert_ops.begin(); op != insert_ops.end(); ++op) {
      op->prepare(back);
      op->operate(back);
    }

    // put the backbuffer-sequencer into the undo buffer
    op_sequencer_replace* redo = new op_sequencer_replace(back.sequencer_tracks);
    prepare_operation_redo(redo);
    prepare_operation_undo(undo);

  }

  void player::sequencer_remove_events(int track, int start, int ticks) {
    operation_copy_flags flags;
    flags.copy_sequencer_tracks = true;
    merge_backbuffer_flags(flags);

    op_sequencer_replace* undo = new op_sequencer_replace(back.sequencer_tracks);

    // set up temporary operation-objects to perform the sequencer editing on the back buffer
    std::vector<op_sequencer_set_event> remove_ops;
    std::vector<op_sequencer_set_event> insert_ops;
    for (size_t i = 0; i < back.sequencer_tracks[track].events.size(); i++) {
      sequence_event& ev = back.sequencer_tracks[track].events[i];
      if (ev.time >= start) {
	// this event is subject to moving
	remove_ops.push_back(op_sequencer_set_event(ev.time, track, -1));
	if (ev.time - ticks >= start)
	  insert_ops.push_back(op_sequencer_set_event(ev.time - ticks, track, ev.pattern_event.value));
      }
    }

    // do the reading and writing, not calling finish() to avoid sending any events
    std::vector<op_sequencer_set_event>::iterator op;
    for (op = remove_ops.begin(); op != remove_ops.end(); ++op) {
      op->prepare(back);
      op->operate(back);
    }
    for (op = insert_ops.begin(); op != insert_ops.end(); ++op) {
      op->prepare(back);
      op->operate(back);
    }

    // put the backbuffer-sequencer into the undo buffer
    op_sequencer_replace* redo = new op_sequencer_replace(back.sequencer_tracks);
    prepare_operation_redo(redo);
    prepare_operation_undo(undo);
  }

  /***

      Wavetable

  ***/

  int player::wave_load_sample(int wave, int level, int offset, bool clear, std::string name, zzub::instream* datastream) {
    operation_copy_flags flags;
    flags.copy_wavetable = true;
    merge_backbuffer_flags(flags);

    // because of limitations in buzz, the mono/stereo flag is specified for all levels
    // a situation occurs if we try to load a mono sample as the second level where the first
    // level is a stereo sample. what to do? three possibilities: 
    //     - convert both to mono? 
    //     - convert both to stereo?
    //     - import the stereo sample as mono sample data, so the user has to convert the 
    //       second sample manually if it mismatches the earlier sample?

    waveimporter importer;
    importwave_info wavedata;

    if (!importer.open(name.c_str(), datastream)) return 0;
    if (!importer.get_wave_level_info(0, 0, wavedata)) {
      importer.close();
      return 0;
    }

    assert(wavedata.channels != 0);

    int bytes_per_sample = sizeFromWaveFormat(wavedata.format) * wavedata.channels;
    char* buffer = new char[bytes_per_sample * wavedata.sample_count];

    importer.read_wave_level_samples(0, 0, buffer);
    importer.close();

    bool reset_wave = false;
    // determine cases where we want to set/change the wave stereo flag before loading a sample:
    if ((back.wavetable.waves[wave]->levels.size() == 1 && level == 0 && clear) || back.wavetable.waves[wave]->levels.size() == 0) {
      reset_wave = true;
    }

    // add needed levels
    while (back.wavetable.waves[wave]->levels.size() <= (size_t)level) {
      wave_add_level(wave);
    }

    // clear previous wave level contents (with undo)
    if (clear)
      wave_clear_level(wave, level);

    if (reset_wave) {
      if (wavedata.channels == 2)
	wave_set_flags(wave, wave_flag_stereo); else
	wave_set_flags(wave, 0);
		
      // TODO: set the wave format here as well
      // allocate_level = wave_set_format + wave_insert_sample_data ?
      //wave_set_format(wave, level, wavedata.sample_count);
      wave_allocate_level(wave, level, 0, wavedata.channels, wavedata.format);
    }

    assert(offset <= back.wavetable.waves[wave]->get_sample_count(level));

    // now load sample data into an insert_samples-operation
    op_wavetable_insert_sampledata* redo = new op_wavetable_insert_sampledata(wave, level, offset);

    redo->samples = buffer;
    redo->samples_format = wavedata.format;
    redo->samples_length = wavedata.sample_count;
    redo->samples_channels = wavedata.channels;

    prepare_operation_redo(redo);

    op_wavetable_remove_sampledata* undo = new op_wavetable_remove_sampledata(wave, level, offset, wavedata.sample_count);
    prepare_operation_undo(undo);

    return wavedata.sample_count;
  }

  void player::wave_allocate_level(int wave, int level, int sample_count, int channels, wave_buffer_type format) {
    operation_copy_flags flags;
    flags.copy_wavetable = true;
    merge_backbuffer_flags(flags);

    wave_info_ex& w = *back.wavetable.waves[wave];

    op_wavetable_allocate_wavelevel* redo = new op_wavetable_allocate_wavelevel(wave, level, sample_count, channels, format);

    op_wavetable_allocate_wavelevel* undo = new op_wavetable_allocate_wavelevel(wave, level, w.get_sample_count(level), w.get_stereo() ? 2 : 1, w.get_wave_format(level));

    // TODO: preserve wave data in undo
    prepare_operation_redo(redo);
    prepare_operation_undo(undo);
  }

  void player::wave_add_level(int wave) {
    op_wavetable_add_wavelevel* redo = new op_wavetable_add_wavelevel(this, wave);
    prepare_operation_redo(redo);

    op_wavetable_remove_wavelevel* undo = new op_wavetable_remove_wavelevel(wave, -1);
    prepare_operation_undo(undo);
  }

  void player::wave_remove_level(int wave, int level) {
    // TODO: since move_wave_level isnt implemented yet, undo only works with the last level - ie we cant delete an arbitrary level yet
    op_wavetable_remove_wavelevel* redo = new op_wavetable_remove_wavelevel(wave, level);
    prepare_operation_redo(redo);

    op_wavetable_move_wavelevel* undo_move = new op_wavetable_move_wavelevel(wave, back.wavetable.waves[wave]->levels.size(), level);
    prepare_operation_undo(undo_move);

    op_wavetable_add_wavelevel* undo = new op_wavetable_add_wavelevel(this, wave);
    prepare_operation_undo(undo);
  }

  void player::wave_move_level(int wave, int level, int newlevel) {
    op_wavetable_move_wavelevel* redo = new op_wavetable_move_wavelevel(wave, level, newlevel);
    prepare_operation_redo(redo);

    op_wavetable_move_wavelevel* undo = new op_wavetable_move_wavelevel(wave, newlevel, level);
    prepare_operation_undo(undo);
  }

  void player::wave_set_name(int wave, std::string name) {

    operation_copy_flags flags;
    flags.copy_wavetable = true;
    merge_backbuffer_flags(flags);

    op_wavetable_wave_replace* undo = new op_wavetable_wave_replace(wave, *back.wavetable.waves[wave]);
    prepare_operation_undo(undo);

    wave_info_ex data;
    data = *back.wavetable.waves[wave];
    data.name = name;
    op_wavetable_wave_replace* redo = new op_wavetable_wave_replace(wave, data);
    prepare_operation_redo(redo);

  }

  void player::wave_set_volume(int wave, float volume) {

    operation_copy_flags flags;
    flags.copy_wavetable = true;
    merge_backbuffer_flags(flags);

    op_wavetable_wave_replace* undo = new op_wavetable_wave_replace(wave, *back.wavetable.waves[wave]);
    prepare_operation_undo(undo);

    wave_info_ex data;
    data = *back.wavetable.waves[wave];
    data.volume = volume;
    op_wavetable_wave_replace* redo = new op_wavetable_wave_replace(wave, data);
    prepare_operation_redo(redo);

  }

  void player::wave_set_flags(int wave, int waveflags) {
    operation_copy_flags flags;
    flags.copy_wavetable = true;
    merge_backbuffer_flags(flags);

    op_wavetable_wave_replace* undo = new op_wavetable_wave_replace(wave, *back.wavetable.waves[wave]);
    prepare_operation_undo(undo);

    wave_info_ex data;
    data = *back.wavetable.waves[wave];
    data.flags = waveflags;
    op_wavetable_wave_replace* redo = new op_wavetable_wave_replace(wave, data);
    prepare_operation_redo(redo);
  }

  void player::wave_set_path(int wave, std::string name) {
    operation_copy_flags flags;
    flags.copy_wavetable = true;
    merge_backbuffer_flags(flags);

    op_wavetable_wave_replace* undo = new op_wavetable_wave_replace(wave, *back.wavetable.waves[wave]);
    prepare_operation_undo(undo);

    wave_info_ex data;
    data = *back.wavetable.waves[wave];
    data.fileName = name;
    op_wavetable_wave_replace* redo = new op_wavetable_wave_replace(wave, data);
    prepare_operation_redo(redo);
  }

  void player::wave_set_loop_begin(int wave, int level, int pos) {
    operation_copy_flags flags;
    flags.copy_wavetable = true;
    merge_backbuffer_flags(flags);

    op_wavetable_wavelevel_replace* undo = new op_wavetable_wavelevel_replace(wave, level, back.wavetable.waves[wave]->levels[level]);
    prepare_operation_undo(undo);

    wave_level_ex data;
    data = back.wavetable.waves[wave]->levels[level];
    data.loop_start = pos;
    op_wavetable_wavelevel_replace* redo = new op_wavetable_wavelevel_replace(wave, level, data);
    prepare_operation_redo(redo);
  }


  void player::wave_set_loop_end(int wave, int level, int pos) {
    operation_copy_flags flags;
    flags.copy_wavetable = true;
    merge_backbuffer_flags(flags);

    op_wavetable_wavelevel_replace* undo = new op_wavetable_wavelevel_replace(wave, level, back.wavetable.waves[wave]->levels[level]);
    prepare_operation_undo(undo);

    wave_level_ex data;
    data = back.wavetable.waves[wave]->levels[level];
    data.loop_end = pos;
    op_wavetable_wavelevel_replace* redo = new op_wavetable_wavelevel_replace(wave, level, data);
    prepare_operation_redo(redo);
  }


  void player::wave_set_samples_per_second(int wave, int level, int sps) {
    operation_copy_flags flags;
    flags.copy_wavetable = true;
    merge_backbuffer_flags(flags);

    op_wavetable_wavelevel_replace* undo = new op_wavetable_wavelevel_replace(wave, level, back.wavetable.waves[wave]->levels[level]);
    prepare_operation_undo(undo);

    wave_level_ex data;
    data = back.wavetable.waves[wave]->levels[level];
    data.samples_per_second = sps;
    op_wavetable_wavelevel_replace* redo = new op_wavetable_wavelevel_replace(wave, level, data);
    prepare_operation_redo(redo);
  }

  void player::wave_set_root_note(int wave, int level, int note) {
    operation_copy_flags flags;
    flags.copy_wavetable = true;
    merge_backbuffer_flags(flags);

    op_wavetable_wavelevel_replace* undo = new op_wavetable_wavelevel_replace(wave, level, back.wavetable.waves[wave]->levels[level]);
    prepare_operation_undo(undo);

    wave_level_ex data;
    data = back.wavetable.waves[wave]->levels[level];
    data.root_note = note;
    op_wavetable_wavelevel_replace* redo = new op_wavetable_wavelevel_replace(wave, level, data);
    prepare_operation_redo(redo);
  }

  void player::wave_clear_level(int wave, int level) {
    operation_copy_flags flags;
    flags.copy_wavetable = true;
    merge_backbuffer_flags(flags);

    wave_info_ex& w = *back.wavetable.waves[wave];
    int numsamples = w.get_sample_count(level);
    op_wavetable_remove_sampledata* redo = new op_wavetable_remove_sampledata(wave, level, 0, numsamples);

    op_wavetable_insert_sampledata* undo = new op_wavetable_insert_sampledata(wave, level, 0);
    int channels = w.get_stereo() ? 2 : 1;
    wave_buffer_type format = w.get_wave_format(level);
    int bytes_per_sample = sizeFromWaveFormat(format) * channels;
    int num_samples = w.get_sample_count(level);
    char* buffer = new char[bytes_per_sample * num_samples];
    memcpy(buffer, w.get_sample_ptr(level), bytes_per_sample * num_samples);
    undo->samples = buffer;
    undo->samples_channels = channels;
    undo->samples_format = format;
    undo->samples_length = num_samples;

    prepare_operation_redo(redo);
    prepare_operation_undo(undo);
  }

  void player::wave_clear(int wave) {
    // copy necessary fields to back buffer and operate there
    operation_copy_flags flags;
    flags.copy_wavetable = true;
    merge_backbuffer_flags(flags);

    int num_levels = back.wavetable.waves[wave]->levels.size();
    for (int i = 0 ; i < num_levels; i++) {
      // free sample data in levels
      wave_clear_level(wave, num_levels - i - 1);
      wave_remove_level(wave, num_levels - i - 1);

      // reset name, flags, volume, envelopes, etc
      op_wavetable_wave_replace* undo_reset = new op_wavetable_wave_replace(wave, *back.wavetable.waves[wave]);
      wave_info_ex blank_wave;
      op_wavetable_wave_replace* redo_reset = new op_wavetable_wave_replace(wave, blank_wave);
      prepare_operation_redo(redo_reset);
      prepare_operation_undo(undo_reset);
    }
  }


  void player::wave_insert_samples(int wave, int level, int target_offset, int sample_count, int channels, wave_buffer_type format, void* bytes) {
    assert(false);
  }

  void player::wave_remove_samples(int wave, int level, int target_offset, int sample_count) {
    operation_copy_flags flags;
    flags.copy_wavetable = true;
    merge_backbuffer_flags(flags);
    op_wavetable_remove_sampledata* redo = new op_wavetable_remove_sampledata(wave, level, target_offset, sample_count);
    op_wavetable_insert_sampledata* undo = new op_wavetable_insert_sampledata(wave, level, target_offset);
    wave_info_ex& w = *back.wavetable.waves[wave];
    int channels = w.get_stereo() ? 2 : 1;
    wave_buffer_type format = w.get_wave_format(level);
    int bytes_per_sample = sizeFromWaveFormat(format) * channels;
    //int num_samples = w.get_sample_count(level);
    char* buffer = new char[bytes_per_sample * sample_count];
    memcpy(buffer, w.get_sample_ptr(level, target_offset), bytes_per_sample * sample_count);
    undo->samples = buffer;
    undo->samples_channels = channels;
    undo->samples_format = format;
    undo->samples_length = sample_count;
    prepare_operation_redo(redo);
    prepare_operation_undo(undo);
  }

  void player::wave_set_envelopes(int wave, const vector<zzub::envelope_entry>& envelopes) {

    operation_copy_flags flags;
    flags.copy_wavetable = true;
    merge_backbuffer_flags(flags);

    op_wavetable_wave_replace* undo = new op_wavetable_wave_replace(wave, *back.wavetable.waves[wave]);
    prepare_operation_undo(undo);

    wave_info_ex data;
    data = *back.wavetable.waves[wave];
    data.envelopes = envelopes;
    op_wavetable_wave_replace* redo = new op_wavetable_wave_replace(wave, data);
    prepare_operation_redo(redo);
  }
} // namespace zzub
