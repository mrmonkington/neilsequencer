/* lad2zzub.cpp

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

#include "lad2zzub.h"

#include "other\autoptr.h"

//*****************************************************************************************************************************************************

BOOL APIENTRY DllMain(HINSTANCE hm,DWORD dw,LPVOID lp)
{
	switch (dw)
	{

	case DLL_PROCESS_ATTACH:
		{
			CLadspa::hInst = hm;			
		}
		break;

	case DLL_PROCESS_DETACH:
		{

		}
		break;
	
	}
	
	return TRUE;
}

//**** CLadspaCollection ******************************************************************************************************************************

CLadspaCollection::CLadspaPlugins::CIterator::CIterator()
{
}

CLadspaCollection::CLadspaPlugins::CIterator::CIterator(CLadspaInfo::CIterator _it,CLadspaInfo::CIterator _end) : it(_it), end(_end)
{

}
			
CLadspaCollection::CLadspaPlugins::CIterator::~CIterator()
{
}

const CLadspaCollection::CLadspaInfo &CLadspaCollection::CLadspaPlugins::CIterator::operator *(void) const
{
	return *(*it);
}

const CLadspaCollection::CLadspaInfo *CLadspaCollection::CLadspaPlugins::CIterator::operator ->(void) const
{
	return *it;
}

bool CLadspaCollection::CLadspaPlugins::CIterator::Next(void)
{
	++it;

	return (it!=end);	
}

bool CLadspaCollection::CLadspaPlugins::CIterator::End(void) const
{
	return (it==end);
}
		

CLadspaCollection::CLadspaPlugins::CLadspaPlugins()
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

			::strcat(path,"\\Gear\\Ladspa\\");

			::CreateDirectory(path,NULL);

			char datpath[ MAX_PATH + 32 ];

			::strcpy(datpath,path);

			::strcat(datpath,"ladspa.dat");

			list<SPluginScan *> scan;

			HANDLE handle = ::CreateFile(datpath,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

			if (handle!=INVALID_HANDLE_VALUE)
			{				
				DWORD size = ::GetFileSize(handle,0);

				if (size)
				{
					string buffer;

					for (;;)
					{								
						if (ReadDatString(handle,buffer))
						{
							SPluginScan *plug = new SPluginScan();

							if (plug)
							{
								const int n = int(ReadDatDword(handle));																

								if (n>0)
								{
									plug->numPlugins = n;

									plug->path = buffer;

									plug->plugins.resize(n);

									vector<SPluginInfo> &pl = plug->plugins;

									for (int i=0;i<n;++i)
									{
										ReadDatString(handle,pl[i].name);
										ReadDatString(handle,pl[i].label);
										ReadDatString(handle,pl[i].author);										
										
										pl[i].uniqueID = ReadDatDword(handle);
										pl[i].index = ReadDatDword(handle);
										pl[i].numParams = ReadDatDword(handle);
										pl[i].numDesc = n;
									}

									scan.push_back(plug);
								}
							}						
						}
						else
						{
							break;
						}
					}

					::CloseHandle(handle);
				}
				else
				{
					::CloseHandle(handle);

					::DeleteFile(datpath);				
				}
			}

			handle = ::CreateFile(datpath,GENERIC_WRITE,0,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
			
			if (handle!=INVALID_HANDLE_VALUE)
			{
				::SetFilePointer(handle,0,0,FILE_END);

				ScanPlugins(handle,path,scan);						
		
				::CloseHandle(handle);
			}	

			if (scan.size())
			{
				list<SPluginScan *>::iterator it = scan.begin();
				const list<SPluginScan *>::iterator end = scan.end();

				while (it!=end)
				{
					delete (*it);

					++it;
				}
				
				scan.clear();
			}
		}
	}
}


bool CLadspaCollection::CLadspaPlugins::WriteDatDword(HANDLE handle,const DWORD d)
{	
	if (handle==INVALID_HANDLE_VALUE) 
	{
		return false;
	}
	
	DWORD size = 0;

	return ::WriteFile(handle,&d,sizeof(DWORD),&size,0)!=0;
}

bool CLadspaCollection::CLadspaPlugins::WriteDatString(HANDLE handle,const string &str)
{	
	if ( handle==INVALID_HANDLE_VALUE || str.empty() ) 
	{
		return false;
	}

	const int len = (int)str.length();

	DWORD size = 0;

	if (!WriteDatDword(handle,len))
	{
		return false;
	}
	
	return ::WriteFile(handle,str.c_str(),len,&size,0)!=0;	
}

DWORD CLadspaCollection::CLadspaPlugins::ReadDatDword(HANDLE handle)
{		
	DWORD size = 0;

	DWORD ret = 0;

	if ( handle==INVALID_HANDLE_VALUE || !::ReadFile(handle,&ret,sizeof(DWORD),&size,0) || size!=sizeof(DWORD) )
	{
		return 0xffffffff;
	}
	else
	{
		return ret;
	}
}

bool CLadspaCollection::CLadspaPlugins::ReadDatString(HANDLE handle,string &str)
{	
	DWORD len = ReadDatDword(handle);
	
	if ( !len || len==0xffffffff ) 
	{
		return false;
	}	
	
	TAutoArrayPtr<char> buf(len+1,true);

	DWORD size = 0;	

	if ( !::ReadFile(handle,buf,len,&size,0) || size!=len )
	{
		return false;
	}

	str = buf;	

	return true;
}

CLadspaCollection::SPluginScan *CLadspaCollection::CLadspaPlugins::IsScanned(const char *path,list<SPluginScan *> &scan)
{
	if ( scan.size() && path && path[0] )
	{
		SPluginScan find;

		find.path = path;
			 
		list<SPluginScan *>::iterator it = scan.begin();
		const list<SPluginScan *>::iterator end = scan.end();

		while (it!=end)
		{
			if ( *(*it) == find )
			{
				return (*it);
			}	

			++it;
		}				
	}

	return 0;
}

void CLadspaCollection::CLadspaPlugins::ScanPlugins(HANDLE handle,char *path,list<SPluginScan *> &scan)
{
	if (handle==INVALID_HANDLE_VALUE || !path || !path[0] ) 
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
				ScanPlugins(handle,tmp,scan);
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

		SPluginScan *scanned = IsScanned(tmp,scan);

		if (scanned)
		{
			if (scanned->numPlugins>0)
			{				
				const int n = scanned->numPlugins;

				if (n==scanned->plugins.size())
				{
					for (int i=0;i<n;++i)
					{
						CLadspaCollection::CLadspaInfo *plugInfo = new CLadspaCollection::CLadspaInfo(tmp,scanned->plugins[i]);

						if (plugInfo)
						{
							plugins.push_back(plugInfo);
						}					
					}					
				}				
			}	
		}
		else
		{
			CLadspaPlugin ladspa;

			if (ladspa.open(tmp))
			{
				unsigned int n = 0;

				while (ladspa.getDescriptor(n))
				{
					++n;
				}

				if (n>0)
				{
					WriteDatString(handle,tmp);

					WriteDatDword(handle,n);

					for (unsigned int i=0;i<n;++i)
					{						
						if (ladspa.loadLadspa(i))
						{							
							WriteDatString(handle,ladspa.getName());
							WriteDatString(handle,ladspa.getLabel());
							WriteDatString(handle,ladspa.getAuthor());							
							WriteDatDword(handle,ladspa.getUniqueID());
							WriteDatDword(handle,i);
							WriteDatDword(handle,DWORD(ladspa.getNumParams()));

							CLadspaCollection::SPluginInfo info;

							info.name = ladspa.getName();
							info.label = ladspa.getLabel();
							info.author = ladspa.getAuthor();
							info.uniqueID = ladspa.getUniqueID();
							info.index = i;
							info.numDesc = n;
							info.numParams = ladspa.getNumParams();

							CLadspaCollection::CLadspaInfo *plugInfo = new CLadspaCollection::CLadspaInfo(tmp,info);

							if (plugInfo)
							{
								plugins.push_back(plugInfo);
							}

							ladspa.freeLadspa();	
						}
					}
				}

				ladspa.close();			
			}						
		}		
		
		BOOL b = ::FindNextFile(fHandle,&x);

		if (!b) 
		{
			::FindClose(fHandle);

			return;
		}	
	}	
}

CLadspaCollection::CLadspaPlugins::~CLadspaPlugins()
{
	if (plugins.size())
	{		
		CLadspaInfo::CIterator it = plugins.begin();
		CLadspaInfo::CIterator end = plugins.end();

		while (it!=end)
		{
			delete (*it);

			++it;
		}

		plugins.clear();
	}
}

int CLadspaCollection::CLadspaPlugins::GetNumPlugins(void) const
{
	return int(plugins.size());
}

bool CLadspaCollection::CLadspaPlugins::IsEmpty(void) const
{
	return !plugins.size();
}

CLadspaCollection::CLadspaPlugins::CIterator CLadspaCollection::CLadspaPlugins::GetPlugins(void)
{
	return CIterator(plugins.begin(),plugins.end());
}
		
CLadspaCollection::CLadspaInfo::CLadspaInfo() : info()
{
	version = zzub::version;
	flags = zzub::plugin_flag_mono_to_stereo | zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;

	min_tracks = CLadspa::eMinTracks;
	max_tracks = CLadspa::eMaxTracks;	

	name = "Polac Ladspa Loader";
	short_name = "PLAD";
	author = "Frank Potulski";	
	uri = "@xlutop.com/lad2zzub";
	commands = "&About...";
}

CLadspaCollection::CLadspaInfo::CLadspaInfo(const char *path,const CLadspaCollection::SPluginInfo &pinfo) : info()
{
	version = zzub::version;
	flags = zzub::plugin_flag_mono_to_stereo | zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;

	min_tracks = CLadspa::eMinTracks;
	max_tracks = CLadspa::eMaxTracks;	
	
	lad_index = pinfo.index;
	lad_num_desc = pinfo.numDesc;
	lad_path = path;
	lad_name = pinfo.name;
	lad_label = pinfo.label;
	lad_author = pinfo.author;

	lad_uri = "@xlutop.com/lad2zzub/";
	lad_uri += lad_name;

	ReplaceChar((char *)lad_uri.c_str(),' ','+');

	/*lad_uri = "ladspa:";
	lad_uri += lad_author;
	lad_uri += "/";
	lad_uri += lad_name;*/

	lad_uid = pinfo.uniqueID;

	lad_commands = "About ";
	lad_commands += lad_name;
	lad_commands += "...";

	name = lad_name.c_str();
	short_name = lad_label.c_str();
	author = lad_author.c_str();
	uri = lad_uri.c_str();
	commands = lad_commands.c_str();

	int n = pinfo.numParams;

	if (n>0)
	{
		while (--n>=0)
		{
			char *cp = new char[32];

			if (cp)
			{
				::memset(cp,0,32);

				lad_param_name.push_back(cp);
			}
		}

		n = pinfo.numParams;

		for (int i=0;i<n;++i)
		{
			parameter &global = add_global_parameter();

			global.set_word();

			global.set_name(lad_param_name[i]);
			global.set_description(lad_param_name[i]);

			global.set_flags(parameter_flag_state);

			global.set_value_min(CLadspa::eMinParam);
			global.set_value_max(CLadspa::eMaxParam);
			global.set_value_default(CLadspa::eMinParam);
		}

		parameter &inertia = add_global_parameter();

		inertia.set_word();

		inertia.set_name("Inertia");
		inertia.set_description("Inertia in ticks");

		inertia.set_flags(parameter_flag_state);

		inertia.set_value_min(0);
		inertia.set_value_max(CLadspa::eMaxInertia);
		inertia.set_value_default(CLadspa::eDefaultInertia);
	}
}

