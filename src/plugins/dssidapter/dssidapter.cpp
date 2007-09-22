// dssi plugin adapter
// Copyright (C) 2006 Leonard Ritter (contact@leonard-ritter.com)
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
#include 	"zzub/plugin.h"
#include <vector>
#include <string>
#include <assert.h>

#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

extern "C"
{
#include "utils.h"
}

#include "../ladspadapter/paramtools.h"


using namespace zzub;

static struct _param_note : parameter {
	_param_note() {
		set_note();
	}
} param_note;

static struct _param_volume : parameter {
	_param_volume() {
		set_byte();
		set_name("Volume");
		set_description("Volume (00-7F)");
		set_value_min(0);
		set_value_max(0x7F);
		set_value_none(0xFF);
		set_value_default(0);
	}
} param_volume;

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
	
	virtual zzub::plugin* create_plugin() const;
	virtual bool store_info(zzub::archive *) const { return false; }
};

std::vector<dssi_info *> infos;

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
		i->type = zzub::plugin_type_generator;
		i->name = i->m_name.c_str();
		i->min_tracks = 1;
		i->max_tracks = 16;
		i->short_name = i->m_label.c_str();
		i->author = i->m_maker.c_str();
		
		i->m_uri = "@zzub.org/dssidapter/" + i->m_label;
		replace(i->m_uri.begin(), i->m_uri.end(), ' ', '+');
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
				zzub::parameter *param = new zzub::parameter();
				i->global_parameters.push_back(param);
				i->m_params.push_back(param);
				i->m_paramnames.push_back(name);
				param->name = i->m_paramnames[i->m_paramnames.size()-1].c_str();
				param->description = param->name;
				setup_ladspa_parameter(param, hint, mp);
				
				i->m_metaparams.push_back(mp);
			}
			else if (LADSPA_IS_PORT_AUDIO(pd))
			{
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
		i->uri = i->m_uri.c_str();
		//~ for (int p = 0; p < i->params.size(); ++i)
		//~ {
		//~ }			
		index++;
	}
}


struct tvals {
	unsigned char note;
	unsigned char volume;
};

struct tstate {
	unsigned char note;
	
	tstate() {
		note = zzub::note_value_none;
	}
};

struct dssidapter : plugin, event_handler
{
	char *globals;
	tvals trackvals[16];
	tstate trackstates[16];
	const dssi_info *myinfo;
	const DSSI_Descriptor *desc;
	void *library;
	LADSPA_Handle handle;
	LADSPA_Data *data_values;
	float inputs[16][256];
	float outputs[16][256];
	snd_seq_event_t events[256];
	int eventcount;
	zzub::metaplugin*_metaplugin;
	
