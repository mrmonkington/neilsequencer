/* ladspaplugin.h

   Copyright (C) 2004-2007 Frank Potulski (polac@gmx.de)

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

#ifndef __LADSPAPLUGIN_H
#define __LADSPAPLUGIN_H

#include "ladspa.h"

#include <windows.h>
#include <list>
#include <set>

struct PARAMS
{
	PARAMS(){}
	
	float data;

	LADSPA_Data min;
	LADSPA_Data max;

	const char *name;

	const LADSPA_PortRangeHintDescriptor *hint;
};

struct INOUTS
{
	INOUTS(){};
	
	float *data;
	
	const char *name;
};

struct SDescriptor
{	
	SDescriptor(const LADSPA_Descriptor *d=0,unsigned int i=0)
	{
		pDescriptor=d;
		idx=i;
	}

	bool operator <(const SDescriptor &other)const	
	{
		return (idx<other.idx);													
	}
	
	bool operator==(const SDescriptor &other)const
	{		
		return (idx==other.idx);
	}

	unsigned int idx;

	const LADSPA_Descriptor *pDescriptor;
};

class CLadspaList
{	
public:

	CLadspaList(char *c)
	{
		if (!c) return;	

		memset(dllPath,0,MAX_PATH);
		strcpy(dllPath,c);

		hm = 0;
		instanceCount = 0;
		ladspa_descriptor = 0;
	}	

	~CLadspaList()
	{
		if (descriptors.size())
		{
			descriptors.clear();
		}

		if (hm)
		{
			FreeLibrary(hm);
			hm=0;
		}
				
		instanceCount = 0;
		ladspa_descriptor = 0;
	}	

	unsigned int addInstance(void)
	{		
		if ( !instanceCount && !hm )
		{
			try
			{
				hm=LoadLibrary(dllPath);

				if (!hm) return 0xffffffff;
			}
			catch(...)
			{
				hm = 0;
				
				return 0xffffffff;
			}

			try
			{
				ladspa_descriptor = (const LADSPA_Descriptor *(__cdecl *)(unsigned long)) GetProcAddress(hm,"ladspa_descriptor");
				
				if ( !ladspa_descriptor ) return 0xffffffff;
			}
			catch(...)
			{
				FreeLibrary(hm);
				hm = 0;

				return 0xffffffff;
			}
		}				
			
		return ++instanceCount;
	};

	unsigned int removeInstance(void)
	{	
		if (instanceCount>=1)
		{
			instanceCount--;
		}		

		if ( !instanceCount && hm )
		{
			FreeLibrary(hm);
			hm=0;
		}

		return instanceCount;
	};
	
	const LADSPA_Descriptor *getDescriptor(int index)
	{
		if ( !ladspa_descriptor ) return NULL;

		std::set<SDescriptor>::iterator it=descriptors.find( SDescriptor(0,index) );

		if (it!=descriptors.end())
		{
			SDescriptor *d = (SDescriptor *)&(*it);

			return d->pDescriptor;
		}
		else
		{
			LADSPA_Descriptor *pDescriptor;
			
			try
			{
				pDescriptor = (LADSPA_Descriptor *)ladspa_descriptor(index);
			}
			catch(...)
			{
				return NULL;
			}

			if (pDescriptor)
			{
				descriptors.insert( SDescriptor(pDescriptor,index) );
			}

			return pDescriptor;
		}

		return 0;
	}

	char *getDllPath(void)
	{
		return dllPath;
	}

	std::set<SDescriptor>					descriptors;

private:

	char									dllPath[MAX_PATH];

	int										instanceCount;
	
	const LADSPA_Descriptor					*(__cdecl *ladspa_descriptor)(unsigned long);

	HMODULE									hm;


};

class CLadspaPlugin
{
public:

	CLadspaPlugin();
	~CLadspaPlugin();

	bool open(const char *path);
	void close(void);

	void getPluginInfos(int index=0);

	bool loadLadspa(int index=0);
	bool freeLadspa(void);

	bool createInstance(unsigned long sr=44100, long bs=256);
	bool destroyInstance(void);

	void process(float **in,float **ou,int ofs,int ns);
	void processReplacing(float **in,float **ou,int ofs,int ns);

	const LADSPA_Descriptor *getDescriptor(int index);
	unsigned long getNumDescriptors(void);	
	
	const char *getAuthor(void) const;
	const char *getName(void) const;
	const char *getLabel(void) const;
	const char *getCopyright(void) const;

	unsigned int getUniqueID(void);
	
	int getNumParams(void){ return numInParams; };	
	unsigned int getNumInputs(void){return numInputs;};
	unsigned int getNumOutputs(void){return numOutputs;};	

	const char *getParamName(int idx);		
	const char *getInputName(int idx);
	const char *getOutputName(int idx);
	
	float getDefParamVal(int idx,LADSPA_Data *df=0,LADSPA_Data *mi=0,LADSPA_Data *mx=0);	
	
	void setParameter(int idx,float val);
	float getParameter(int idx,float val=-1.f);

	void setBlockSize(long bs);
	void setSampleRate(unsigned long sr);
		
	CLadspaList *findPlugin(char *c);
	bool removeFromList(void);	
		
private:
		
	char									*pDllPath;

	CLadspaList								*lp;

	std::list<CLadspaList *>				*plp;

	const LADSPA_Descriptor					*pDescriptor;	

	LADSPA_Handle							pLadspaHandle;
	
	PARAMS									*inparams;
	PARAMS									*outparams;

	INOUTS									*inputs;
	INOUTS									*outputs;
	
	unsigned long							sampleRate;
	long									blockSize;

	int										numInputs;	
	int										numOutputs;
	int										numInParams;
	int										numOutParams;

	char									dummy[2];

	
};

#endif