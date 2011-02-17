/* utils.h

   Free software by Richard W.E. Furse. Do with as you will. No
   warranty. */

#ifndef DSSI_SDK_LOAD_PLUGIN_LIB
#define DSSI_SDK_LOAD_PLUGIN_LIB

/*****************************************************************************/

#include "dssi.h"

/*****************************************************************************/

void * loadDSSIPluginLibrary(const char * pcPluginFilename);

void unloadDSSIPluginLibrary(void * pvDSSIPluginLibrary);

const DSSI_Descriptor *
findDSSIPluginDescriptor(void * pvDSSIPluginLibrary,
			   const char * pcPluginLibraryFilename,
			   const char * pcPluginLabel);

/*****************************************************************************/

typedef void DSSIPluginSearchCallbackFunction
(const char * pcFullFilename, 
 void * pvPluginHandle,
 DSSI_Descriptor_Function fDescriptorFunction);

void DSSIPluginSearch(DSSIPluginSearchCallbackFunction fCallbackFunction);

/*****************************************************************************/

/* Function in default.c: */

/* Find the default value for a port. Return 0 if a default is found
   and -1 if not. */
int getLADSPADefault(const LADSPA_PortRangeHint * psPortRangeHint,
		     const unsigned long          lSampleRate,
		     LADSPA_Data                * pfResult);

/*****************************************************************************/

#endif

/* EOF */
