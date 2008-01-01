// ladspa plugin adapter
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


// ladspadapter allows running LADSPA plugins as zzub plugins

#include <ladspa.h>
#include <zzub/signature.h>
#include 	"zzub/plugin.h"
#include <vector>
#include <string>
#include <assert.h>

extern "C"
{
#include "utils.h"
}

#include "paramtools.h"


using namespace zzub;

struct ladspa_info : zzub::info
{
	std::string m_filename;
	std::string m_label;
	std::string m_name;
	std::string m_maker;
	std::string m_uri;
	std::vector<std::string> m_paramnames;
	std::vector<std::string> m_paramhints;
	std::vector<zzub::parameter*> m_params;
	std::vector<ladspa_param> m_metaparams;
	int m_index;
	std::vector<ladspa_param> m_audioins;
	std::vector<ladspa_param> m_audioouts;
	
	virtual zzub::plugin* create_plugin() const;
	virtual bool store_info(zzub::archive *data) const { return false; }
};

std::vector<ladspa_info *> infos;

void enumerate_ladspaplugin
(const char * pcFullFilename, 
 void * pvPluginHandle,
 LADSPA_Descriptor_Function fDescriptorFunction)
{
	printf("enumerating '%s'...\n", pcFullFilename);
	int index = 0;
	while (true)
	{
		const LADSPA_Descriptor *desc = fDescriptorFunction(index);
		if (!desc)
			break;
		ladspa_info *i = new ladspa_info();
		infos.push_back(i);
		i->m_filename = pcFullFilename;
		i->m_index = index;
		i->m_label = desc->Label;
		i->m_name = desc->Name;		
		i->m_maker = desc->Maker;
		i->flags = zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
		i->name = i->m_name.c_str();
		i->short_name = i->m_label.c_str();
		i->author = i->m_maker.c_str();
		
		i->m_uri = "@zzub.org/ladspadapter/" + i->m_label;
		replace(i->m_uri.begin(), i->m_uri.end(), ' ', '+');
		for (int port = 0; port < desc->PortCount; port++)
		{
			std::string name = desc->PortNames[port];
			LADSPA_PortDescriptor pd = desc->PortDescriptors[port];
			LADSPA_PortRangeHint hint = desc->PortRangeHints[port];
			ladspa_param mp;
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
		i->uri = i->m_uri.c_str();
		index++;
	}
}

struct ladspadapter : plugin
{
	char *globals;
	const ladspa_info *myinfo;
	const LADSPA_Descriptor *desc;
	void *library;
	LADSPA_Handle handle;
	LADSPA_Data *data_values;
	float inputs[16][256];
	float outputs[16][256];
	int silencecount;
	
	~ladspadapter()
	{
		if (handle)
		{
			if (desc->deactivate)			
				desc->deactivate(handle);
			desc->cleanup(handle);
		}
		unloadLADSPAPluginLibrary(library);
		if (globals)
		{
			delete[] globals;
		}
		if (data_values)
		{
			delete[] data_values;
		}
	}
	
	ladspadapter(const ladspa_info *_ladspa_info)
	{
		globals = 0;
		global_values = 0;
		data_values = 0;
		myinfo = _ladspa_info;		
		if (myinfo->m_params.size())
		{
			int size = 0;
			std::vector<ladspa_param>::const_iterator i;
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
			data_values = new LADSPA_Data[myinfo->m_params.size()];
		}
		track_values = 0;
		attributes = 0;
		library = loadLADSPAPluginLibrary(myinfo->m_filename.c_str());
		desc = findLADSPAPluginDescriptor(library, myinfo->m_filename.c_str(), myinfo->m_label.c_str());
		handle = 0;
		silencecount = 0;
	}
	
	virtual void destroy()
	{
		delete this;
	}
	
	virtual void init(archive *pi)
	{
		handle = desc->instantiate(desc, _master_info->samples_per_second);
		std::vector<ladspa_param>::const_iterator i;
		int index = 0;
		for (i = myinfo->m_metaparams.begin(); i != myinfo->m_metaparams.end(); ++i)
		{
			if (getLADSPADefault(&i->hint, _master_info->samples_per_second, &data_values[index]) == -1)
			{
				data_values[index] = 0.0f;
			}			
			desc->connect_port(handle, i->index, &data_values[index]);
			index++;
		}
		index = 0;
		for (i = myinfo->m_audioins.begin(); i != myinfo->m_audioins.end(); ++i)
		{
			desc->connect_port(handle, i->index, inputs[index]);
			memset(inputs[index],0,sizeof(float)*256);
			index++;
		}
		index = 0;
		for (i = myinfo->m_audioouts.begin(); i != myinfo->m_audioouts.end(); ++i)
		{
			desc->connect_port(handle, i->index, outputs[index]);
			memset(inputs[index],0,sizeof(float)*256);
			index++;
		}
		
		if (desc->activate)
			desc->activate(handle);
	}
	
	virtual void process_events()
	{
		std::vector<ladspa_param>::const_iterator i;
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
	}
	
	virtual void process_controller_events() {}
	
	virtual const char * describe_value(const int param, const int value)
	{ 
		static char text[256];
		if (param < myinfo->m_metaparams.size()) {
			ladspa_param mp = myinfo->m_metaparams[param];
			return describe_ladspa_value(mp, value, _master_info->samples_per_second, text);
		} 
		return 0; 
	}
	
	virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) { return false; }
	
	virtual bool process_stereo(float **pin, float **pout, int numsamples, int const mode)
	{
		if (mode == zzub::process_mode_no_io)
			return false;
		if (mode & zzub::process_mode_read) {
			silencecount = 0;
			if (myinfo->m_audioins.size() == 1) {
				float *plin = inputs[0];
				float *pIL = pin[0];
				float *pIR = pin[1];
				for (int i = 0; i < numsamples; ++i) {
					*plin++ = (*pIL++ * 0.5) + (*pIR++ * 0.5);
				}
			} else if (myinfo->m_audioins.size() >= 2) {
				memcpy(inputs[0], pin[0], sizeof(float) * numsamples);
				memcpy(inputs[1], pin[1], sizeof(float) * numsamples);
			}
		} else {
			if (silencecount > _master_info->samples_per_second) {
				return false;
			}
			for (int i = 0; i < myinfo->m_audioins.size(); ++i) {
				memset(inputs[i], 0, sizeof(float)*numsamples);
			}
		}
		desc->run(handle, numsamples);
		if (mode & zzub::process_mode_write) {
			memcpy(pout[0], outputs[0], sizeof(float) * numsamples);
			if (myinfo->m_audioouts.size() == 1) {
				memcpy(pout[1], outputs[0], sizeof(float) * numsamples);
			} else if (myinfo->m_audioouts.size() >= 2) {
				memcpy(pout[1], outputs[1], sizeof(float) * numsamples);
			} else {
				return false;
			}
			if (buffer_has_signals(pout[0], numsamples) || buffer_has_signals(pout[1], numsamples)) {
				silencecount = 0;
				return true;
			}
			silencecount += numsamples;
			return false;
		}
		return true; 
	}
	
	// ::zzub::plugin methods
	virtual void stop() {}
	virtual void load(zzub::archive *arc) {}
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

zzub::plugin* ladspa_info::create_plugin() const {
	return new ladspadapter(this);
}

struct ladspaplugincollection : zzub::plugincollection {
	// Called by the host initially. The collection registers
	// plugins through the pluginfactory::register_info method.
	// The factory pointer remains valid and can be stored
	// for later reference.
	virtual void initialize(zzub::pluginfactory *factory) {
		printf("initializing ladspadapter...\n");
		LADSPAPluginSearch(enumerate_ladspaplugin);
		for (int i=0; i < infos.size(); ++i)
		{
			factory->register_info(infos[i]);
		}	
		printf("ladspadapter: enumerated %i plugin(s).\n", infos.size());
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
	return new ladspaplugincollection();
}

const char *zzub_get_signature() { return ZZUB_SIGNATURE; }
