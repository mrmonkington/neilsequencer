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

#pragma once

namespace zzub {
	
struct input_plugin_info : zzub::info {

	input_plugin_info() {
		this->type = zzub::plugin_type_generator;
		this->flags = 0;
		this->name = "Audio Input";
		this->short_name = "Input";
		this->author = "n/a";
		this->uri = "@zzub.org/input";
		this->commands = "/Input Channel";
		add_attribute()
			.set_name("Record Channel")
			.set_value_min(0)
			.set_value_max(32)
			.set_value_default(0);
		//~ add_global_parameter()
			//~ .set_byte()
			//~ .set_name("Channel")
			//~ .set_description("Recording channel")
			//~ .set_value_min(1)
			//~ .set_value_max(32)
			//~ .set_value_none(255)
			//~ .set_state_flag()
			//~ .set_value_default(1);

	}
	
	virtual zzub::plugin* create_plugin() const;
	virtual bool store_info(zzub::archive *) const { return false; }
};

// A plugin collection registers plugin infos and provides
// serialization services for plugin info, to allow
// loading of plugins from song data.
struct input_plugincollection : plugincollection {
	input_plugin_info _info;
	
	// Called by the host initially. The collection registers
	// plugins through the pluginfactory::register_info method.
	// The factory pointer remains valid and can be stored
	// for later reference.
	virtual void initialize(zzub::pluginfactory *factory);
	
	// Called by the host upon song loading. If the collection
	// can not provide a plugin info based on the uri or
	// the metainfo passed, it should return a null pointer.
	// This will usually only be called if the host does
	// not know about the uri already.
	virtual const zzub::info *get_info(const char *uri, zzub::archive *arc) { return 0; }
	
	// Returns the uri of the collection to be identified,
	// return zero for no uri. Collections without uri can not be 
	// configured.
	virtual const char *get_uri() { return 0; }
	
	// Called by the host to set specific configuration options,
	// usually related to paths.
	virtual void configure(const char *key, const char *value) {}
	
	// Called by the host upon destruction. You should
	// delete the instance in this function
	virtual void destroy() {}
};

}
