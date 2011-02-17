// -*- c-basic-offset: 8 -*-
// dssi plugin adapter
// Copyright (C) 2006 Leonard Ritter (contact@leonard-ritter.com)
// Copyright (C) 2008 James McDermott (jamesmichaelmcdermott@gmail.com)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.


// dssidapter allows running DSSI plugins as zzub plugins


#include <dssi.h>
#include <zzub/signature.h>
#include "zzub/plugin.h"

#include <vector>
#include <string>
#include <assert.h>
#include <alsa/asoundlib.h>
#include <alsa/seq.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>
#include "dssidapter.h"


extern "C"
{
#include <lo/lo.h>
}

extern "C"
{
#include "utils.h"
}
#include "../ladspadapter/paramtools.h"

const char *myName = "zzub DSSI adaptor";

typedef ladspa_param dssi_param;

struct dssi_info : zzub::info
{
     std::string m_filename;
     std::string m_label;
     std::string m_name;
     std::string m_maker;
     std::string m_uri;
     std::vector<std::string> m_paramnames;
     std::vector<std::string> m_paramhints;
     std::vector<zzub::parameter*> m_params;
     std::vector<dssi_param> m_metaparams;
     int m_index;
     std::vector<dssi_param> m_audioins;
     std::vector<dssi_param> m_audioouts;
     std::vector<int> portIdx2ControlInIdx;

     zzub::attribute *attrMIDIChannel;
     virtual zzub::plugin* create_plugin() const;
     virtual bool store_info(zzub::archive *) const { return false; }

};

std::vector<dssi_info *> infos;

#include "osc_functions.h"

void enumerate_dssiplugin
(const char * pcFullFilename, 
 void * pvPluginHandle,
 DSSI_Descriptor_Function fDescriptorFunction)
{
     int index = 0;
     while (true)
     {
	  const DSSI_Descriptor *desc = fDescriptorFunction(index);
	  if (!desc)
	       break;
	  dssi_info *i = new dssi_info();
	  infos.push_back(i);
	  i->m_filename = pcFullFilename;
	  i->m_index = index;
	  i->m_label = desc->LADSPA_Plugin->Label;
	  i->m_name = desc->LADSPA_Plugin->Name;		
	  i->m_maker = desc->LADSPA_Plugin->Maker;
	  // We set this flag even if it's not true, because otherwise Aldrin
	  // gets confused and thinks the Oscilloscope is a generator?
	  i->flags = zzub::plugin_flag_has_audio_output;
	  i->name = strdup(i->m_name.c_str());
	  i->min_tracks = 1;
	  i->max_tracks = 16;
	  i->short_name = strdup(i->m_label.c_str());
	  i->author = strdup(i->m_maker.c_str());
		
	  i->m_uri = "@zzub.org/dssidapter/" + i->m_label;
	  replace(i->m_uri.begin(), i->m_uri.end(), ' ', '+');
	  i->attrMIDIChannel = &i->add_attribute()
	       .set_name("MIDI Channel (0=off)")
	       .set_value_min(0)
	       .set_value_max(16)
	       .set_value_default(0);

	  for (int port = 0; port < desc->LADSPA_Plugin->PortCount; port++)
	  {
	       std::string name = desc->LADSPA_Plugin->PortNames[port];
	       LADSPA_PortDescriptor pd = desc->LADSPA_Plugin->PortDescriptors[port];
	       LADSPA_PortRangeHint hint = desc->LADSPA_Plugin->PortRangeHints[port];
	       dssi_param mp;
	       mp.index = port;
	       mp.hint = hint;
	       if (LADSPA_IS_PORT_CONTROL(pd))
	       {
		    if (LADSPA_IS_PORT_INPUT(pd)) {
			 i->portIdx2ControlInIdx.push_back(i->m_metaparams.size());
			 zzub::parameter *param = new zzub::parameter();
			 i->global_parameters.push_back(param);
			 i->m_params.push_back(param);
			 i->m_paramnames.push_back(name);
			 param->name = strdup(i->m_paramnames[i->m_paramnames.size()-1].c_str());
			 param->description = param->name;
			 setup_ladspa_parameter(param, hint, mp);
				
			 i->m_metaparams.push_back(mp);
		    } else {
			 i->portIdx2ControlInIdx.push_back(-1);
		    }
	       }
	       else if (LADSPA_IS_PORT_AUDIO(pd))
	       {
		    i->portIdx2ControlInIdx.push_back(-1);
		    if (LADSPA_IS_PORT_INPUT(pd))
		    {
			 i->m_audioins.push_back(mp);
		    }
		    else if (LADSPA_IS_PORT_OUTPUT(pd))
		    {
			 i->m_audioouts.push_back(mp);
		    }
	       }
	  }
	  i->m_params.push_back(&param_note);
	  i->track_parameters.push_back(&param_note);
	  i->m_params.push_back(&param_volume);
	  i->track_parameters.push_back(&param_volume);
	  i->uri = strdup(i->m_uri.c_str());

	  if (i->m_audioins.size()) {
	       i->flags |= zzub::plugin_flag_has_audio_input;
	  }
	  if (verbose) printf("plugin %s: flags: input = %d, output = %d\n", i->name.c_str(),
			      i->flags & zzub::plugin_flag_has_audio_input,
			      i->flags & zzub::plugin_flag_has_audio_output);
	  if (verbose) {
	       printf("plugin %s portIdx2ControlInIdx:", i->name.c_str());
	       for (int port = 0; port < desc->LADSPA_Plugin->PortCount; port++)
		    printf("%d ", i->portIdx2ControlInIdx[port]);
	       printf("\n");
	  }
			
	  index++;
     }
}


