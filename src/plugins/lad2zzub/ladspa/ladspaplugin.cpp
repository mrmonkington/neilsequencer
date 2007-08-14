/* ladspaplugin.cpp

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

#include "ladspaplugin.h"

#include "windows.h"
#include "stdio.h"
#include "math.h"

std::list<CLadspaList *> lplist;

inline float scale(float x,float min,float max)
{
	return expf( logf(min) * (1.0f-x) + logf(max) * x );

	
	/*const float offset = 0.001f;
	const float start = min + offset;
	const float end = max + offset;
	
	return start * pow( end / start,x) - offset;*/
}

CLadspaPlugin::CLadspaPlugin()
{		
	pDllPath = NULL;

	lp = NULL;

	plp = &lplist;	

	pDescriptor = NULL;

	pLadspaHandle = NULL;

	sampleRate = 44100;
	
	blockSize = 256;

	inparams = NULL;

	outparams = NULL;

	inputs = NULL;

	outputs = NULL;

	numInputs = 0;

	numOutputs = 0;
	
	numInParams = 0;

	numOutParams = 0;

	memset(dummy,0,2);
}

CLadspaPlugin::~CLadspaPlugin()
{
	close();
}


bool CLadspaPlugin::open(const char *path)
{
	close();
	
	if (!path) return false;
	if (!plp) return false;

	int len=(int)::strlen(path)+1;

	pDllPath = new char[len];

	if (!pDllPath) return false;

	::strcpy(pDllPath,path);

	lp=findPlugin(pDllPath);
	
	if (!lp)
	{
		lp = new CLadspaList(pDllPath);
		
		if (!lp)
		{
			close();

			return false;
		}

		if ( lp->addInstance()==0xffffffff )
		{
			close();

			return false;
		}
		
		plp->push_back(lp);
	}
	else
	{
		if ( lp->addInstance()==0xffffffff )
		{
			close();

			return false;
		}	
	}
		
	return true;
}

void CLadspaPlugin::close()
{			
	if (pDllPath)
	{
		delete[] pDllPath;

		pDllPath = 0;
	}

	freeLadspa();
			
	if ( lp && !lp->removeInstance() )
	{		
		removeFromList();
	}	

	pDescriptor = 0;
}

const LADSPA_Descriptor *CLadspaPlugin::getDescriptor(int index)
{	
	if (!lp) return NULL;
	
	return lp->getDescriptor(index);	
}

CLadspaList *CLadspaPlugin::findPlugin(char *c)
{
	if (!c) return 0;
	if (!plp) return 0;
	if (!plp->size()) return 0;

	for (std::list<CLadspaList *>::iterator it=plp->begin(); it!=plp->end(); ++it)
	{
		CLadspaList *lp = *it;

		if ( !_stricmp(c,lp->getDllPath()) )
		{
			return lp;
		}
	}

	return 0;
}

bool CLadspaPlugin::removeFromList(void)
{
	if (!lp) return false;
	if (!plp) return false;
	if (!plp->size()) return false;

	plp->remove(lp);

	delete lp;
	lp = NULL;

	return true;
}

unsigned long CLadspaPlugin::getNumDescriptors(void)
{			
	if (!lp) return 0;
	
	return (long)lp->descriptors.size();
}

const char *CLadspaPlugin::getAuthor(void) const
{
	if (!pDescriptor) return 0;	

	return pDescriptor->Maker;
}

const char *CLadspaPlugin::getCopyright(void) const
{
	if (!pDescriptor) return 0;	

	return pDescriptor->Copyright;
}

const char *CLadspaPlugin::getName(void) const
{
	if (!pDescriptor) return 0;	

	return pDescriptor->Name;
}

const char *CLadspaPlugin::getLabel(void) const
{
	if (!pDescriptor) return 0;	

	return pDescriptor->Label;
}

unsigned int CLadspaPlugin::getUniqueID(void)
{
	if (!pDescriptor) return 0;

	return pDescriptor->UniqueID;
}

