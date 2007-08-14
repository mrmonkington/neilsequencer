/* psy2zzub.h

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

#ifndef ___PSY2ZZUB_H
#define ___PSY2ZZUB_H

#include <windows.h>

#include <memory.h>
#include <stdlib.h>
#include <stdio.h>

#include <string>
#include <list>
#include <vector>

#include "other\autoptr.h"

#include <zzub\plugin.h>
#include <zzub\signature.h>

#include "psycle\plugin_interface.hpp"

using namespace zzub;
using namespace psycle;
using namespace std;

#pragma intrinsic(memset)
#pragma intrinsic(memcpy)

#define _STRING_H_ 1

//*****************************************************************************************************************************************************

class CPsycleCollection : public plugincollection
{
public:

	class CPsycleInfo : public info
	{
		friend class CPsycle;
		friend class CPsycleCollection;		

	public:

		struct SParamInfo
		{
			SParamInfo()
			{
				name[0] = 0;
				desc[0] = 0;
			};

			char name[32];
			char desc[32];
		};
			
		CPsycleInfo();
		virtual ~CPsycleInfo();

		bool operator <(const CPsycleInfo &other) const;
		bool operator ==(const CPsycleInfo &other) const;

		plugin *create_plugin(void) const;	
		bool store_info(archive *arc) const;

	private:		

		string psy_name;
		string psy_label;
		string psy_path;
		string psy_author;
		string psy_uri;
		string psy_command;

		vector<SParamInfo> psy_param;
	};	

	CPsycleCollection();
	virtual ~CPsycleCollection();

	void ScanPlugins(char *path);
	void AddPlugin(const char * const path);

	void initialize(pluginfactory *factory);
		
	const info *get_info(const char *uri,archive *arc);
		
	const char *get_uri(void);
		
	void configure(const char *key,const char *value);

	void destroy(void);

private:

	static void ReplaceChar(char *str,const char toReplace,const char replacedBy);	

private:

	pluginfactory *factory;

	list<CPsycleInfo *> plugins;
};

//*****************************************************************************************************************************************************

class CPsycle : public plugin, public CFxCallback
{
	friend BOOL APIENTRY DllMain(HINSTANCE hm,DWORD dw,LPVOID lp);
	friend const char *zzub_get_signature(void);
	friend class CPsycleCollection::CPsycleInfo;
	friend class CPsycleCollection;

#pragma pack(1)

	struct STrackParam
	{		
		unsigned char note;
		unsigned short command;
	};

	/*struct SGlobalParam
	{		
		__forceinline operator unsigned short(void)
		{
			return param;
		};
		
		unsigned short param;		
	};*/

	struct SGlobalParams
	{
		enum 
		{
			eSizeof_Byte=0,
			eSizeof_Short
		};

		SGlobalParams(const CPsycleCollection::CPsycleInfo *info) : params(0), index(0), size(0)
		{
			if (info)
			{
				const int n = int(info->global_parameters.size());

				if (n>0)
				{
					int size = 0;

					index = (int *)::malloc( sizeof(int) * n );

					this->size = (char *)::malloc( sizeof(char) * n );

					if ( index && this->size )
					{
						for (int i=0;i<n;++i)
						{
							const parameter &param = *info->global_parameters[i];

							switch (param.type)
							{
							
							case zzub_parameter_type_note:
							case zzub_parameter_type_switch:
							case zzub_parameter_type_byte:
								{
									index[i] = size;

									this->size[i] = eSizeof_Byte;
									
									++size;
								}
								break;

							case zzub_parameter_type_word:
								{
									index[i] = size;

									this->size[i] = eSizeof_Short;

									size += 2;
								}
								break;

							default:
								{									
									::free(index);

									index = 0;

									::free(this->size);

									this->size = 0;

									return;
								}
								break;

							}
						}

						if (size>0)
						{
							params = (unsigned char *)::malloc(size);
						}					
					}									
				}			
			}
		};

		~SGlobalParams()
		{
			if (params)
			{
				::free(params);

				params = 0;
			}

			if (index)
			{
				::free(index);

				index = 0;
			}

			if (size)
			{
				::free(size);

				size = 0;
			}
		};
			
		__forceinline int operator [](const int n) const
		{			
			//return int( params[ index[n] ] );

			return (size[n]==eSizeof_Byte) ? int( params[ index[n] ] ) : int( *( (unsigned short *)&params[ index[n] ] ) );
		};		

		unsigned char *params;	

		int *index;
		
		char *size;
	};	

#pragma pack()

public:

	enum 
	{
		eVersion = 1,
		eMinTracks = 8,
		eMaxTracks = 32,
		eMinValue = 0,
		eMaxValue = 0xFFFE		
	};

//**** CFxCallback ************************************************************************************************************************************

	void MessBox(char *ptxt,char *caption,unsigned int type);
	
	int CallbackFunc(int cbkID,int par1,int par2,int par3);
	
	float *GetWaveLData(int inst,int wave);
	float *GetWaveRData(int inst,int wave);
	
	int GetTickLength(void);
	int GetSamplingRate(void);
	int GetBPM(void);
	int GetTPB(void);
	
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

//**** CPsycle ****************************************************************************************************************************************

private:
	
	CPsycle(const CPsycleCollection::CPsycleInfo *_info);
	virtual ~CPsycle();

	bool Open(void);
	bool Close(void);

	__forceinline bool IsDenormal(const float &val) const
	{		
		return ( *((unsigned int *)&val) & 0x7f800000 ) == 0;
	};

private:	

	static HINSTANCE hInst;	

	CPsycleCollection::CPsycleInfo *info;

	SGlobalParams global;

	//TAutoArrayPtr<SGlobalParam> global;
	TAutoArrayPtr<STrackParam> track;	

	metaplugin *cm;

	HMODULE hm;

	CMachineInterface *mi;
	const CMachineParameter **pinfo;

	char describe[128];

	int numTracks;
};

//*****************************************************************************************************************************************************

#endif