CLadspaCollection::CLadspaInfo::~CLadspaInfo()
{
	int n = int(lad_param_name.size());

	if (n>0)
	{		
		while (--n>=0)
		{
			char *cp = lad_param_name[n];

			if (cp)
			{
				delete cp;
			}
		}

		lad_param_name.clear();
	}
}

void CLadspaCollection::CLadspaInfo::ReplaceChar(char *str,const char toReplace,const char replacedBy)
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

bool CLadspaCollection::CLadspaInfo::operator <(const CLadspaInfo &other) const
{
	return ( lad_path.compare(other.lad_path) < 0 );
}

bool CLadspaCollection::CLadspaInfo::operator ==(const CLadspaInfo &other) const
{
	return ( lad_path.compare(other.lad_path) == 0 );
}

plugin *CLadspaCollection::CLadspaInfo::create_plugin(void) const
{		
	return new CLadspa(this);
}
	
bool CLadspaCollection::CLadspaInfo::store_info(archive *arc) const
{
	return false;
}


CLadspaCollection::CLadspaCollection() : factory(0), plugins(0)
{
}

CLadspaCollection::~CLadspaCollection()
{	
}

void CLadspaCollection::initialize(pluginfactory *factory)
{	
	this->factory = factory;

	if (this->factory)
	{		
		plugins = new CLadspaPlugins();

		if ( plugins && !plugins->IsEmpty() )
		{
			CLadspaPlugins::CIterator it = plugins->GetPlugins();

			do
			{				
				const CLadspaInfo &plugInfo = (const CLadspaInfo &)*it;				

				this->factory->register_info(&plugInfo);
			}
			while (it.Next());
		}
	}
}
		
