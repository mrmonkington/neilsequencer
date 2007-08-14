/* psy2zzub.cpp

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

#include "psy2zzub.h"

//*****************************************************************************************************************************************************

//__declspec(dllexport) ::CMachineInterface * cdecl CreateMachine() { return new typename; } \
//__declspec(dllexport) void cdecl DeleteMachine(::CMachineInterface & plugin) { delete &plugin; } \

typedef CMachineInfo const * const (* GetInfo)(void);
typedef CMachineInterface *(* CreateMachine)(void);
typedef void (* DeleteMachine)(CMachineInterface &plugin);

BOOL APIENTRY DllMain(HINSTANCE hm,DWORD dw,LPVOID lp)
{
	switch (dw)
	{

	case DLL_PROCESS_ATTACH:
		{
			CPsycle::hInst = hm;			
		}
		break;

	case DLL_PROCESS_DETACH:
		{

		}
		break;
	
	}
	
	return TRUE;
}

float Normalize(const int val,const int min,const int max)
{		
	return float( val- min ) / float( max - min );
};

int Scale(float normal,const int min,const int max)
{
	return int( normal * float( max - min) + 0.5f ) + min;
};

//**** CPsycleCollection ******************************************************************************************************************************
		
CPsycleCollection::CPsycleInfo::CPsycleInfo() : info()
{	
}

CPsycleCollection::CPsycleInfo::~CPsycleInfo()
{
}

plugin *CPsycleCollection::CPsycleInfo::create_plugin(void) const
{		
	return new CPsycle(this);
}
	
bool CPsycleCollection::CPsycleInfo::store_info(archive *arc) const
{
	return false;
}


CPsycleCollection::CPsycleCollection() : factory(0)
{
}

CPsycleCollection::~CPsycleCollection()
{	
}

void CPsycleCollection::initialize(pluginfactory *factory)
{	
	this->factory = factory;

	if (this->factory)
	{
		HMODULE hm = ::GetModuleHandle(NULL);

		if (hm)
		{
			char path[ MAX_PATH + 32 ] = {0};

			::GetModuleFileName(hm,path,MAX_PATH);		

			size_t n = ::strlen(path);

			if (n)
			{
				while (n--)
				{
					if (path[n]=='\\')
					{
						path[n] = 0;

						break;
					}
				}

				::strcat(path,"\\Gear\\Psycle\\");
				
				ScanPlugins(path);
			}
		}
	}
}
		
const info *CPsycleCollection::get_info(const char *uri,archive *arc)
{	
	if ( uri && arc )
	{
		
	}
	
	return 0;
}

const char *CPsycleCollection::get_uri(void)
{	
	return 0;
}
		
void CPsycleCollection::configure(const char *key,const char *value)
{
	int i=0;	
}
		
void CPsycleCollection::destroy(void)
{		
	if (plugins.size())
	{
		list<CPsycleInfo *>::iterator it = plugins.begin();
		const list<CPsycleInfo *>::iterator end = plugins.end();

		while (it!=end)
		{
			delete (*it);

			++it;
		}

		plugins.clear();
	}

	factory = 0;

	delete this;
}


void CPsycleCollection::ScanPlugins(char *path)
{
	if ( !path || !path[0] ) 
	{
		return;
	}
			
	const int len = int(::strlen(path));

	if ( path[len-1] == '\\' )
	{
		path[len-1] = 0;
	}

	char file[MAX_PATH] = {0};
	char tmp[MAX_PATH] = {0};
			
	::sprintf(file,"%s\\*",path);
	
	WIN32_FIND_DATA x = {0};

	HANDLE fHandle = INVALID_HANDLE_VALUE;

	for (;;)
	{
		if (fHandle==INVALID_HANDLE_VALUE) fHandle = ::FindFirstFile(file,&x);
		if (fHandle==INVALID_HANDLE_VALUE) break;

		::sprintf(tmp,"%s\\%s",path,x.cFileName);
		
		if (::GetFileAttributes(tmp)&FILE_ATTRIBUTE_DIRECTORY)
		{
			if ( ::strcmp(x.cFileName,".") && ::strcmp(x.cFileName,"..") )
			{								
				ScanPlugins(tmp);
			}
		}

		BOOL b = ::FindNextFile(fHandle,&x);
		
		if (!b) 
		{
			::FindClose(fHandle);

			break;
		}
	}
		
	::sprintf(file,"%s\\*.dll",path);

	fHandle = INVALID_HANDLE_VALUE;

	for (;;)
	{
		if (fHandle==INVALID_HANDLE_VALUE) fHandle = ::FindFirstFile(file,&x);
		if (fHandle==INVALID_HANDLE_VALUE) break;

		::sprintf(tmp,"%s\\%s",path,x.cFileName);

		AddPlugin(tmp);
		
		BOOL b = ::FindNextFile(fHandle,&x);

		if (!b) 
		{
			::FindClose(fHandle);

			return;
		}	
	}	
}

void CPsycleCollection::ReplaceChar(char *str,const char toReplace,const char replacedBy)
{
	if ( str && toReplace!=replacedBy )
	{
		while (*str)
		{
			char &cmp = *str;

			if (cmp==toReplace)
			{
				cmp = replacedBy;
			}

			++str;
		}			
	}
}

void CPsycleCollection::AddPlugin(const char * const path)
{
	if ( !factory || !path || !path[0] ) 
	{
		return;
	}

	HMODULE hm = ::LoadLibrary(path);

	if (hm)
	{
		GetInfo getInfo = GetInfo(::GetProcAddress(hm,"GetInfo"));

		if (getInfo)
		{
			const CMachineInfo * const info = getInfo();	

			if (info)
			{
				CPsycleInfo *p = new CPsycleInfo();

				if (p)
				{
					p->type = (info->Flags==psycle::GENERATOR) ? zzub::plugin_type_generator : zzub::plugin_type_effect;

					p->version = zzub::version;
					p->flags = zzub::plugin_flag_mono_to_stereo;					

					p->psy_name = info->Name;
					p->name = p->psy_name.c_str();

					p->psy_label = info->ShortName;
					p->short_name = p->psy_label.c_str();

					p->psy_author = info->Author;
					p->author = p->psy_author.c_str();

					p->psy_command = info->Command;
					p->psy_command += "\nAbout ";
					p->psy_command += info->Name;
					p->psy_command += "...";
					p->commands = p->psy_command.c_str();

					p->psy_uri = "@xlutop.com/psy2zzub/";					
					p->psy_uri += p->psy_name;
					ReplaceChar((char *)p->psy_uri.c_str(),' ','+');
					ReplaceChar((char *)p->psy_uri.c_str(),'\t','+');
					ReplaceChar((char *)p->psy_uri.c_str(),'\n','+');
					ReplaceChar((char *)p->psy_uri.c_str(),'\r','+');
					p->uri = p->psy_uri.c_str();

					p->psy_path = path;

					if ( info->numParameters>0 && info->Parameters )
					{
						const int n = info->numParameters;

						p->psy_param.reserve(n);
						
						for (int i=0;i<n;++i)
						{
							zzub::parameter &dstParam = p->add_global_parameter();

							const psycle::CMachineParameter &srcParam = (*info->Parameters[i]);

							dstParam.set_word();

							if (srcParam.Flags==MPF_STATE)
							{
								dstParam.set_flags(zzub::parameter_flag_state);
							}
							
							if ( srcParam.MinValue>=CPsycle::eMinValue && srcParam.MaxValue<=CPsycle::eMaxValue )
							{
								dstParam.set_value_min(srcParam.MinValue);
								dstParam.set_value_max(srcParam.MaxValue);
								dstParam.set_value_default(srcParam.DefValue);
							}
							else if ( ( srcParam.MaxValue - srcParam.MinValue ) <= CPsycle::eMaxValue )
							{
								dstParam.set_value_min(0);
								dstParam.set_value_max( srcParam.MaxValue - srcParam.MinValue );
								dstParam.set_value_default( srcParam.DefValue - srcParam.MinValue );								
							}
							else
							{
								dstParam.set_value_min(CPsycle::eMinValue);
								dstParam.set_value_max(CPsycle::eMaxValue);
								dstParam.set_value_default( Scale( Normalize(srcParam.DefValue,srcParam.MinValue,srcParam.MaxValue) , dstParam.value_min , dstParam.value_max ) );
							}	

							if (dstParam.value_max<=0x01)
							{
								dstParam.type = parameter_type_switch;
								dstParam.value_none = 0xff;
							}							
							else if (dstParam.value_max<0xff)
							{
								dstParam.type = parameter_type_byte;
								dstParam.value_none = 0xff;
							}

							::strncpy(p->psy_param[i].name,srcParam.Name,32);
							p->psy_param[i].name[31] = 0;
							dstParam.set_name(p->psy_param[i].name);

							::strncpy(p->psy_param[i].desc,srcParam.Description,32);
							p->psy_param[i].desc[31] = 0;
							dstParam.set_description(p->psy_param[i].desc);
						}
					}
					
					if (info->Flags==psycle::GENERATOR)
					{
						p->min_tracks = CPsycle::eMinTracks;
						p->max_tracks = CPsycle::eMaxTracks;

						zzub::parameter &paramNote = p->add_track_parameter();

						paramNote.set_note();
						paramNote.set_flags(zzub::parameter_flag_event_on_edit);

						zzub::parameter &paramCommand = p->add_track_parameter();

						paramCommand.set_word();
						paramCommand.set_value_min(0x0001);
						paramCommand.set_value_max(0xFFFF);
						paramCommand.set_value_none(0x0000);
						paramCommand.set_name("Note Command");
						paramCommand.set_description("Note Command");
					}
					else
					{
						p->min_tracks = 0;
						p->max_tracks = 0;
					}

					plugins.push_back(p);

					factory->register_info(p);
				}				
			}
		}

		::FreeLibrary(hm);
	}
}

//**** CPsycle ****************************************************************************************************************************************

HINSTANCE CPsycle::hInst = 0;

//CPsycle::CPsycle(const CPsycleCollection::CPsycleInfo *_info) : info((CPsycleCollection::CPsycleInfo *)_info), global( int(_info->global_parameters.size()) ), track( int(_info->track_parameters.size()) * eMaxTracks ), mi(0), pinfo(0), cm(0), numTracks(0), params(_info)
CPsycle::CPsycle(const CPsycleCollection::CPsycleInfo *_info) : info((CPsycleCollection::CPsycleInfo *)_info), track( int(_info->track_parameters.size()) * eMaxTracks ), mi(0), pinfo(0), cm(0), numTracks(0), global(_info)
{	
	global_values = global.params;
	track_values = track;

	attributes = 0;
}

CPsycle::~CPsycle()
{
}

bool CPsycle::Open(void)
{
	if (!info) return false;

	Close();

	hm = ::LoadLibrary(info->psy_path.c_str());	

	if (!hm) return false;

	GetInfo getInfo = (GetInfo)::GetProcAddress(hm,"GetInfo");

	if (!getInfo)
	{
		::FreeLibrary(hm);

		hm = 0;

		return false;	
	}

	const CMachineInfo *minfo = getInfo();

	if (!minfo)
	{
		::FreeLibrary(hm);

		hm = 0;

		return false;		
	}

	pinfo = minfo->Parameters;

	CreateMachine createMachine = (CreateMachine)::GetProcAddress(hm,"CreateMachine");

	if (!createMachine)
	{
		::FreeLibrary(hm);

		hm = 0;

		return false;
	}	

	mi = createMachine();

	if (!mi)
	{
		::FreeLibrary(hm);

		hm = 0;

		return false;
	}

	mi->pCB = this;

	mi->Init();

	return true;
}

bool CPsycle::Close(void)
{
	if (mi)
	{
		try
		{
			DeleteMachine deleteMachine = (DeleteMachine)::GetProcAddress(hm,"DeleteMachine");

			if (deleteMachine)
			{		
				deleteMachine(*mi);
			}
			else
			{
				delete mi;
			}
		}
		catch(...)
		{
		}

		mi = 0;
	}
	
	if (hm)
	{
		try
		{
			::FreeLibrary(hm);
		}
		catch(...)
		{
		}

		hm = 0;
	}		

	pinfo = 0;

	return true;
}

//**** CFxCallback ************************************************************************************************************************************

void CPsycle::MessBox(char *ptxt,char *caption,unsigned int type)
{
	::MessageBox(::GetForegroundWindow(),ptxt,caption,type);
}

int CPsycle::CallbackFunc(int cbkID,int par1,int par2,int par3)
{
	return 0;
}

float *CPsycle::GetWaveLData(int inst,int wave)
{
	return 0;
}

float *CPsycle::GetWaveRData(int inst,int wave)
{
	return 0;
}

int CPsycle::GetTickLength(void)
{
	return _master_info->samples_per_tick;
}

int CPsycle::GetSamplingRate(void)
{
	return _master_info->samples_per_second;
}

int CPsycle::GetBPM(void)
{
	return _master_info->beats_per_minute;
}

int CPsycle::GetTPB(void)
{
	return _master_info->ticks_per_beat;
}

//**** plugin *****************************************************************************************************************************************

void CPsycle::init(archive *arc)
{	
	if (!Open())
	{
		return;	
	}

	if (arc)
	{
		instream *istr = arc->get_instream("");
		
		if (istr)
		{
			int n = 0;
			
			istr->read<int>(n);

			if (n==1)
			{				
				istr->read<int>(n);

				if (n>0)
				{
					TAutoArrayPtr<psycle::uint8> data(n,true);

					istr->read(data,n);

					mi->PutData( data.ToPtr<psycle::uint8>() );				
				}
			}			
		}		
	}
}

void CPsycle::save(archive *arc)
{		
	if (!arc) return;
	if (!mi) return;
		
	outstream *ostr = arc->get_outstream("");

	if (ostr)					
	{
		const int sizeBytes = mi->GetDataSize();

		if (sizeBytes>0)
		{
			TAutoArrayPtr<psycle::uint8> data(sizeBytes,true);

			mi->GetData( data.ToPtr<psycle::uint8>() );

			ostr->write<int>(eVersion);
			ostr->write<int>(sizeBytes);
			ostr->write(data,sizeBytes);
		}
	}
}

void CPsycle::destroy(void)
{
	Close();

	delete this;
}

void CPsycle::process_events(void)
{	
	if (!mi) return;	

	const int n = int(info->global_parameters.size());
	
	for (int i=0;i<n;++i)
	{
		const zzub::parameter &srcParam = *info->global_parameters[i];		
		const psycle::CMachineParameter &dstParam = *pinfo[i];

		const int val = global[i];
				
		if (val!=srcParam.value_none)
		{						
			mi->ParameterTweak( i , Scale( Normalize(val,srcParam.value_min,srcParam.value_max) , dstParam.MinValue, dstParam.MaxValue ) );
		}
	}
	
	for (i=0;i<numTracks;++i)
	{
		const STrackParam &param = track[i];

		if (param.note!=zzub::note_value_none)
		{			
			if (param.note!=zzub::note_value_off)
			{				
				const int note = ( ( param.note >> 4 ) * 12 ) + ( param.note & 15 );

				mi->SeqTick(i,note,0,param.command>>8,param.command&0xff);				
			}
			else
			{								
				mi->SeqTick(i,psycle::NOTE_NO,0,0,0);				
			}
		}			
	}
}

bool CPsycle::process_stereo(float **pin,float **pout,int numsamples,int mode)
{	
	if (mi)
	{
		if (mode&process_mode_write)
		{
			float *ldst = pout[0];
			float *rdst = pout[1];

			const float *lsrc = pin[0];
			const float *rsrc = pin[1];

			int n = numsamples;

			while (--n>=0)
			{
				*ldst++ = *lsrc++ * 32768.0f;
				*rdst++ = *rsrc++ * 32768.0f;		
			}

			mi->Work(pout[0],pout[1],numsamples,numTracks);

			n = numsamples;

			ldst = pout[0];
			rdst = pout[1];

			while (--n>=0)
			{
				*ldst++ /= 32768.0f;
				*rdst++ /= 32768.0f;
			}		

			lsrc = pout[0];
			rsrc = pout[1];

			while (--numsamples>=0)
			{
				if ( !IsDenormal(*lsrc) || !IsDenormal(*rsrc) )
				{
					return true;
				}

				++lsrc;
				++rsrc;
			}		
		}				
	}

	return false;
}

void CPsycle::stop(void)
{
	if (mi)
	{
		mi->Stop();
	}
}

void CPsycle::attributes_changed(void)
{

}

void CPsycle::command(int index)
{
	if (!mi) return;		

	if (!index)
	{
		mi->Command();
	}
	else
	{
		char buffer[32] = {0};
		string text;

		text =  "Copyright (C) 2007 by Frank Potulski (polac@gmx.de)\n\n";
		text += "now wrapping...\n\n";
		text += "Name:\t\t";
		text += info->name;
		text += "\nLabel:\t\t";
		text += info->short_name;
		text += "\nAuthor:\t\t";
		text += info->author;
		text += "\n\nPath:\t\t";
		text += info->psy_path;

		text += "\n\nNumParams:\t";
		::sprintf(buffer,"%d",info->global_parameters.size());
		text += buffer;
		
		::MessageBox( ::GetForegroundWindow() , text.c_str() , "Polac Psycle Loader v0.01a" , MB_OK );
	}
}

void CPsycle::set_track_count(int count)
{
	numTracks = count;
}

void CPsycle::mute_track(int index)
{
	if (mi)
	{
		mi->MuteTrack(index);
	}
}

bool CPsycle::is_track_muted(int index) const
{
	if (mi)
	{
		return mi->IsTrackMuted(index);
	}

	return false;
}

void CPsycle::midi_note(int channel,int value,int velocity)
{
	if (mi)
	{
		mi->MidiNote(channel,value,velocity);
	}
}

void CPsycle::midi_control_change(int ctrl,int channel,int value)
{
}

void CPsycle::event(unsigned int data)
{	
}

const char *CPsycle::describe_value(int param,int value)
{
	describe[0] = 0;

	if (mi)
	{
		if ( param < int(info->global_parameters.size()) )
		{
			const zzub::parameter &srcParam = *info->global_parameters[param];		
			const psycle::CMachineParameter &dstParam = *pinfo[param];			

			value = Scale( Normalize(value,srcParam.value_min,srcParam.value_max) , dstParam.MinValue , dstParam.MaxValue );

			if (!mi->DescribeValue(describe,param,value))
			{
				::sprintf(describe,"%d",value);
			}
		}
		else
		{
			::sprintf(describe,"%.2X %.2X", value >> 8 , value & 0xff );
		}
	}
	
	return describe;
}

const envelope_info **CPsycle::get_envelope_infos(void)
{
	return 0;
}

bool CPsycle::play_wave(int wave,int note,float volume)
{
	return false;
}

void CPsycle::stop_wave(void)
{

}

int CPsycle::get_wave_envelope_play_position(int env)
{
	return -1;
}

const char *CPsycle::describe_param(int param)
{			
	return 0;
}

bool CPsycle::set_instrument(const char *name)
{
	return false;
}

void CPsycle::get_sub_menu(int index,outstream *os)
{

}

void CPsycle::add_input(const char *name)
{

}

void CPsycle::delete_input(const char *name)
{

}

void CPsycle::rename_input(const char *oldname,const char *newname)
{

}

void CPsycle::input(float **samples,int size,float amp)
{

}

bool CPsycle::handle_input(int index,int amp,int pan)
{
	return false;
}

//*****************************************************************************************************************************************************

ZZUB_EXTERN_C const char *zzub_get_signature(void) {	
	return ZZUB_SIGNATURE;
}

ZZUB_EXTERN_C plugincollection *zzub_get_plugincollection(void)
{	
	return new CPsycleCollection();
}

//*****************************************************************************************************************************************************