bool CLadspaPlugin::loadLadspa(int index)
{				
	if (index<0) return false;
			
	freeLadspa();

	pDescriptor=getDescriptor(index);

	if (!pDescriptor) return false;	

	for (int i=0;i<pDescriptor->PortCount;i++)
	{						
		if (pDescriptor->PortDescriptors[i]&LADSPA_PORT_CONTROL)
		{												
			if (pDescriptor->PortDescriptors[i]&LADSPA_PORT_INPUT)
			{
				numInParams++;
			}
			else if (pDescriptor->PortDescriptors[i]&LADSPA_PORT_OUTPUT)
			{
				numOutParams++;
			}
		}
		else if (pDescriptor->PortDescriptors[i]&LADSPA_PORT_AUDIO)
		{
			if (pDescriptor->PortDescriptors[i]&LADSPA_PORT_INPUT)
			{
				numInputs++;
			}
			else if (pDescriptor->PortDescriptors[i]&LADSPA_PORT_OUTPUT)
			{
				numOutputs++;
			}
		}
	}			

	if (numInParams)
	{
		inparams=new PARAMS[numInParams];

		if (!inparams) return false;

		int count=0;

		for (int i=0;i<pDescriptor->PortCount;i++)
		{						
			if ( pDescriptor->PortDescriptors[i]&LADSPA_PORT_CONTROL && pDescriptor->PortDescriptors[i]&LADSPA_PORT_INPUT )
			{					
				inparams[count].name=pDescriptor->PortNames[i];

				getDefParamVal(count,&inparams[count].data,&inparams[count].min,&inparams[count].max);

				inparams[count].hint=&pDescriptor->PortRangeHints[i].HintDescriptor;

				count++;				
			}
		}
	}

	if (numOutParams)
	{
		outparams=new PARAMS[numOutParams];

		if (!outparams) return false;

		int count=0;

		for (int i=0;i<pDescriptor->PortCount;i++)
		{						
			if ( pDescriptor->PortDescriptors[i]&LADSPA_PORT_CONTROL && pDescriptor->PortDescriptors[i]&LADSPA_PORT_OUTPUT )
			{					
				outparams[count].name=pDescriptor->PortNames[i];

				getDefParamVal(count,&outparams[count].data,&outparams[count].min,&outparams[count].max);

				count++;
			}
		}
	}

	if (numInputs)
	{	
		inputs=new INOUTS[numInputs];
		if (!inputs) return false;

		for (int count=0,i=0;i<pDescriptor->PortCount;i++)
		{
			if (pDescriptor->PortDescriptors[i]&LADSPA_PORT_AUDIO)
			{
				if (pDescriptor->PortDescriptors[i]&LADSPA_PORT_INPUT)
				{	
					inputs[count].data=NULL;
					inputs[count++].name=pDescriptor->PortNames[i];					
				}

				if (count==numInputs) break;
			}			
		}	
	}

	if (numOutputs)
	{
		outputs=new INOUTS[numOutputs];
		if (!outputs) return false;

		for (int count=0,i=0;i<pDescriptor->PortCount;i++)
		{
			if (pDescriptor->PortDescriptors[i]&LADSPA_PORT_AUDIO)
			{
				if (pDescriptor->PortDescriptors[i]&LADSPA_PORT_OUTPUT)
				{	
					outputs[count].data=NULL;
					outputs[count++].name=pDescriptor->PortNames[i];					
				}

				if (count==numOutputs) break;
			}			
		}
	
	}
	
	return true;
}

bool CLadspaPlugin::freeLadspa(void)
{
	if (!pDescriptor) return false;

	destroyInstance();

	if (inparams)
	{
		delete[] inparams;
		inparams = NULL;
	}

	if (outparams)
	{
		delete[] outparams;
		outparams = NULL;
	}

	if (inputs)
	{
		delete[] inputs;
		inputs=NULL;
	}
	
	if (outputs)
	{
		delete[] outputs;
		outputs=NULL;
	}

	numInputs=0;
	numOutputs=0;
	numInParams=0;
	numOutParams=0;
	
	pDescriptor=NULL;	

	return true;
}