dssidapter::~dssidapter()
{
     // If the UI is open, tell it to quit.
     if (uiTarget && ui_osc_quit_path) {
	  lo_send(uiTarget, ui_osc_quit_path, "");
     }
     if (handle)
     {
	  if (desc->LADSPA_Plugin->deactivate)
	  {
	       desc->LADSPA_Plugin->deactivate(handle);
	  }
	  if (desc->LADSPA_Plugin->cleanup)
	  {
	       desc->LADSPA_Plugin->cleanup(handle);
	  }
     }
     unloadDSSIPluginLibrary(library);
     if (globals)
     {
	  delete[] globals;
     }
     if (data_values)
     {
	  delete[] data_values;
     }
     if (events)
     {
	  delete[] events;
     }
}
	
dssidapter::dssidapter(const dssi_info *_dssi_info)
{
     if (verbose) printf("plugin <%s>: in constructor!\n", _dssi_info->m_name.c_str());
     if (verbose) printf("arc_key_str = %p\n", arc_key_str);
     eventcount = 0;
     events = new snd_seq_event_t[256];
     globals = 0;
     global_values = 0;
     data_values = 0;
     myinfo = _dssi_info;
     track_count = 1;

     if (myinfo->global_parameters.size())
     {
	  int size = 0;
	  std::vector<dssi_param>::const_iterator i;
	  for (i = myinfo->m_metaparams.begin(); i != myinfo->m_metaparams.end(); ++i)
	  {
	       switch (i->param->type) {
	       case parameter_type_switch: size += 1; break;
	       case parameter_type_byte: size += 1; break;
	       case parameter_type_word: size += 2; break;
	       default: break;
	       }
	  }
	  globals = new char[size];
	  global_values = globals;
	  data_values = new LADSPA_Data[myinfo->global_parameters.size()];
     }
     track_values = trackvals;
     attributes = (int *)&aval;
     library = loadDSSIPluginLibrary(myinfo->m_filename.c_str());
     desc = findDSSIPluginDescriptor(library, myinfo->m_filename.c_str(), myinfo->m_label.c_str());
     handle = 0;



     uiTarget = NULL;
     uiSource = NULL;
     ui_initial_show_sent = 0;
     uiNeedsProgramUpdate = 0;
     ui_osc_control_path = NULL;
     ui_osc_program_path = NULL;
     ui_osc_quit_path = NULL;
     ui_osc_show_path = NULL;

     pluginProgramCount = 0;
     pluginPrograms = NULL;
     currentBank = 0;
     currentProgram = 0;
     pendingBankLSB = -1;
     pendingBankMSB = -1;
     pendingProgramChange = -1;
			
     arc_program_idx = arc_key_count = arc_key_size = 0;

}
	