const info *CLadspaCollection::get_info(const char *uri,archive *arc)
{	
	if ( uri && arc )
	{
		if ( plugins && !plugins->IsEmpty() )
		{
			CLadspaPlugins::CIterator it = plugins->GetPlugins();

			do
			{				
				const CLadspaInfo &plugInfo = *it;

				if (!::strcmp(uri,plugInfo.lad_uri.c_str()))
				{
					return &plugInfo;
				}
			}
			while (it.Next());
		}
	}
	
	return 0;
}

const char *CLadspaCollection::get_uri(void)
{	
	return 0;
}
		
void CLadspaCollection::configure(const char *key,const char *value)
{
	int i=0;	
}
		
void CLadspaCollection::destroy(void)
{	
	if (plugins)
	{
		delete plugins;

		plugins = 0;
	}
	
	factory = 0;

	delete this;
}

//**** CLadspa ****************************************************************************************************************************************

HINSTANCE CLadspa::hInst = 0;

CLadspa::CLadspa(const CLadspaCollection::CLadspaInfo *_info) : info((CLadspaCollection::CLadspaInfo *)_info), ladspa(0), ladspaInitialized(false), cm(0), ins(0), outs(0), paramInertia(0), inertia(eDefaultInertia)
{
	global_values = 0;
	track_values = 0;
	attributes = 0;

	if (info)
	{		
		params.Initialize(int(info->global_parameters.size()));

		global_values = params.params;
	}
}

