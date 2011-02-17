#pragma once

#include "decompress.h"

#define BLOCKSIZE 0x40

typedef struct _COMPRESSINFO
{
	WORD awCompressvalues[4];
	WORD awCompressBuffers[4 * BLOCKSIZE];
	LPWORD lpwTempData;

}COMPRESSINFO;

BOOL InitWavePack(WAVEUNPACK * wavepackinfo, zzub::outstream* pOutStrm);
BOOL CompressWave(WAVEUNPACK * packinfo,LPWORD lpwWaveData,
				  DWORD dwNumSamples,BOOL bStereo);

BOOL FlushPackedBuffer(WAVEUNPACK * packinfo,BOOL bFlushPartial);
