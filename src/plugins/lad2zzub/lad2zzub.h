/* lad2zzub.h

   Copyright (C) 2007 Frank Potulski (polac@gmx.de)

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.
  
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
   USA. */

#ifndef ___LAD2ZZUB_H
#define ___LAD2ZZUB_H

#include <windows.h>

#include <memory.h>
#include <stdio.h>

#include <string>
#include <list>
#include <vector>

#include <zzub\plugin.h>
#include <zzub\signature.h>

#include "ladspa\ladspaplugin.h"

using namespace zzub;
using namespace std;

#pragma intrinsic(memset)
#pragma intrinsic(memcpy)

#define _STRING_H_ 1
//#define ___WIN32___ 1
//#define ___LINUX___ 1

//*****************************************************************************************************************************************************

class CLadspaCollection : public plugincollection
{
public:

	struct SPluginInfo;		
	class CLadspaInfo : public info
	{
	public:

		typedef list<CLadspaInfo *>::iterator CIterator;
	
		CLadspaInfo();
		CLadspaInfo(const char *path,const SPluginInfo &pinfo);	
		virtual ~CLadspaInfo();

		bool operator <(const CLadspaInfo &other) const;
		bool operator ==(const CLadspaInfo &other) const;

		plugin *create_plugin(void) const;	
		bool store_info(archive *arc) const;

	private:

		static void ReplaceChar(char *str,const char toReplace,const char replacedBy);		

	public:
						
		string lad_name;
		string lad_label;
		string lad_path;
		string lad_author;
		string lad_uri;
		string lad_commands;

		int lad_uid;
		int lad_index;
		int lad_num_desc;
		
		vector<char *> lad_param_name;
	};

	struct SPluginInfo
	{
		SPluginInfo() : index(0), numDesc(0), uniqueID(0), numParams(0)
		{		
		};

		int index;
		int numDesc;

		unsigned int uniqueID;

		int numParams;

		string name;
		string label;
		string author;		
	};

	struct SPluginScan
	{	
		SPluginScan()
		{
		};

		~SPluginScan()
		{
			plugins.clear();
		};

		bool operator ==(const SPluginScan &other) const
		{
			return ::stricmp(path.c_str(),other.path.c_str())==0;
		};

		string path;		

		int numPlugins;

		vector<SPluginInfo> plugins;
	};

	class CLadspaPlugins
	{
	public:		

		class CIterator
		{
		public:
			
			CIterator();
			CIterator(CLadspaInfo::CIterator _it,CLadspaInfo::CIterator _end);
			~CIterator();

			const CLadspaInfo &operator *(void) const;
			const CLadspaInfo *operator ->(void) const;

			bool Next(void);
			bool End(void) const;

		private:

			CLadspaInfo::CIterator it;
			CLadspaInfo::CIterator end;
		};

		CLadspaPlugins();
		~CLadspaPlugins();

		int GetNumPlugins(void) const;	
		bool IsEmpty(void) const;

		CIterator GetPlugins(void);

	private:

		static DWORD ReadDatDword(HANDLE handle);
		static bool WriteDatDword(HANDLE handle,const DWORD d);
		
		static bool ReadDatString(HANDLE handle,string &str);
		static bool WriteDatString(HANDLE handle,const string &str);

		static SPluginScan *IsScanned(const char *path,list<SPluginScan *> &scan);
		
		void ScanPlugins(HANDLE handle,char *path,list<SPluginScan *> &scan);		

	private:

		list<CLadspaInfo *> plugins;		
	};

	CLadspaCollection();
	virtual ~CLadspaCollection();

	void initialize(pluginfactory *factory);
		
	const info *get_info(const char *uri,archive *arc);
		
	const char *get_uri(void);
		
	void configure(const char *key,const char *value);

	void destroy(void);

private:

	pluginfactory *factory;	

	CLadspaPlugins *plugins;
};

//*****************************************************************************************************************************************************

class CLadspa : public plugin
{
	friend BOOL APIENTRY DllMain(HINSTANCE hm,DWORD dw,LPVOID lp);
	friend const char *zzub_get_signature(void);
	friend class CLadspaCollection::CLadspaInfo;
	friend class CLadspaCollection;

#pragma pack(1)
		
	class SGlobalParams
	{
	public:

		SGlobalParams() : params(0)
		{
			
		};

		~SGlobalParams()
		{
			if (params)
			{
				delete[] params;
			}
		};

		void Initialize(const int n)
		{
			if (params)
			{
				delete[] params;
			}

			params = new unsigned short[n];
		};

		__forceinline unsigned short &operator [](const int index)
		{
			return params[index];
		};		

		unsigned short *params;
	};	

#pragma pack()

	struct SParam
	{				
		unsigned short newParam;

		float inertia;

		float curParam;
	};

public:

	enum 
	{
		eVersion = 1,
		eMinTracks = 0,
		eMaxTracks = 0,
		eNumGlobalParams = 0,
		eNumTrackParams = 0,
		eNumParams = eNumGlobalParams + eNumTrackParams,
		eNumAttributes = 0,
		eMinParam = 0x0000,
		eMaxParam = 0x8000,		
		eMaxInertia = 32000,
		eDefaultInertia = 1000,
		eInertiaTick = 1000
	};
	
//**** plugin *****************************************************************************************************************************************

	void init(archive *arc);
	void save(archive *arc);

	void destroy(void);	

	void process_events(void);

	bool process_stereo(float **pin,float **pout,int numsamples,int mode);
	
	void stop(void);
	
	void attributes_changed(void);
	void command(int index);
	
	void set_track_count(int count);
	
	void mute_track(int index);	
	bool is_track_muted(int index) const;
	
	void midi_note(int channel,int value,int velocity);
	void midi_control_change(int ctrl,int channel,int value);

	void event(unsigned int data);
	
	const char * describe_value(int param,int value);
	const envelope_info **get_envelope_infos(void);
	bool play_wave(int wave,int note,float volume);
	
	void stop_wave(void);
	
	int get_wave_envelope_play_position(int env);

	const char *describe_param(int param);

	bool set_instrument(const char *name);
	void get_sub_menu(int index,outstream *os);
	
	void add_input(const char *name);
	void delete_input(const char *name);
	void rename_input(const char *oldname,const char *newname);
	void input(float **samples,int size,float amp);
	bool handle_input(int index,int amp,int pan);

//**** CLadspa ****************************************************************************************************************************************

private:
	
	CLadspa(const CLadspaCollection::CLadspaInfo *_info);
	virtual ~CLadspa();	

	__forceinline bool IsDenormal(const float &val) const
	{		
		return ( *((unsigned int *)&val) & 0x7f800000 ) == 0;
	};

private:	

	static HINSTANCE hInst;	

	CLadspaCollection::CLadspaInfo *info;

	CLadspaPlugin *ladspa;

	bool ladspaInitialized;

	SGlobalParams params;	

	metaplugin *cm;

	float **ins;
	float **outs;

	char paramDescribe[64];

	unsigned short inertia;	

	SParam *paramInertia;
};

//*****************************************************************************************************************************************************

#endif