CLadspa::~CLadspa()
{
}

void CLadspa::init(archive *arc)
{
	if (!info) return;
	if (!_host) return;
	if (!_master_info) return;
	
	inertia = eDefaultInertia;

	cm = _host->get_metaplugin();	

	ladspa = new CLadspaPlugin();

	if (!ladspa) return;	

	if (!ladspa->open(info->lad_path.c_str())) return;
	if (!ladspa->loadLadspa(info->lad_index)) return;
	if (!ladspa->createInstance(_master_info->samples_per_second,zzub_buffer_size)) return;

	if (info->lad_param_name.size())
	{
		int n = int(info->lad_param_name.size());

		paramInertia = new SParam[n];

		if (!paramInertia) return;							

		while (--n>=0)
		{					
			parameter *param = (parameter *)info->global_parameters[n];

			paramInertia[n].newParam = param->value_default = int( ladspa->getDefParamVal(n) * eMaxParam );
			paramInertia[n].curParam = float(paramInertia[n].newParam);
			paramInertia[n].inertia = 0;

			::strncpy(info->lad_param_name[n],ladspa->getParamName(n),31);
		}
	}
	
	unsigned int n = ladspa->getNumOutputs();

	if ( n==0 || n>2 ) return;	

	if (n)
	{
		outs = new float *[n];

		for (unsigned int i=0;i<n;++i)
		{
			outs[i] = new float[zzub_buffer_size];

			::memset( outs[i] , 0 , zzub_buffer_size * sizeof(float) );
		}
	}

	n = ladspa->getNumInputs();	

	if (n>2) return;			

	if (n)
	{
		ins = new float *[n];

		for (unsigned int i=0;i<n;++i)
		{
			ins[i] = new float[zzub_buffer_size];

			::memset( ins[i] , 0 , zzub_buffer_size * sizeof(float) );
		}
	}	

	if (arc)
	{
		int version = 0;

		instream *istr = arc->get_instream("");
		
		if ( istr && istr->size()==sizeof(int) )
		{
			istr->read<int>(version);			
		}

		ladspaInitialized = (version==eVersion) ? true : false;		
	}
	else		 
	{
		ladspaInitialized = true;
	}	
}

void CLadspa::save(archive *arc)
{		
	if (!arc) return;
	if (!ladspaInitialized) return;
		
	outstream *ostr = arc->get_outstream("");

	if (ostr)					
	{			
		ostr->write<int>(eVersion);
	}
}

void CLadspa::destroy(void)
{		
	if (ladspa)
	{	
		if (ins)
		{
			unsigned int n = ladspa->getNumInputs();
			
			while (n--)
			{
				delete[] ins[n];
			}

			delete[] ins;

			ins = 0;
		}

		if (outs)
		{
			unsigned int n = ladspa->getNumOutputs();
			
			while (n--)
			{
				delete[] outs[n];
			}			

			delete[] outs;

			outs = 0;
		}		

		delete ladspa;

		ladspa = 0;
	}

	if (paramInertia)
	{
		delete[] paramInertia;

		paramInertia = 0;
	}

	info = 0;

	cm = 0;

	delete this;
}