bool dssidapter::invoke(zzub_event_data_t& data)
{
     if (data.type == zzub::event_type_double_click) {
	  if (uiTarget && ui_osc_show_path) {
	       // The previous GUI never exited, so just show it and return.
	       lo_send(uiTarget, ui_osc_show_path, "");
	       return true;
	  }

	  size_t dpos = myinfo->m_filename.find_last_of('/');
	  std::string directory, filename;
	  if (dpos != std::string::npos) {
	       directory = myinfo->m_filename.substr(0, dpos);
	       filename = myinfo->m_filename.substr(dpos+1, myinfo->m_filename.length()-4-dpos);
	       if (verbose) printf("directory = %s, filename = %s\n", directory.c_str(), filename.c_str());
	       std::string searchpath;
	       searchpath = directory + std::string("/") + filename + std::string("/");
	       struct stat sdir;
	       int res = stat(searchpath.c_str(), &sdir);
	       std::string label = desc->LADSPA_Plugin->Label;
	       if (res != 0) {
		    searchpath = directory + std::string("/") + label + std::string("/");
		    res = stat(searchpath.c_str(), &sdir);
	       }
	       if (res != 0)
		    return false;
	       struct dirent **namelist;
	       struct stat statinfo;
	       int n;
				
	       std::string guipath;
				
	       n = scandir(searchpath.c_str(), &namelist, 0, 0);
	       if (n >= 0) {
		    while(n--) {
			 std::string fullfilepath = searchpath + namelist[n]->d_name;
			 if (!stat(fullfilepath.c_str(), &statinfo))
			 {
			      if (!S_ISDIR(statinfo.st_mode))
			      {
				   std::string fullname=namelist[n]->d_name;					
				   int dpos=(int)fullname.find_last_of('_');
				   if (dpos != std::string::npos) {
					if (verbose) printf("using %s\n", fullfilepath.c_str());
					guipath = fullfilepath;
				   }
			      }
			 }
			 free(namelist[n]);
		    }
		    free(namelist);
	       }


	       /* Create OSC thread -- FIXME this makes Aldrin unresponsive for a few seconds */
	       /* when using liblo 0.23 -- seems to be an IPv6 misconfigure. liblo 0.24 avoids the */
	       /* problem */
	       serverThread = lo_server_thread_new(NULL, osc_error);
	       char osc_path_tmp[1024];

	       snprintf((char *)osc_path_tmp, 31, "/dssi");
	       tmp = lo_server_thread_get_url(serverThread);
	       osc_url = (char *)malloc(strlen(tmp) + strlen(osc_path_tmp));
	       sprintf(osc_url, "%s%s", tmp, osc_path_tmp + 1);
	       if (verbose) {
		    printf("%s: registering %s\n", myName, osc_url);
	       }
	       free(tmp);

	       lo_server_thread_add_method(serverThread, NULL, NULL, osc_message_handler,
					   this);
	       lo_server_thread_start(serverThread);
				
	       if (guipath.length() && osc_url) {
		    std::string sofile = filename + std::string(".so");	
		    if (!fork()) {
			 execl(guipath.c_str(), 
			       guipath.c_str(), 
			       osc_url, 
			       sofile.c_str(), 
			       label.c_str(), 
			       _host->get_name(_metaplugin),
			       NULL);
		    }
	       }

	  }
	  return true;
     }
     return false;
}

void dssidapter::destroy()
{
     if (verbose) printf("in destroy\n");
     // do nothing, because "delete this;" caused a double-free
}
	