void CLadspaPlugin::setBlockSize(long bs)
{
	if (!bs) return;
	if (!pDescriptor) return;
	if (!pLadspaHandle) return;
			
	if (blockSize!=bs)
	{			
		createInstance(sampleRate,bs);
	}	
}

void CLadspaPlugin::setSampleRate(unsigned long sr)
{
	if (!sr) return;
	if (!pDescriptor) return;
	if (!pLadspaHandle) return;
	
	if (sampleRate!=sr)
	{			
		createInstance(sr,blockSize);
	}	
}

bool CLadspaPlugin::createInstance(unsigned long sr, long bs)
{
	if (!pDescriptor) return false;
	if (!sr) return false;
	if (!bs) return false;

	destroyInstance();

	sampleRate=sr;
	blockSize=bs;

	if ( numInputs && inputs )
	{		
		for (int i=0;i<numInputs;i++)
		{
			inputs[i].data=new float[blockSize];
			if (!inputs[i].data) 
			{
				delete[] inputs;
				inputs=NULL;

				return false;
			}
			
			memset(inputs[i].data,0,sizeof(LADSPA_Data)*blockSize);
		}				
	}

	if ( numOutputs && outputs )
	{		
		for (int i=0;i<numOutputs;i++)
		{
			outputs[i].data=new float[blockSize];
			if (!outputs[i].data) 
			{
				delete[] outputs;
				outputs=NULL;

				return false;
			}
			
			memset(outputs[i].data,0,sizeof(LADSPA_Data)*blockSize);
		}				
	}
	
	pLadspaHandle = pDescriptor->instantiate(pDescriptor,sampleRate);

	if (!pLadspaHandle) return false;		
	
	for (int ipCount=0,opCount=0,iCount=0,oCount=0,i=0;i<pDescriptor->PortCount;i++)
	{
		if (pDescriptor->PortDescriptors[i]&LADSPA_PORT_CONTROL)
		{
			if (pDescriptor->PortDescriptors[i]&LADSPA_PORT_INPUT)
			{
				pDescriptor->connect_port(pLadspaHandle,i,&inparams[ipCount++].data);
			}
			else if (pDescriptor->PortDescriptors[i]&LADSPA_PORT_OUTPUT)
			{
				pDescriptor->connect_port(pLadspaHandle,i,&outparams[opCount++].data);
			}
		}
		else if (pDescriptor->PortDescriptors[i]&LADSPA_PORT_AUDIO)
		{
			if (pDescriptor->PortDescriptors[i]&LADSPA_PORT_INPUT)
			{
				pDescriptor->connect_port(pLadspaHandle,i,inputs[iCount++].data);
			}
			else if (pDescriptor->PortDescriptors[i]&LADSPA_PORT_OUTPUT)
			{
				pDescriptor->connect_port(pLadspaHandle,i,outputs[oCount++].data);
			}
		}
	}

	if (pDescriptor->activate)
	{
		pDescriptor->activate(pLadspaHandle);
	}

	return true;
}

bool CLadspaPlugin::destroyInstance(void)
{		
	if (!pDescriptor) return false;
	if (!pLadspaHandle) return false;	

	if (pDescriptor->deactivate) 
	{
		pDescriptor->deactivate(pLadspaHandle);		
	}

	if (pDescriptor->cleanup)
	{
		try
		{
			pDescriptor->cleanup(pLadspaHandle);
		}
		catch(...){}
	}
		
	if (inputs)
	{
		for (int i=0;i<numInputs;i++)
		{
			if (inputs[i].data)
			{
				delete[] inputs[i].data;
				
				inputs[i].data=NULL;
			}
		}		
	}

	if (outputs)
	{
		for (int i=0;i<numOutputs;i++)
		{
			if (outputs[i].data)
			{
				delete[] outputs[i].data;

				outputs[i].data=NULL;
			}
		}		
	}	
	
	pLadspaHandle=NULL;

	return true;
}

