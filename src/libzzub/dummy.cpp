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

const size_t default_dummy_maxtracks=255;

dummy_info::dummy_info() {
	this->flags = zzub::plugin_flag_mono_to_stereo;
	this->max_tracks = default_dummy_maxtracks;
	this->name = "Dummy";
	this->short_name = "Dummy";
	this->author = "n/a";
	this->uri = "urn:zzub:Dummy";
}
	
zzub::plugin* dummy_info::create_plugin() const { 
	return new dummy_plugin(this); 
}

bool dummy_info::store_info(zzub::archive *arc) const { 
	return false; 
}


/***

    dummy_plugin

***/

dummy_plugin::dummy_plugin(const zzub::info* info) {
	attributes = new int[info->attributes.size()];

	if (info->global_parameters.size()>0)
		global_values = new char[info->global_parameters.size()*2]; else
		global_values = 0;

	if (info->track_parameters.size()>0)
		track_values = new char[info->track_parameters.size()*2*default_dummy_maxtracks]; else
		track_values =0;
}

void dummy_plugin::destroy() {
	delete[] attributes;

	if (global_values != 0)
		delete[] global_values;

	if (track_values != 0)
		delete[] track_values;

	delete this;
}

void dummy_plugin::init(zzub::archive* arc) {
	// we assume player::create_plugin gives us an archive which contains the
	// data bytes only (and not the remainder of e.g the .bmx)
	if (!arc) return ;
	instream* inf = arc->get_instream("");
	data.resize(inf->size());
	inf->read(&data.front(), data.size());
}

void dummy_plugin::load(zzub::archive* arc) {
	if (!arc) return ;
	instream* inf = arc->get_instream("");
	data.resize(inf->size());
	inf->read(&data.front(), (int)data.size());
}

void dummy_plugin::save(zzub::archive* arc) {
	if (!arc) return ;
	outstream* outf = arc->get_outstream("");
	outf->write(&data.front(), (int)data.size());
}

void dummy_plugin::process_events() {
}

bool dummy_plugin::process_stereo(float **pin, float **pout, int numsamples, int mode) {
	if (mode==zzub::process_mode_write || mode==zzub::process_mode_no_io)
		return false;
	return true;
}

};