void dssidapter::init(archive *arc)
{
     if (verbose) printf("in init()!\n");
     _metaplugin = _host->get_metaplugin();
     _host->set_event_handler(_metaplugin, this);
     handle = desc->LADSPA_Plugin->instantiate(desc->LADSPA_Plugin, _master_info->samples_per_second);
     if (!desc->run_synth && !desc->run_multiple_synths) {
	  printf("%s: Warning: no run_synth() method and no run_multiple_synth() method\n", myName);
     }
     if (myinfo->m_audioouts.size() < 1) {
	  printf("%s: Warning: no audio outputs\n", myName);
     }
     std::vector<dssi_param>::const_iterator i;
     int index = 0;
     for (i = myinfo->m_metaparams.begin(); i != myinfo->m_metaparams.end(); ++i)
     {
	  if (getLADSPADefault(&i->hint, _master_info->samples_per_second, &data_values[index]) == -1)
	  {
	       data_values[index] = 0.0f;
	  }
	  desc->LADSPA_Plugin->connect_port(handle, i->index, &data_values[index]);
	  index++;
     }
     index = 0;
     for (i = myinfo->m_audioins.begin(); i != myinfo->m_audioins.end(); ++i)
     {
	  desc->LADSPA_Plugin->connect_port(handle, i->index, inputs[index]);
	  memset(inputs[index],0,sizeof(float)*256);
	  index++;
     }
     index = 0;
     for (i = myinfo->m_audioouts.begin(); i != myinfo->m_audioouts.end(); ++i)
     {
	  desc->LADSPA_Plugin->connect_port(handle, i->index, outputs[index]);
	  memset(inputs[index],0,sizeof(float)*256);
	  index++;
     }
		
     // read from the archive
     if (arc) {
	  read_from_archive(arc);
     } else {
	  arc_program_idx = 0;
	  arc_key_count = 0;
     }

     if (desc->configure) {
	  char *message;
	  for (int i = 0; i < arc_key_count; i++) {
	       if (verbose) printf("calling configure(): key <%s> value <%s>\n", arc_key[i], arc_value[i]);
	       message = desc->configure(handle, arc_key[i], arc_value[i]);
	  }
     }

     /* Look up synth programs */
			
     query_programs(this);


     //
     if (desc->select_program && pluginProgramCount > 0 && !arc) {
	  if (verbose) printf("in init, selecting program %d\n", arc_program_idx);
	  /* select program at index read from the save file (or 0 if none) */
	  unsigned long bank = pluginPrograms[arc_program_idx].Bank;
	  pendingBankMSB = bank / 128;
	  pendingBankLSB = bank % 128;
	  pendingProgramChange = pluginPrograms[arc_program_idx].Program;
	  uiNeedsProgramUpdate = 1;
     } else {
	  if (verbose) printf("in init, not using control changes\n");
	  // printf("in init, using control changes\n");
	  // std::vector<dssi_param>::const_iterator i;
	  // int index = 0;
	  // for (i = myinfo->m_metaparams.begin(); i != myinfo->m_metaparams.end(); ++i) {
	  // 	if (getLADSPADefault(&i->hint, _master_info->samples_per_second, 
	  // 			     &data_values[index]) == -1) {
	  // 		data_values[index] = 0.0f;
	  // 	}
	  // 	printf("in init, data_values[%d] = %f\n", index, data_values[index]);
	  // }
     }
			
     if (desc->LADSPA_Plugin->activate)
	  desc->LADSPA_Plugin->activate(handle);

     eventcount = 0;
}

void dssidapter::read_from_archive(archive *arc) {
     zzub::instream *pi = arc->get_instream("");

     // I think we don't need bank and program because arc_program_idx deals with that.
     // unsigned long bank, program;
     // pi->read(&bank, sizeof(unsigned long));
     // pi->read(&program, sizeof(unsigned long));
     // schedule_program_change(this, bank, program);

     pi->read(&arc_program_idx, sizeof(unsigned long));
     if (verbose) printf("loading: arc_program_idx = %d\n", arc_program_idx);
     pi->read(&arc_key_count, sizeof(unsigned long));
     if (verbose) printf("loading: arc_key_count = %d\n", arc_key_count);
     pi->read(&arc_key_size, sizeof(unsigned long));
     if (verbose) printf("loading: arc_key_size = %d\n", arc_key_size);
     pi->read(arc_key_str, sizeof(char) * arc_key_size);
     arc_key_str[arc_key_size] = '\0';
     if (verbose) printf("loading: arc_key_str <%s>\n", arc_key_str);
     int idx = 0;
     for (int i = 0; i < arc_key_count; i++) {
	  sscanf(&arc_key_str[idx], "%s\n", arc_key[i]);
	  idx += strlen(arc_key[i]) + 1;
	  sscanf(&arc_key_str[idx], "%s\n", arc_value[i]);
	  idx += strlen(arc_value[i]) + 1;
	  if (verbose) printf("loading: key <%s> value <%s>\n", arc_key[i], arc_value[i]);
     }
}