void CLadspa::process_events(void)
{
	if (!ladspaInitialized) return;

	const int n = ladspa->getNumParams();

	if (n>0)
	{
		const unsigned short &value = params[n];

		if (value!=0xffff)
		{			
			inertia = value;
		}

		if (!inertia)
		{
			for (int i=0;i<n;++i)
			{
				const unsigned short &value = params[i];

				if (value!=0xffff)
				{					
					paramInertia[i].curParam = value;
					paramInertia[i].newParam = value;
					paramInertia[i].inertia = 0;

					ladspa->setParameter( i , float(value) / eMaxParam );
				}
			}
		}
		else
		{
			for (int i=0;i<n;++i)
			{
				const unsigned short &value = params[i];

				if (value!=0xffff)
				{										
					paramInertia[i].newParam = value;

					paramInertia[i].inertia = float( ( paramInertia[i].newParam - paramInertia[i].curParam ) /  ( ( float(inertia) / eInertiaTick ) * _master_info->samples_per_tick ) );
				}
			}
		}
	}	
}

bool CLadspa::process_stereo(float **pin,float **pout,int numsamples,int mode)
{
	if (!ladspaInitialized) return false;

	if (inertia)
	{
		const int n = ladspa->getNumParams();

		if (n>0)
		{		
			for (int i=0;i<n;++i)
			{
				SParam &prm = paramInertia[i];

				if (prm.curParam>prm.newParam) 
				{
					prm.curParam += ( prm.inertia * numsamples );

					if (prm.curParam<prm.newParam) 
					{
						prm.curParam = float(prm.newParam);
					}

					ladspa->setParameter( i , prm.curParam / eMaxParam );
				}
				else if (prm.curParam<prm.newParam) 
				{
					prm.curParam += ( prm.inertia * numsamples );

					if (prm.curParam>prm.newParam)
					{
						prm.curParam = float(prm.newParam);
					}					

					ladspa->setParameter( i , prm.curParam / eMaxParam );
				}					 			
			}
		}
	}	

	int ns = 0;
	
	const unsigned int ni =ladspa->getNumInputs();

	if (ni==2)
	{
		switch (mode)
		{

		case zzub_process_mode_write:
			{
				for (unsigned int i=0;i<ni;++i)
				{
					::memset( ins[i] , 0 , numsamples * sizeof(float) );
				}			
			}
			break;

		case zzub_process_mode_read_write:
			{
				float *fpout_l = ins[0];
				float *fpout_r = ins[1];

				const float *fpin_l = pin[0];
				const float *fpin_r = pin[1];								

				ns = numsamples;

				while (--ns>=0)
				{
					*fpout_l++ = *fpin_l++;
					*fpout_r++ = *fpin_r++;
				}				
			}
			break;		
		
		}	
	}
	else
	{
		switch (mode)
		{

		case zzub_process_mode_write:
			{				
				::memset( ins[0] , 0 , numsamples * sizeof(float) );			
			}
			break;

		case zzub_process_mode_read_write:
			{				
				float *fpout = ins[0];

				const float *fpin_l = pin[0];
				const float *fpin_r = pin[1];
				
				ns = numsamples;

				while (--ns>=0)
				{
					*fpout++ = ( *fpin_l++ + *fpin_r++ ) * 0.5f;
				}				
			}
			break;		
		
		}	
	}

	if (mode&zzub_process_mode_write)
	{
		ladspa->processReplacing(ins,outs,0,numsamples);

		const unsigned int no =ladspa->getNumOutputs();

		if (no==2)
		{			
			for (unsigned int i=0;i<no;++i)
			{
				float *fpout = pout[i];			
				const float *fpin = outs[i];

				ns = numsamples;

				while (--ns>=0)
				{
					*fpout++ = *fpin++;
				}
			}			
		}
		else
		{			
			float *fpout_l = pout[0];
			float *fpout_r = pout[1];

			const float *fpin = outs[0];			

			ns = numsamples;

			while (--ns>=0)
			{			
				*fpout_l = ( *fpin++ * 0.5f );				
				*fpout_r++ = *fpout_l++;
			}		
		}

		const float *fp_l = pout[0];
		const float *fp_r = pout[1];

		ns = numsamples;

		while (--ns>=0)
		{
			if ( !IsDenormal(*fp_l++) || !IsDenormal(*fp_r++) )
			{
				return true;
			}			
		}		
	}

	return false;
}

