/*
  Copyright (C) 2003-2007 Anders Ervik <calvin@countzero.no>
  
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

#pragma once

#include <errno.h> 
#include <pthread.h>
#include <unistd.h> 
#include <semaphore.h> 

#if defined(__powerpc__) || defined(__POWERPC__) || defined(_M_PPC) || defined(__BIG_ENDIAN__)
#define ZZUB_BIG_ENDIAN
#endif

#define strcmpi strcasecmp

#define _USE_MATH_DEFINES
#include <cmath>
#include <cassert>
#include <cstring>
#include <map>
#include <list>
#include <stack>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/depth_first_search.hpp>

#define NO_ZZUB_AUDIODRIVER_TYPE
#define NO_ZZUB_MIDIDRIVER_TYPE
#define NO_ZZUB_PLUGINCOLLECTION_TYPE
#define NO_ZZUB_INPUT_TYPE
#define NO_ZZUB_OUTPUT_TYPE
#define NO_ZZUB_ARCHIVE_TYPE
#define NO_ZZUB_MIDIMAPPING_TYPE
#define NO_ZZUB_PATTERN_TYPE
#define NO_ZZUB_PARAMETER_TYPE
#define NO_ZZUB_ATTRIBUTE_TYPE
#define NO_ZZUB_PLUGINLOADER_TYPE
#define NO_ZZUB_PLUGIN_TYPE
#define NO_ZZUB_SEQUENCE_TYPE
#define NO_ZZUB_WAVELEVEL_TYPE
#define NO_ZZUB_ENVELOPE_TYPE
#define NO_ZZUB_WAVE_TYPE
#define NO_ZZUB_RECORDER_TYPE
#define NO_ZZUB_PLAYER_TYPE

struct zzub_flatapi_player;

namespace zzub {
  struct metaplugin_proxy;
  struct info;
  struct pattern;
  struct event_connection_binding;
  struct wave_proxy;
  struct wavelevel_proxy;
  struct sequence_proxy;
  struct parameter;
  struct attribute;
  struct envelope_entry;
  struct midimapping;
  struct recorder;
  struct pluginlib;
  struct mem_archive;
  struct audiodriver;
  struct mididriver;
  struct instream;
  struct outstream;
}

// internal types
typedef zzub_flatapi_player zzub_player_t;
typedef zzub::audiodriver zzub_audiodriver_t;
typedef zzub::mididriver zzub_mididriver_t;
typedef zzub::metaplugin_proxy zzub_plugin_t;
typedef const zzub::info zzub_pluginloader_t;
typedef zzub::pluginlib zzub_plugincollection_t;
typedef zzub::pattern zzub_pattern_t;
typedef zzub::sequence_proxy zzub_sequence_t;
typedef zzub::event_connection_binding zzub_event_connection_binding_t;
typedef zzub::wave_proxy zzub_wave_t;
typedef zzub::wavelevel_proxy zzub_wavelevel_t;
typedef zzub::parameter zzub_parameter_t;
typedef zzub::attribute zzub_attribute_t;
typedef zzub::envelope_entry zzub_envelope_t;
typedef zzub::midimapping zzub_midimapping_t;
typedef zzub::recorder zzub_recorder_t;
typedef zzub::mem_archive zzub_archive_t;
typedef zzub::instream zzub_input_t;
typedef zzub::outstream zzub_output_t;


#include "synchronization.h"
#include "zzub/plugin.h"
#include "pluginloader.h"
#include "timer.h"
#include "driver.h"
#include "midi.h"
#include "wavetable.h"
#include "input.h"
#include "output.h"
#include "master.h"
#include "recorder.h"
#include "graph.h"
#include "song.h"
#include "undo.h"
#include "operations.h"
#include "thread_id.h"
#include "player.h"
#include "connections.h"

#define _unused(x) ((void)x)