void dssidapter::process_events()
{
     std::vector<dssi_param>::const_iterator i;
     int index = 0;
     int offset = 0;
     for (i = myinfo->m_metaparams.begin(); i != myinfo->m_metaparams.end(); ++i)
     {
	  //printf("in metaparam loop\n");
	  int value = 0;
	  switch (i->param->type) {
	  case parameter_type_switch: value = *(unsigned char *)&globals[offset]; offset += 1; break;
	  case parameter_type_byte: value = *(unsigned char *)&globals[offset]; offset += 1; break;
	  case parameter_type_word: value = *(unsigned short *)&globals[offset]; offset += 2; break;
	  default: break;
	  }
	  if (value != i->param->value_none)
	  {
	       data_values[index] = convert_ladspa_value(*i, value, _master_info->samples_per_second);
	       printf("dssidapter: process_events(): data_values[%d] = %f, zzub value = %d\n", index, data_values[index], value);
	       for (int j = 0; j < myinfo->portIdx2ControlInIdx.size(); j++) {
		    if (myinfo->portIdx2ControlInIdx[j] == index) {
			 if (uiTarget && ui_osc_control_path) {
			      lo_send(uiTarget, ui_osc_control_path, "if", j, data_values[index]);
			 }
		    }
	       }

	  }
	  index++;
     }
     // Don't zero eventcount here: it could have been
     // incremented in midi_note or set_track_count.
     // Instead, zero it in process_stereo after processing
     // events.
     // eventcount = 0;
     for (int t = 0; t < track_count; t++) {
	  tvals &tv = trackvals[t];
	  tstate &ts = trackstates[t];
	  if (tv.note != zzub::note_value_none) {
	       //printf("in track_count tv.note branch\n");
	       // Turn off any existing note
	       if (ts.note != zzub::note_value_none) {
		    schedule_note_off(t);
	       }
	       // Add new note.
	       snd_seq_event_t &ev = events[eventcount];
	       memset(&ev, 0, sizeof(snd_seq_event_t));
	       if (tv.note != zzub::note_value_off) {
		    ev.type = SND_SEQ_EVENT_NOTEON;
		    ev.data.note.note = (tv.note>>4)*12 + (tv.note&0xf)-1;
		    ev.data.note.velocity = 127;
		    ts.note = tv.note;
		    eventcount++;
	       }				
	  }
     }
}
	
const char * dssidapter::describe_value(const int param, const int value)
{ 
     static char text[256];
     if (param < myinfo->m_metaparams.size()) {
	  dssi_param mp = myinfo->m_metaparams[param];
	  return describe_ladspa_value(mp, value, _master_info->samples_per_second, text);
     } 
     return 0; 
}

void dssidapter::set_track_count(int ntracks)
{
     if (ntracks < track_count) {
	  for (int t = ntracks; t < track_count; t++) {
	       tstate &ts = trackstates[t];
	       if (ts.note != zzub::note_value_none) {
		    schedule_note_off(t);
	       }
	  }
     }
     track_count = ntracks;
}

void dssidapter::stop() {
     for (int t = 0; t < track_count; t++) {
	  tstate &ts = trackstates[t];
	  if (ts.note != zzub::note_value_none) {
	       schedule_note_off(t);
	  }
     }
}

void dssidapter::schedule_note_off(int t) {
     // ts.note is the note playing on track t
     tvals &tv = trackvals[t];
     tstate &ts = trackstates[t];
     snd_seq_event_t &ev = events[eventcount];
     memset(&ev, 0, sizeof(snd_seq_event_t));
     ev.type = SND_SEQ_EVENT_NOTEOFF;
     ev.data.note.note = (ts.note>>4)*12 + (ts.note&0xf)-1;
     ts.note = zzub::note_value_none;
     eventcount++;
}

bool dssidapter::process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) { return false; }	
		