void CLadspa::stop(void)
{
}

void CLadspa::attributes_changed(void)
{
	int i=0;
}

void CLadspa::command(int index)
{
	if (index==0)
	{		
		string buffer;

		buffer = "Copyright (C) 2007 Frank Potulski (polac@gmx.de)\n\nnow wrapping...\n\n";

		if (ladspaInitialized) 
		{
			char digits[64];
			
			buffer += "Name:\t\t";
			buffer += info->lad_name;
			buffer += "\n";
			buffer += "ShortName:\t";
			buffer += info->lad_label;
			buffer += "\n";
			buffer += "Author:\t\t";
			buffer += info->lad_author;
			buffer += "\n";
			buffer += "Copyright:\t";
			buffer += ladspa->getCopyright();
			buffer += "\n\n";
			buffer += "Path:\t\t";
			buffer += info->lad_path;
			buffer += "\n\n";
			buffer += "DescriptorInUse:\t";
			::sprintf(digits,"%d",info->lad_index+1);
			buffer += digits;
			buffer += "/";
			::sprintf(digits,"%d",info->lad_num_desc);
			buffer += digits;
			buffer += "\n\n";
			buffer += "UniqueID:\t";
			::sprintf(digits,"%.8X (%d)",ladspa->getUniqueID(),ladspa->getUniqueID());
			buffer += digits;
			buffer += "\n";
			buffer += "\n";
			buffer += "NumParams:\t";
			::sprintf(digits,"%d",ladspa->getNumParams());
			buffer += digits;
			buffer += "\n";		
			buffer += "NumInputs:\t";
			::sprintf(digits,"%d",ladspa->getNumInputs());
			buffer += digits;
			buffer += "\n";
			buffer += "NumOutputs:\t";
			::sprintf(digits,"%d",ladspa->getNumOutputs());
			buffer += digits;
			buffer += "\n";
		}
		else
		{
			buffer += "nothing";
		}		

		::MessageBox(::GetForegroundWindow(),buffer.c_str(),"Polac Ladspa Loader v0.01a",MB_OK);
	}
}

void CLadspa::set_track_count(int count)
{
}

void CLadspa::mute_track(int index)
{
}

bool CLadspa::is_track_muted(int index) const
{
	return false;
}

void CLadspa::midi_note(int channel,int value,int velocity)
{
}

void CLadspa::midi_control_change(int ctrl,int channel,int value)
{
}

void CLadspa::event(unsigned int data)
{
}

const char *CLadspa::describe_value(int param,int value)
{
	paramDescribe[0] = 0;

	if (ladspaInitialized)
	{
		if (param>=0)
		{
			const int n = ladspa->getNumParams();
			
			if (param<n)
			{
				::sprintf( paramDescribe , "%.2f" , ladspa->getParameter(param,float(value)/eMaxParam) );
			}
			else if (param==n)
			{
				::sprintf( paramDescribe , "%.3f Ticks" , float(value)/eInertiaTick );
			}
		}			
	}
	else
	{
		::strcpy(paramDescribe,"?");
	}

	return paramDescribe;
}

const envelope_info **CLadspa::get_envelope_infos(void)
{
	return 0;
}

bool CLadspa::play_wave(int wave,int note,float volume)
{
	return false;
}

void CLadspa::stop_wave(void)
{

}

int CLadspa::get_wave_envelope_play_position(int env)
{
	return -1;
}

const char *CLadspa::describe_param(int param)
{			
	return 0;
}

bool CLadspa::set_instrument(const char *name)
{
	return false;
}

void CLadspa::get_sub_menu(int index,outstream *os)
{

}

void CLadspa::add_input(const char *name)
{

}

void CLadspa::delete_input(const char *name)
{

}

void CLadspa::rename_input(const char *oldname,const char *newname)
{

}

void CLadspa::input(float **samples,int size,float amp)
{

}

bool CLadspa::handle_input(int index,int amp,int pan)
{
	return false;
}

//*****************************************************************************************************************************************************

ZZUB_EXTERN_C const char *zzub_get_signature(void) {
	return ZZUB_SIGNATURE;
}

ZZUB_EXTERN_C plugincollection *zzub_get_plugincollection(void)
{	
	return new CLadspaCollection();
}

//*****************************************************************************************************************************************************