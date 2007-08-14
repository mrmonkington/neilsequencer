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

#include "common.h"
#include "dummy.h"

namespace zzub {

/*! \struct dummy_info
	\brief Dummy machine base description.

	A dummy plugin will get a unique dummy_info which is instantiated and modified as the dummy
	is created.
*/

/*! \struct dummy_plugin
	\brief Dummy plugin implementation.
*/

/*!	\struct dummy_loader
	\brief Dummy plugin loader.
*/

const size_t default_dummy_maxtracks=255;

struct dummy_info : zzub::info {
	dummy_info() {
		this->type = zzub::plugin_type_master;
		this->flags = zzub::plugin_flag_mono_to_stereo;
		this->max_tracks = default_dummy_maxtracks;
		this->name = "Dummy";
		this->short_name = "Dummy";
		this->author = "n/a";
		this->uri = "urn:zzub:Dummy";
	}
	
	virtual zzub::plugin* create_plugin() const { return 0; }
	virtual bool store_info(zzub::archive *arc) const { return false; }
} dummyInfo;

/***

    dummy_plugin

***/

dummy_plugin::dummy_plugin(const zzub::info* info) {
    attributes = new int[info->attributes.size()];

    if (info->global_parameters.size()>0)
	    global_values=new char[info->global_parameters.size()*2]; else
        global_values=0;

    if (info->track_parameters.size()>0)
	    track_values=new char[info->track_parameters.size()*2*default_dummy_maxtracks]; else
        track_values=0;
}

void dummy_plugin::process_events() {
}

bool dummy_plugin::process_stereo(float **pin, float **pout, int numsamples, int mode) {
	if (mode==zzub::process_mode_write || mode==zzub::process_mode_no_io)
		return false;
	return true;
}


dummy_loader::dummy_loader(int type, std::string pluginUri, int attributes, int globalValues, int trackValues, parameter* params)
    :pluginloader(0, 0) 
{
    dummy_info* newInfo=new dummy_info();

    this->uri=pluginUri;

    newInfo->type=type;
    newInfo->uri=this->uri.c_str();
    //~ newInfo->attribute_count=attributes;
    //~ newInfo->global_parameter_count=globalValues;
    //~ newInfo->track_parameter_count=trackValues;
    
    for (size_t i = 0; i < attributes; ++i) {
        attribute& a = newInfo->add_attribute();
    }

    // copy incoming params because they wont be valid after loading ends
	for (size_t i=0; i < globalValues; ++i) {
		parameter &p = newInfo->add_global_parameter();
		p = *params;
        char* name = new char[strlen(params->name)+1];
        strcpy(name, params->name);
        p.name = name;
		params++;
	}
	for (size_t i=0; i < trackValues; ++i) {
		parameter &p = newInfo->add_track_parameter();
		p = *params;
        char* name = new char[strlen(params->name)+1];
        strcpy(name, params->name);
        p.name = name;
		params++;
	}

    plugin_info=newInfo;

}

plugin* dummy_loader::createMachine() {
    return new dummy_plugin(this->plugin_info);
}


};