bool dssidapter::process_stereo(float **pin, float **pout, int numsamples, int const mode)
{
     //process pending program changes

     if (pendingProgramChange >= 0) {
	  if (verbose)
	       printf("dssidapter: pendingProgramChange = %d\n", pendingProgramChange);

	  int pc = pendingProgramChange;
	  int msb = pendingBankMSB;
	  int lsb = pendingBankLSB;

	  if (lsb >= 0) {
	       if (msb >= 0) {
		    currentBank = lsb + 128 * msb;
	       } else {
		    currentBank = lsb + 128 * (currentBank / 128);
	       }
	  } else if (msb >= 0) {
	       currentBank = (currentBank % 128) + 128 * msb;
	  }

	  currentProgram = pc;
				
	  pendingProgramChange = -1;
	  pendingBankMSB = -1;
	  pendingBankLSB = -1;

	  if (desc->select_program) {
	       // FIXME after this call, the
	       // host should re-read the
	       // control-in values, because
	       // the plugin may have
	       // re-written them. Host
	       // should save those
	       // control-in values as the
	       // current values.
	       if (verbose) printf("process_stereo: calling select_program(): currentBank = %d, currentProgram = %d\n", currentBank, currentProgram);
	       desc->select_program(handle, currentBank, currentProgram);
	  }
     }
	
     if (mode == zzub::process_mode_no_io)
     {
	  if (verbose) printf("dssidapter: process_stereo() returning false\n");
	  return false;
     }
     if (myinfo->m_audioins.size() == 1) {
	  for (int i = 0; i < numsamples; i++) {
	       inputs[0][i] = (pin[0][i] + pin[1][i]) * 0.5f;
	  }
     } else if (myinfo->m_audioins.size() >= 2) {
	  // FIXME if it's greater than 2, should mix some of them.
	  memcpy(inputs[0], pin[0], sizeof(float) * numsamples);
	  memcpy(inputs[1], pin[1], sizeof(float) * numsamples);
     } else {
	  // No audio inputs -- no problem.
     }

     if (desc->run_synth) {
	  desc->run_synth(handle, numsamples, events, eventcount);
     } else if (desc->run_multiple_synths) {
	  desc->run_multiple_synths(1, &handle, numsamples, &events, &eventcount);
     } else {
	  // No synth method -- what should we return?
     }

     eventcount = 0;
     if (myinfo->m_audioouts.size() == 1) {
	  memcpy(pout[0], outputs[0], sizeof(float) * numsamples);
	  memcpy(pout[1], outputs[0], sizeof(float) * numsamples);
     } else if (myinfo->m_audioouts.size() >= 2) {
	  // FIXME if it's greater than 2, should mix some of them.
	  memcpy(pout[0], outputs[0], sizeof(float) * numsamples);
	  memcpy(pout[1], outputs[1], sizeof(float) * numsamples);
     } else {
	  // No audio outputs -- return what?
     }
     return true; 
}

void dssidapter::midi_note(int channel, int value, int velocity)
{
     if (verbose) {
	  printf("midi_note: %d %d %d\n", channel, value, velocity);
     }
     if (channel != aval.channel) {
	  if (verbose) printf("midi note doesn't match aval channel (%d): returning\n", aval.channel);
	  return;
     }

     snd_seq_event_t &ev = events[eventcount];
     memset(&ev, 0, sizeof(snd_seq_event_t));
     if (velocity != 0) {
	  ev.type = SND_SEQ_EVENT_NOTEON;
	  ev.data.note.note = value; //(value >> 4) * 12 + (value & 0xf) - 1;
	  ev.data.note.velocity = velocity;
	  eventcount++;
     } else {
	  ev.type = SND_SEQ_EVENT_NOTEOFF;
	  ev.data.note.note = value; //(value >> 4) * 12 + (value & 0xf) - 1;
	  ev.data.note.velocity = velocity;
	  eventcount++;
     }
     return;
}
void dssidapter::midi_note_off(int channel, int value, int velocity)
{
     if (verbose) {
	  printf("midi_note_off: %d %d %d\n", channel, value, velocity);
     }
     if (channel != aval.channel) {
	  if (verbose) printf("midi note-off doesn't match aval channel (%d): returning\n", aval.channel);
	  return;
     }

     snd_seq_event_t &ev = events[eventcount];
     memset(&ev, 0, sizeof(snd_seq_event_t));
     ev.type = SND_SEQ_EVENT_NOTEOFF;
     ev.data.note.note = value; //(value >> 4) * 12 + (value & 0xf) - 1;
     ev.data.note.velocity = velocity;
     eventcount++;
     return;
}



