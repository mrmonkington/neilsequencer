// This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
// copyright 2007 Frank Potulski <polac@gmx.de>
// copyright 2007 members of the psycle project http://psycle.sourceforge.net

#ifndef ZZUB__PLUGINS__PSYCLE_TO_ZZUB__INCLUDED
#define ZZUB__PLUGINS__PSYCLE_TO_ZZUB__INCLUDED
#pragma once

#include <cstdlib>
#include <cstdio>
#include <string>
#include <list>
#include <vector>

#if defined _WIN64 || defined _WIN32
	#include <windows.h> // for HMODULE
#endif

#include <zzub/plugin.h>
#include <zzub/signature.h>

#include "psycle/plugin_interface.hpp" ///\todo should be <psycle/plugin_interface.hpp>

namespace zzub { namespace plugins { namespace psycle_to_zzub {

	/// module handling
	namespace module {
		typedef 
			#if defined _WIN64 || defined _WIN32
				HMODULE
			#else
				void *
			#endif
			handle_type;
	}

	class plugin_info;
	
	class plugin : public zzub::plugin, private psycle::plugin_interface::CFxCallback {
		public:
			///\name implementation for psycle::plugin_interface::CFxCallback
			///\{
				void MessBox(char const * message, char const * caption, unsigned int type) const;
				int CallbackFunc(int /*cbkID*/, int /*par1*/, int /*par2*/, void* /*par3*/) { return 0; }
				float * unused0(int, int) { return 0; }
				float * unused1(int, int) { return 0; }
				int GetTickLength() const { return _master_info->samples_per_tick; }
				int GetSamplingRate() const { return _master_info->samples_per_second; }
				int GetBPM() const { return _master_info->beats_per_minute; }
				int GetTPB() const { return _master_info->ticks_per_beat; }
			///\}
	
			///\name implementation for zzub::plugin
			///\{
				void init(zzub::archive *arc);
				void process_events();
				bool process_stereo(float **pin, float **pout, int numsamples, int mode);
				void stop() { if(psycle_plugin) psycle_plugin->Stop(); }
				void save(zzub::archive *arc);
				void command(int index);
				void set_track_count(int count) { track_count = count; }
				const char * describe_value(int param, int value);
			///\}
			
		private:
			plugin(const plugin_info *);
			virtual ~plugin() throw();

			bool open();
			bool close();

			const plugin_info * info;

			class global_params_type {
				public:
					global_params_type(const plugin_info *);
					~global_params_type();
					
					int operator[](const int n) const {
						return (size[n] == 1) ?
							data[index[n]] :
							*reinterpret_cast<unsigned short int *>(&data[index[n]]);
					}
				
				public:
					unsigned char * data;
					int * index;
					char * size;
					friend class plugin;
			} global_params;

			#pragma pack(push, 1)
			struct track_param {
				unsigned char note;
				unsigned short command;
			} * track_params;
			#pragma pack(pop)

			//zzub::metaplugin * metaplugin;
			module::handle_type module_handle;
			psycle::plugin_interface::CMachineInterface * psycle_plugin;
			psycle::plugin_interface::CMachineParameter const * const * psycle_plugin_param_info;
			int track_count;
			char one_param_description[128];
			
			friend class plugin_collection;
			friend class plugin_info;
	};
	
	class plugin_info : public zzub::info {
		public:
			///\name implementation for zzub::info
			///\{
				zzub::plugin * create_plugin() const { return new plugin(this); }
				bool store_info(zzub::archive*) const { return false; }
			///\}

		private:
			std::string psy_name, psy_label, psy_path, psy_author, psy_uri, psy_command;
			struct param_info {
				param_info() { name[0] = 0; desc[0] = 0; }
				char name[32];
				char desc[32];
			};
			std::vector<param_info> psy_param;
			friend class plugin_collection;
			friend class plugin;
	};
			
	class plugin_collection : public zzub::plugincollection
	{
		public:
			plugin_collection() : factory() {}
			~plugin_collection();
			
			///\name implementation for zzub::plugincollection
			///\{
				void initialize(zzub::pluginfactory * factory);
				const zzub::info * get_info(const char *uri, zzub::archive *arc) { return 0; }
				const char * get_uri() { return 0; }
				void configure(const char * key, const char * value) {}
				void destroy();
			///\}

		private:
			zzub::pluginfactory *factory;
			void scan_plugins(std::string const & path);
			void add_plugin(std::string const & path);
			std::list<plugin_info*> plugin_infos;
	};
}}}
#endif