	~dssidapter()
	{
		if (handle)
		{
			if (desc->LADSPA_Plugin->deactivate)			
				desc->LADSPA_Plugin->deactivate(handle);
			desc->LADSPA_Plugin->cleanup(handle);
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
	}
	
	dssidapter(const dssi_info *_dssi_info)
	{
		eventcount = 0;
		globals = 0;
		global_values = 0;
		data_values = 0;
		myinfo = _dssi_info;		
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
		attributes = 0;
		library = loadDSSIPluginLibrary(myinfo->m_filename.c_str());
		desc = findDSSIPluginDescriptor(library, myinfo->m_filename.c_str(), myinfo->m_label.c_str());
		handle = 0;
	}
	
	virtual bool invoke(zzub_event_data_t& data)
	{
		if (data.type == zzub::event_type_double_click) {
			size_t dpos = myinfo->m_filename.find_last_of('/');
			std::string directory, filename;
			if (dpos != std::string::npos) {
				directory = myinfo->m_filename.substr(0, dpos);
				filename = myinfo->m_filename.substr(dpos+1, myinfo->m_filename.length()-4-dpos);
				printf("directory = %s, filename = %s\n", directory.c_str(), filename.c_str());
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
									printf("using %s\n", fullfilepath.c_str());
									guipath = fullfilepath;
								}
							}
						}
						free(namelist[n]);
					}
					free(namelist);
				}
				
				char osc_url[1024];
				if (guipath.length() && _host->get_osc_url(_metaplugin,osc_url)) {
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

	virtual void destroy()
	{
		delete this;
	}
	
	virtual void init(archive *)
	{
		_metaplugin = _host->get_metaplugin();
		_host->set_event_handler(_metaplugin, this);
		handle = desc->LADSPA_Plugin->instantiate(desc->LADSPA_Plugin, _master_info->samples_per_second);
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
		
		if (desc->LADSPA_Plugin->activate)
			desc->LADSPA_Plugin->activate(handle);
	}
	
	virtual void process_events()
	{
		std::vector<dssi_param>::const_iterator i;
		int index = 0;
		int offset = 0;
		for (i = myinfo->m_metaparams.begin(); i != myinfo->m_metaparams.end(); ++i)
		{
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
			}
			index++;
		}
		eventcount = 0;
		for (int t = 0; t < 1; t++) {
			tvals &tv = trackvals[t];
			tstate &ts = trackstates[t];
			if (tv.note != zzub::note_value_none) {
				if (ts.note != zzub::note_value_none) {
					snd_seq_event_t &ev = events[eventcount];
					memset(&ev, 0, sizeof(snd_seq_event_t));
					ev.type = SND_SEQ_EVENT_NOTEOFF;
					ev.data.note.note = (ts.note>>4)*12 + (ts.note&0xf)-1;
					ts.note = zzub::note_value_none;
					eventcount++;
				}
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
	
	virtual const char * describe_value(const int param, const int value)
	{ 
		static char text[256];
		if (param < myinfo->m_metaparams.size()) {
			dssi_param mp = myinfo->m_metaparams[param];
			return describe_ladspa_value(mp, value, _master_info->samples_per_second, text);
		} 
		return 0; 
	}
	
		
	virtual bool process_stereo(float **pin, float **pout, int numsamples, int const mode)
	{
		if( mode!=zzub::process_mode_write )
			return false;
		assert(myinfo->m_audioins.size() == 0);
		if (desc->run_synth) {
			desc->run_synth(handle, numsamples, events, eventcount);
		} else {
			printf("dssidapter: no run_synth :(\n");
		}
		eventcount = 0;
		if (myinfo->m_audioouts.size() == 1) {
			memcpy(pout[0], outputs[0], sizeof(float) * numsamples);
			memcpy(pout[1], outputs[0], sizeof(float) * numsamples);
		} else if (myinfo->m_audioouts.size() >= 2) {
			memcpy(pout[0], outputs[0], sizeof(float) * numsamples);
			memcpy(pout[1], outputs[1], sizeof(float) * numsamples);
		} else {
			printf("dssidapter: no output :(\n");
			return false;
		}
		return true; 
	}
	
	// ::zzub::plugin methods
	virtual void process_controller_events() {}
	virtual void stop() {}
	virtual void save(zzub::archive*) {}
	virtual void attributes_changed() {}
	virtual void command(int) {}
	virtual void set_track_count(int) {}
	virtual void mute_track(int) {}
	virtual bool is_track_muted(int) const { return false; }
	virtual void midi_note(int, int, int) {}
	virtual void event(unsigned int) {}
	virtual const zzub::envelope_info** get_envelope_infos() { return 0; }
	virtual bool play_wave(int, int, float) { return false; }
	virtual void stop_wave() {}
	virtual int get_wave_envelope_play_position(int) { return -1; }
	virtual const char* describe_param(int) { return 0; }
	virtual bool set_instrument(const char*) { return false; }
	virtual void get_sub_menu(int, zzub::outstream*) {}
	virtual void add_input(const char*) {}
	virtual void delete_input(const char*) {}
	virtual void rename_input(const char*, const char*) {}
	virtual void input(float**, int, float) {}
	virtual void midi_control_change(int, int, int) {}
	virtual bool handle_input(int, int, int) { return false; }
};

zzub::plugin* dssi_info::create_plugin() const {
	return new dssidapter(this);
}

struct dssiplugincollection : zzub::plugincollection {
	// Called by the host initially. The collection registers
	// plugins through the pluginfactory::register_info method.
	// The factory pointer remains valid and can be stored
	// for later reference.
	virtual void initialize(zzub::pluginfactory *factory) {
		printf("initializing dssidapter...\n");
		DSSIPluginSearch(enumerate_dssiplugin);
		for (int i=0; i < infos.size(); ++i)
		{
			factory->register_info(infos[i]);
		}	
		printf("dssidapter: enumerated %i plugin(s).\n", infos.size());
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
	virtual void destroy() { delete this; }
};

zzub::plugincollection *zzub_get_plugincollection() {
	return new dssiplugincollection();
}

const char *zzub_get_signature() { return ZZUB_SIGNATURE; }