void dssidapter::load(zzub::archive *arc) {
     // This is called when user selects a new preset in Aldrin.
     if (verbose) printf("dssidapter: in load()!\n");
     if (arc) {
	  read_from_archive(arc);
     } else {
	  arc_program_idx = 0;
	  arc_key_count = 0;
     }
}

void dssidapter::save(zzub::archive *arc) {	
     // in osc_configure_handler we see what keys there are,
     // and for each key save the *latest* value.
     // then in update_handler we actually send the keys that we got in init()
     if (verbose) printf("dssidapter: in save()!\n");
     zzub::outstream *po = arc->get_outstream("");
     po->write(&arc_program_idx, sizeof(unsigned long));
     if (verbose) printf("wrote arc_program_idx = %d\n", arc_program_idx);
     po->write(&arc_key_count, sizeof(unsigned long));
     int idx = 0;
     for (int i = 0; i < arc_key_count; i++) {
	  if (verbose) printf("saving key <%s> value <%s>\n", arc_key[i], arc_value[i]);
	  // each sprintf() call overwrites the terminating '\0' of the previous,
	  // so we end up with a '\n'-separated, '\0'-terminated string.
	  idx += sprintf(&arc_key_str[idx], "%s\n", arc_key[i]);
	  idx += sprintf(&arc_key_str[idx], "%s\n", arc_value[i]);
     }
     arc_key_size = idx;
     if (verbose) printf("total size = %d, str <%s>\n", arc_key_size, arc_key_str);
     po->write(&arc_key_size, sizeof(unsigned long));
     po->write(&arc_key_str, sizeof(char) * arc_key_size);

}


float ipol_exp(float v1, float v2, float y) {
	if (v1 == 0.0f)
		v1 = -8; // -48dB or so
	else
		v1 = log(v1);
	v2 = log(v2);
	//return exp(v1*(1-x) + v2*x);
	return log(y/exp(v1))/(v2 - v1);
}


int convert_ladspa_to_zzub(const ladspa_param &mp, float value, float sps) {
	zzub::parameter *param = mp.param;
	LADSPA_PortRangeHintDescriptor hd = mp.hint.HintDescriptor;
	float lb,ub;
	lb = mp.lb;
	ub = mp.ub;
//	if (LADSPA_IS_HINT_INTEGER(hd)) {
//		value = (int)(value - 0.5f);
//	} else if (LADSPA_IS_HINT_TOGGLED(hd)) {
//		value = (int)(value + 0.5f);
//	}
	if (mp.sr)
	{
		lb *= sps;
		ub *= sps;
	}
	float x;
	int v;
	//printf("is int %d value %f  lb %f ub %f \n", LADSPA_IS_HINT_INTEGER(hd), value, lb, ub);	
	if (LADSPA_IS_HINT_LOGARITHMIC(hd)) {
		//reverse of v = ipol_log(lb,ub,x);
		x = ipol_exp(lb, ub, value);
	} else {
		//reverse of v = lb + x * (ub - lb);
		x = (value - lb) / (ub - lb);
	}
	v = param->value_min + x * (param->value_max - param->value_min);
	//printf("v %d x %f pmin %d pmax %d\n", v, x , param->value_min, param->value_max);		
	return v;
}


void dssidapter::set_parameter_value_dssi(int port, float value) {
	int val = 0;
    std::vector<dssi_param>::const_iterator i;
    int index = 0;
    int offset = 0;
    for (i = myinfo->m_metaparams.begin(); i != myinfo->m_metaparams.end(); ++i)
    {
     	if (index == myinfo->portIdx2ControlInIdx[port]) 
     		break;
     	index++;
     }
    printf("%f\n", value);
	val = convert_ladspa_to_zzub(*i, value, _master_info->samples_per_second);
	_host->control_change(_metaplugin, 1, 0, myinfo->portIdx2ControlInIdx[port], val, false, true);
}


