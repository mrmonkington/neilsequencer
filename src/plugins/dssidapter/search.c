/* search.c

   Free software by Richard W.E. Furse. Do with as you will. No
   warranty. */

/*****************************************************************************/

#include <dirent.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/*****************************************************************************/

#include "dssi.h"
#include "utils.h"

/*****************************************************************************/

/* Search just the one directory. */
static void
DSSIDirectoryPluginSearch
(const char * pcDirectory, 
 DSSIPluginSearchCallbackFunction fCallbackFunction) {

  char * pcFilename;
  DIR * psDirectory;
  DSSI_Descriptor_Function fDescriptorFunction;
  long lDirLength;
  long iNeedSlash;
  struct dirent * psDirectoryEntry;
  void * pvPluginHandle;

  lDirLength = strlen(pcDirectory);
  if (!lDirLength)
    return;
  if (pcDirectory[lDirLength - 1] == '/')
    iNeedSlash = 0;
  else
    iNeedSlash = 1;

  psDirectory = opendir(pcDirectory);
  if (!psDirectory)
    return;

  while (1) {

    psDirectoryEntry = readdir(psDirectory);
    if (!psDirectoryEntry) {
      closedir(psDirectory);
      return;
    }

    pcFilename = malloc(lDirLength
			+ strlen(psDirectoryEntry->d_name)
			+ 1 + iNeedSlash);
    strcpy(pcFilename, pcDirectory);
    if (iNeedSlash)
      strcat(pcFilename, "/");
    strcat(pcFilename, psDirectoryEntry->d_name);
    
    pvPluginHandle = dlopen(pcFilename, RTLD_LAZY);
    if (pvPluginHandle) {
      /* This is a file and the file is a shared library! */

      dlerror();
      fDescriptorFunction
	= (DSSI_Descriptor_Function)dlsym(pvPluginHandle,
					    "dssi_descriptor");
      if (dlerror() == NULL && fDescriptorFunction) {
	/* We've successfully found a dssi_descriptor function. Pass
           it to the callback function. */
	fCallbackFunction(pcFilename,
			  pvPluginHandle,
			  fDescriptorFunction);
	free(pcFilename);
      }
      else {
	/* It was a library, but not a DSSI one. Unload it. */
	dlclose(pcFilename);
	free(pcFilename);
      }
    }
  }
}

/*****************************************************************************/

void 
DSSIPluginSearch(DSSIPluginSearchCallbackFunction fCallbackFunction) {

  char * pcBuffer;
  const char * pcEnd;
  const char * pcDSSIPath;
  const char * pcStart;

  pcDSSIPath = getenv("DSSI_PATH");
  if (!pcDSSIPath) {
    fprintf(stderr,
	    "Warning: You do not have a DSSI_PATH "
	    "environment variable set.\n");
	pcDSSIPath = "/usr/local/lib/dssi:/usr/lib/dssi";
	fprintf(stderr,"assuming '%s'\n",pcDSSIPath);	
  }
  
  pcStart = pcDSSIPath;
  while (*pcStart != '\0') {
    pcEnd = pcStart;
    while (*pcEnd != ':' && *pcEnd != '\0')
      pcEnd++;
    
    pcBuffer = malloc(1 + pcEnd - pcStart);
    if (pcEnd > pcStart)
      strncpy(pcBuffer, pcStart, pcEnd - pcStart);
    pcBuffer[pcEnd - pcStart] = '\0';
    
    DSSIDirectoryPluginSearch(pcBuffer, fCallbackFunction);
    free(pcBuffer);

    pcStart = pcEnd;
    if (*pcStart == ':')
      pcStart++;
  }
}

/*****************************************************************************/

/* EOF */
