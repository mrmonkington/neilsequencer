/* load.c

   Free software by Richard W.E. Furse. Do with as you will. No
   warranty. */

/*****************************************************************************/

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*****************************************************************************/

#include "dssi.h"
#include "utils.h"

/*****************************************************************************/

/* This function provides a wrapping of dlopen(). When the filename is
   not an absolute path (i.e. does not begin with / character), this
   routine will search the DSSI_PATH for the file. */
static void *
dlopenDSSI(const char * pcFilename, int iFlag) {

  char * pcBuffer;
  const char * pcEnd;
  const char * pcDSSIPath;
  const char * pcStart;
  int iEndsInSO;
  int iNeedSlash;
  size_t iFilenameLength;
  void * pvResult;

  iFilenameLength = strlen(pcFilename);
  pvResult = NULL;

  if (pcFilename[0] == '/') {

    /* The filename is absolute. Assume the user knows what he/she is
       doing and simply dlopen() it. */

    pvResult = dlopen(pcFilename, iFlag);
    if (pvResult != NULL)
      return pvResult;

  }
  else {

    /* If the filename is not absolute then we wish to check along the
       DSSI_PATH path to see if we can find the file there. We do
       NOT call dlopen() directly as this would find plugins on the
       LD_LIBRARY_PATH, whereas the DSSI_PATH is the correct place
       to search. */

    pcDSSIPath = getenv("DSSI_PATH");
    
    if (pcDSSIPath) {

      pcStart = pcDSSIPath;
      while (*pcStart != '\0') {
	pcEnd = pcStart;
	while (*pcEnd != ':' && *pcEnd != '\0')
	  pcEnd++;
	
	pcBuffer = malloc(iFilenameLength + 2 + (pcEnd - pcStart));
	if (pcEnd > pcStart)
	  strncpy(pcBuffer, pcStart, pcEnd - pcStart);
	iNeedSlash = 0;
	if (pcEnd > pcStart)
	  if (*(pcEnd - 1) != '/') {
	    iNeedSlash = 1;
	    pcBuffer[pcEnd - pcStart] = '/';
	  }
	strcpy(pcBuffer + iNeedSlash + (pcEnd - pcStart), pcFilename);
	
	pvResult = dlopen(pcBuffer, iFlag);
	
	free(pcBuffer);
	if (pvResult != NULL)
	  return pvResult;
	
	pcStart = pcEnd;
	if (*pcStart == ':')
	  pcStart++;
      }
    }
  }

  /* As a last ditch effort, check if filename does not end with
     ".so". In this case, add this suffix and recurse. */
  iEndsInSO = 0;
  if (iFilenameLength > 3)
    iEndsInSO = (strcmp(pcFilename + iFilenameLength - 3, ".so") == 0);
  if (!iEndsInSO) {
    pcBuffer = malloc(iFilenameLength + 4);
    strcpy(pcBuffer, pcFilename);
    strcat(pcBuffer, ".so");
    pvResult = dlopenDSSI(pcBuffer, iFlag);
    free(pcBuffer);
  }

  if (pvResult != NULL)
    return pvResult;

  /* If nothing has worked, then at least we can make sure we set the
     correct error message - and this should correspond to a call to
     dlopen() with the actual filename requested. The dlopen() manual
     page does not specify whether the first or last error message
     will be kept when multiple calls are made to dlopen(). We've
     covered the former case - now we can handle the latter by calling
     dlopen() again here. */
  return dlopen(pcFilename, iFlag);
}

/*****************************************************************************/

void *
loadDSSIPluginLibrary(const char * pcPluginFilename) {

  void * pvPluginHandle;

  pvPluginHandle = dlopenDSSI(pcPluginFilename, RTLD_NOW);
  if (!pvPluginHandle) {
    fprintf(stderr, 
	    "Failed to load plugin \"%s\": %s\n", 
	    pcPluginFilename,
	    dlerror());
    exit(1);
  }

  return pvPluginHandle;
}

/*****************************************************************************/

void 
unloadDSSIPluginLibrary(void * pvDSSIPluginLibrary) {
  dlclose(pvDSSIPluginLibrary);
}

/*****************************************************************************/

const DSSI_Descriptor *
findDSSIPluginDescriptor(void * pvDSSIPluginLibrary,
			   const char * pcPluginLibraryFilename,
			   const char * pcPluginLabel) {

  const DSSI_Descriptor * psDescriptor;
  DSSI_Descriptor_Function pfDescriptorFunction;
  unsigned long lPluginIndex;

  dlerror();
  pfDescriptorFunction 
    = (DSSI_Descriptor_Function)dlsym(pvDSSIPluginLibrary,
					"dssi_descriptor");
  if (!pfDescriptorFunction) {
    const char * pcError = dlerror();
    if (pcError) {
      fprintf(stderr,
	      "Unable to find dssi_descriptor() function in plugin "
	      "library file \"%s\": %s.\n"
	      "Are you sure this is a DSSI plugin file?\n", 
	      pcPluginLibraryFilename,
	      pcError);
      exit(1);
    }
  }

  for (lPluginIndex = 0;; lPluginIndex++) {
    psDescriptor = pfDescriptorFunction(lPluginIndex);
    if (psDescriptor == NULL) {
      fprintf(stderr,
	      "Unable to find label \"%s\" in plugin library file \"%s\".\n",
	      pcPluginLabel,
	      pcPluginLibraryFilename);
      exit(1);      
    }
    if (strcmp(psDescriptor->LADSPA_Plugin->Label, pcPluginLabel) == 0)
      return psDescriptor;
  }
}

/*****************************************************************************/

/* EOF */