void dssidapter::process_controller_events() {}
void dssidapter::attributes_changed() {}
void dssidapter::command(int) {}
void dssidapter::mute_track(int) {}
bool dssidapter::is_track_muted(int) const { return false; }
void dssidapter::event(unsigned int) {}
const zzub::envelope_info** dssidapter::get_envelope_infos() { return 0; }
bool dssidapter::play_wave(int, int, float) { return false; }
void dssidapter::stop_wave() {}
int dssidapter::get_wave_envelope_play_position(int) { return -1; }
const char* dssidapter::describe_param(int) { return 0; }
bool dssidapter::set_instrument(const char*) { return false; }
void dssidapter::get_sub_menu(int, zzub::outstream*) {}
void dssidapter::add_input(const char*) {}
void dssidapter::delete_input(const char*) {}
void dssidapter::rename_input(const char*, const char*) {}
void dssidapter::input(float**, int, float) {}
void dssidapter::midi_control_change(int, int, int) {}
bool dssidapter::handle_input(int, int, int) { return false; }


zzub::plugin* dssi_info::create_plugin() const {
     return new dssidapter(this);
}

struct dssiplugincollection : zzub::plugincollection {
     // Called by the host initially. The collection registers
     // plugins through the pluginfactory::register_info method.
     // The factory pointer remains valid and can be stored
     // for later reference.
     virtual void initialize(zzub::pluginfactory *factory) {
	  if (verbose) printf("initializing dssidapter...\n");
	  DSSIPluginSearch(enumerate_dssiplugin);
	  for (int i=0; i < infos.size(); ++i)
	  {
	       factory->register_info(infos[i]);
	  }	
	  if (verbose) printf("dssidapter: enumerated %i plugin(s).\n", infos.size());
     }
	
     // Called by the host upon song loading. If the collection
     // can not provide a plugin info based on the uri or
     // the metainfo passed, it should return a null pointer.
     virtual const zzub::info *get_info(const char *uri, zzub::archive *data) { return 0; }
	
     // Returns the uri of the collection to be identified,
     // return zero for no uri. Collections without uri can not be 
     // configured.
     virtual const char *get_uri() { return 0; }
	
     // Called by the host to set specific configuration options,
     // usually related to paths.
     virtual void configure(const char *key, const char *value) {}

     // Called by the host upon destruction. You should
     // delete the instance in this function
     virtual void destroy() { 
	  if (verbose) printf("in dssiplugincollection::destroy()\n");
	  delete this; 
     }
};

zzub::plugincollection *zzub_get_plugincollection() {
     return new dssiplugincollection();
}

const char *zzub_get_signature() { return ZZUB_SIGNATURE; }


void
query_programs(struct dssidapter *instance)
{
     int i;

     /* free old lot */
     if (instance->pluginPrograms) {
	  for (i = 0; i < instance->pluginProgramCount; i++)
	       free((void *)instance->pluginPrograms[i].Name);
	  free((char *)instance->pluginPrograms);
	  instance->pluginPrograms = NULL;
	  instance->pluginProgramCount = 0;
     }

     instance->pendingBankLSB = -1;
     instance->pendingBankMSB = -1;
     instance->pendingProgramChange = -1;

     if (instance->desc->get_program &&
	 instance->desc->select_program) {

	  /* Count the programs first */
	  for (i = 0; instance->desc->get_program(instance->handle, i); ++i);

	  if (i > 0) {
	       instance->pluginProgramCount = i;
	       instance->pluginPrograms = (DSSI_Program_Descriptor *)
		    malloc(i * sizeof(DSSI_Program_Descriptor));
	       while (i > 0) {
		    const DSSI_Program_Descriptor *descriptor;
		    --i;
		    descriptor = instance->desc->get_program(instance->handle, i);
		    instance->pluginPrograms[i].Bank = descriptor->Bank;
		    instance->pluginPrograms[i].Program = descriptor->Program;
		    instance->pluginPrograms[i].Name = strdup(descriptor->Name);
		    if (verbose) {
			 printf("%s: %s program %d is MIDI bank %lu program %lu, named '%s'\n",
				myName, instance->myinfo->m_name.c_str(), i,
				instance->pluginPrograms[i].Bank,
				instance->pluginPrograms[i].Program,
				instance->pluginPrograms[i].Name);
		    }
	       }
	  }
     }
}