float CLadspaPlugin::getDefParamVal(int idx,LADSPA_Data *df,LADSPA_Data *mi,LADSPA_Data *mx)
{
	if (!pDescriptor) return -1.f;
	if (idx<0) return -1.f;

	const LADSPA_PortRangeHintDescriptor *range = &pDescriptor->PortRangeHints[idx].HintDescriptor;
	const LADSPA_Data *lower = &pDescriptor->PortRangeHints[idx].LowerBound;
	const LADSPA_Data *upper = &pDescriptor->PortRangeHints[idx].UpperBound;

	float def=1.f;
	float min=0.f;
	float max=10.f;

	float ret = -1.f;

	if ( LADSPA_IS_HINT_BOUNDED_BELOW(*range) )
	{
		min = *lower;
	}

	if ( LADSPA_IS_HINT_BOUNDED_ABOVE(*range) )
	{
		max = *upper;
	}
					
	if ( LADSPA_IS_HINT_HAS_DEFAULT(*range) )
	{
		if ( LADSPA_IS_HINT_DEFAULT_MINIMUM(*range) )
		{
			def = min;

			ret = 0.f;
		}
		else if ( LADSPA_IS_HINT_DEFAULT_LOW(*range) )
		{					
			if ( LADSPA_IS_HINT_LOGARITHMIC(*range) )
			{
				def = expf( logf(min) * 0.75f + logf(max) * 0.25f );
			}
			else
			{
				def = ( min * 0.75f ) + ( max * 0.25f );
			}

			ret = 0.25f;
		}
		else if ( LADSPA_IS_HINT_DEFAULT_MIDDLE(*range) )
		{
			if ( LADSPA_IS_HINT_LOGARITHMIC(*range) )
			{
				def = expf( logf(min) * 0.5f + logf(max) * 0.5f );
			}
			else
			{
				def = ( min * 0.5f ) + ( max * 0.5f );
			}

			ret = 0.5f;
		}
		else if ( LADSPA_IS_HINT_DEFAULT_HIGH(*range) )
		{
			if ( LADSPA_IS_HINT_LOGARITHMIC(*range) )
			{
				def = expf( logf(min) * 0.25f + logf(max) * 0.75f );
			}
			else
			{
				def = ( min * 0.25f ) + ( max * 0.75f );
			}

			ret = 0.75f;
		}
		else if ( LADSPA_IS_HINT_DEFAULT_MAXIMUM(*range) )
		{
			def = max;

			ret = 1.f;
		}
		else if ( LADSPA_IS_HINT_DEFAULT_0(*range) )
		{
			def = 0.f;			
		}
		else if ( LADSPA_IS_HINT_DEFAULT_1(*range) )
		{
			def = 1.f;
		}
		else if ( LADSPA_IS_HINT_DEFAULT_100(*range) )
		{
			def = 100.f;
		}
		else if ( LADSPA_IS_HINT_DEFAULT_440(*range) )
		{
			def = 440.f;
		}
	}
	
	if (def>max) 
	{
		def=max;
	}
	else if (def<min) 
	{
		def=min;
	}
	
	if (ret==-1.f)
	{
		if ( LADSPA_IS_HINT_LOGARITHMIC(*range) )
		{		
			ret = ( logf(def) - logf(min) ) / ( logf(max) - logf(min) );
		}
		else
		{
			ret = 0.f;
			
			float f = (max-min);

			if (f)
			{
				ret = (def-min) / f;
			}			
		}									
	}

	if ( LADSPA_IS_HINT_SAMPLE_RATE(*range) )
	{
		def*=sampleRate;
		min*=sampleRate;
		max*=sampleRate;
	}	
	
	if ( LADSPA_IS_HINT_INTEGER(*range) )
	{
		def=(int)def;
	}	

	if (df) *df=def;
	if (mi) *mi=min;					
	if (mx) *mx=max;	
			
	return ret;
}

void CLadspaPlugin::setParameter(int idx, float val)
{
	if ( idx<0 || idx>=numInParams ) return;	
	if (!inparams) return;

	PARAMS *p=&inparams[idx];

	if ( LADSPA_IS_HINT_INTEGER(*p->hint) )
	{
		p->data = (int) ( p->min + ( (p->max - p->min) * val ) );
	}
	else if ( LADSPA_IS_HINT_TOGGLED(*p->hint) )
	{
		p->data=(int)(val+0.5f);
	}
	else if ( LADSPA_IS_HINT_LOGARITHMIC(*p->hint) )
	{
		p->data = scale(val,p->min,p->max);				
	}
	else
	{
		p->data =  p->min + ( (p->max - p->min) * val );
	}
}

/*float CLadspaPlugin::getParameter(int idx,float val)
{
	if ( idx<0 || idx>=numInParams ) return 0.f;	
	if (!inparams) return 0.f;	

	if (val!=-1.f)
	{
		PARAMS *p=&inparams[idx];

		float f = p->min + ( (p->max - p->min) * val );

		return f;
	}

	return inparams[idx].data;	
}*/

float CLadspaPlugin::getParameter(int idx,float val)
{
	if ( !inparams || idx<0 || idx>=numInParams ) return 0.0f;	

	if (val!=-1.0f)
	{
		float ret;

		const PARAMS *p = &inparams[idx];
		
		if ( LADSPA_IS_HINT_INTEGER(*p->hint) )
		{
			ret = float ( (int) ( p->min + ( (p->max - p->min) * val ) ) );
		}
		else if ( LADSPA_IS_HINT_TOGGLED(*p->hint) )
		{
			ret = float ( (int)(val+0.5f) );
		}
		else if ( LADSPA_IS_HINT_LOGARITHMIC(*p->hint) )
		{
			ret = scale(val,p->min,p->max);				
		}
		else
		{
			ret = p->min + ( (p->max - p->min) * val );
		}		

		return ret;
	}

	return inparams[idx].data;	
}

const char *CLadspaPlugin::getParamName(int idx)
{
	if ( idx<0 || idx>=numInParams ) return dummy;
	if (!inparams) return dummy;
	
	return inparams[idx].name;
}

const char *CLadspaPlugin::getInputName(int idx)
{
	if ( idx<0 || idx>=numInputs ) return dummy;
	if (!inputs) return dummy;

	return inputs[idx].name;
}

const char *CLadspaPlugin::getOutputName(int idx)
{
	if ( idx<0 || idx>=numOutputs ) return dummy;
	if (!outputs) return dummy;

	return outputs[idx].name;
}

void CLadspaPlugin::process(float **in,float **ou,int ofs,int ns)
{
	if ( pDescriptor && pLadspaHandle )
	{
		int samples;

		float *src;
		float *tgt;

		if (inputs)
		{						
			for (int i=0;i<numInputs;i++)
			{
				samples=ns;

				src=&in[i][ofs];
				tgt=inputs[i].data;

				while (samples--)
				{
					*tgt++ = *src++;
				}
			}									
		}

		pDescriptor->run(pLadspaHandle,ns);		

		if (outputs)
		{						
			for (int i=0;i<numOutputs;i++)
			{
				samples=ns;

				src=outputs[i].data;
				tgt=&ou[i][ofs];

				while (samples--)
				{
					*tgt++ += *src++;
				}
			}
		}
	}	
}

void CLadspaPlugin::processReplacing(float **in,float **ou,int ofs,int ns)
{	
	if ( pDescriptor && pLadspaHandle )
	{
		int samples;

		float *src;
		float *tgt;		

		if (inputs)
		{						
			for (int i=0;i<numInputs;i++)
			{
				samples=ns;

				src=&in[i][ofs];
				tgt=inputs[i].data;		

				while (samples--)
				{
					*tgt++ = *src++;					
				}
			}									
		}

		if (outputs)
		{						
			for (int i=0;i<numOutputs;i++)
			{
				samples=ns;

				tgt=outputs[i].data;

				while (samples--)
				{
					*tgt++ = 0;				
				}
			}
		}
		
		pDescriptor->run(pLadspaHandle,ns);		

		if (outputs)
		{						
			for (int i=0;i<numOutputs;i++)
			{
				samples=ns;

				src=outputs[i].data;
				tgt=&ou[i][ofs];

				while (samples--)
				{
					*tgt++ = *src++;
				}
			}
		}
	}	
}


