#include "common.h"
#include "decompress.h"
#include "compress.h"

//=============================BIT PACKING=========================================
BOOL InitWavePack(WAVEUNPACK * wavepackinfo, zzub::outstream* pOutStrm)
{
	wavepackinfo->dwMaxBytes = MAXPACKEDBUFFER;
	wavepackinfo->pStreamOut = pOutStrm;
	wavepackinfo->pStreamIn = 0;
	wavepackinfo->dwCurBit = 0;
	wavepackinfo->dwCurIndex = 0;
	wavepackinfo->dwBytesInBuffer = 0;
	wavepackinfo->abtPackedBuffer[0] = 0;

	return TRUE;
}

BOOL FlushPackedBuffer(WAVEUNPACK * packinfo,BOOL bFlushPartial)
{
	DWORD dwWrit;
	if (packinfo->dwBytesInBuffer > 0)
	{
		//partial data? then write it too
		if ( ((bFlushPartial) && (packinfo->dwCurBit != 0)) ||
			 ((packinfo->dwCurBit == 8))
		   )
		{
			packinfo->dwBytesInBuffer++;
			packinfo->dwCurIndex++;
			packinfo->dwCurBit = 0;
		}

		//write
		if (!packinfo->pStreamOut->write(packinfo->abtPackedBuffer, packinfo->dwBytesInBuffer))
//		if (!fwrite(packinfo->abtPackedBuffer, packinfo->dwBytesInBuffer, 1, packinfo->hFile))
//		if (!WriteFile(packinfo->hFile,packinfo->abtPackedBuffer, packinfo->dwBytesInBuffer,&dwWrit,NULL))
		{
			return FALSE;
		}

		//if we didnt flush a partial byte, then continue with it
		if(packinfo->dwCurBit != 0)
		{
			packinfo->abtPackedBuffer[0] = packinfo->abtPackedBuffer[packinfo->dwCurIndex];
		}

		packinfo->dwBytesInBuffer = 0;
		packinfo->dwCurIndex = 0;

	}

	return TRUE;
}

BOOL PackBits(WAVEUNPACK * packinfo,DWORD dwAmount,DWORD dwValue)
{
	DWORD dwWriteAmount,dwMask,dwSize,dwShift,dwVal;
	DWORD dwMax = 8;

	dwWriteAmount = dwAmount;
	dwShift=0;
	while(dwWriteAmount > 0)
	{
		if(packinfo->dwCurBit == dwMax)
		{	//next byte
			packinfo->dwBytesInBuffer++;
			packinfo->dwCurIndex++;
			packinfo->dwCurBit=0;
		
			//check to see if we need to dump current buffer to file
			if(packinfo->dwBytesInBuffer >= packinfo->dwMaxBytes)
			{	//we do..
				if(!FlushPackedBuffer(packinfo,FALSE))
				{
					return FALSE;
				}
			}

			//zero next byte
			packinfo->abtPackedBuffer[packinfo->dwCurIndex] = 0;
		}

		//calculate size to read from given dword
		dwSize = ((dwWriteAmount + packinfo->dwCurBit) > dwMax) ? dwMax - packinfo->dwCurBit : dwWriteAmount;

		//calculate bitmask
		dwMask = (1 << dwSize) - 1;

		//adjust value and apply mask
		dwVal = dwValue >> dwShift;
		dwVal &= dwMask;
		dwVal = dwVal << packinfo->dwCurBit;
		
		//merge with current byte
		packinfo->abtPackedBuffer[packinfo->dwCurIndex] |= dwVal & 0xff;

		//update info
		packinfo->dwCurBit += dwSize;
		dwShift += dwSize;
		dwWriteAmount -= dwSize;
	}

	return TRUE;
}
//=============================COMPRESSING======================================

void InitCompressionInfo(COMPRESSINFO * compinfo,DWORD dwBlockSize)
{
	memset(compinfo->awCompressvalues,0,sizeof(WORD) * 4);

	if(dwBlockSize != 0)
	{
		compinfo->lpwTempData = (LPWORD)LocalAlloc(LPTR,dwBlockSize * sizeof(WORD));
	}
	else
	{
		compinfo->lpwTempData = NULL;
	}
}

void TidyCompressionInfo(COMPRESSINFO * compinfo)
{
	//if there is temporary data - then free it.
	if (compinfo->lpwTempData != NULL)
	{
		LocalFree(compinfo->lpwTempData);
		compinfo->lpwTempData = NULL;
	}
}


//main block compression stuff.... this is exactly how buzz does it!
BOOL CompressBlock(WAVEUNPACK * packinfo,COMPRESSINFO * compinfo,DWORD dwBlockSize,
				   WORD * lpwData)
{
	DWORD j,i,k,dwIndex,dwValue,dwSize,dwMethod;
	WORD wValue;
	WORD * compressbuffers;
	WORD * wCompValues;
	DWORD adwValuetable[16 * 4] ;

	//init
	compressbuffers = compinfo->awCompressBuffers;
	wCompValues = compinfo->awCompressvalues;

	//set up value table
	dwIndex = 0;
	for(i=0;i<4;i++)
	{
		dwValue = 0;
		for(j=0;j<16;j++)
		{
			adwValuetable[dwIndex] = dwValue;
			dwIndex++;
			dwValue += dwBlockSize;
		}	
	}

	//set up values for this block
	dwIndex = 0;
	for(i=0;i<dwBlockSize;i++)
	{
		//set up temp buffers and compression values
		compressbuffers[i] = lpwData[i];
		for(j=0;j<3;j++)
		{
			wValue = compressbuffers[ (j*dwBlockSize) + i];
			wValue -= wCompValues[j];
			compressbuffers[((j+1)*dwBlockSize) + i] = wValue;
			wCompValues[j] = compressbuffers[(j*dwBlockSize) + i];
		}

		dwIndex = 0;
		for(j=0;j<4;j++)
		{
			wValue = compressbuffers[(j*dwBlockSize) + i];
			if ((signed short)wValue >= 0)
			{
				wValue += wValue;
			}
			else
			{
				wValue += wValue;
				wValue = 0xffff - wValue;
			}
			
			k=16;
			while(k!=0)
			{
				adwValuetable[dwIndex] += wValue + 1;
				dwIndex++;
				wValue = wValue >> 1;
				k--;
			}
		}
	}

	//now select which method and bit size to use
	dwIndex = 0;
	dwValue = 0x7fffffff;
	dwMethod = 0xffffffff;
	dwSize = 0xffffffff;
	for(i=0;i<4;i++)
	{
		for(j=0;j<16;j++)
		{
			if(adwValuetable[dwIndex] < dwValue)
			{	//set new bit size and compression method to use
				dwValue = adwValuetable[dwIndex];
				dwMethod = i;
				dwSize = j;
			}

			//update value table index.
			dwIndex++;
		}
	}

	//write compression method to use
	if (!PackBits(packinfo,2,dwMethod))
	{
		return FALSE;
	}

	//write size of compressed values
	if (!PackBits(packinfo,4,dwSize))
	{
		return FALSE;
	}

	//write block
	for(i=0;i<dwBlockSize;i++)
	{
		wValue = compressbuffers[(dwMethod * dwBlockSize)+i];
		if((signed short)wValue < 0)
		{
			wValue += wValue;
			wValue = 0xffff - wValue;
		}
		else
		{
			wValue += wValue;
		}

		//write value
		if(!PackBits(packinfo,dwSize,wValue))
		{
			return FALSE;
		}

		//write remainder
		wValue = wValue >> dwSize;
		if (wValue != 0)
		{	//write remainder as zeros
			if(!PackBits(packinfo,wValue,0))
			{
				return FALSE;
			}
		}

		//write a 1 to terminate the string of 0's we just wrote
		if(!PackBits(packinfo,1,1))
		{
			return FALSE;
		}
	}

	return TRUE;
}

BOOL ScanForStereoSimularity(LPWORD lpwWaveData,DWORD dwNumSamples)
{
	DWORD i,dwDifBigger,dwDifSmaller;
	WORD r;
	signed short w1,w2;

	dwDifSmaller = 0;
	dwDifBigger = 0;
	for(i=0;i<dwNumSamples;i++)
	{
		//read both channels
		w1 = lpwWaveData[i*2];
		w2 = lpwWaveData[(i*2)+1];
		
		//get difference between the two channels
		r = (w1 > w2) ? w1 - w2 : w2 - w1;

		//make r and w2 positive
		w2 = (w2 < 0) ? w2 * -1 :w2;
		
		//Is the diference larger than w2 itself?
		if(r > w2)
		{	//yes ... 
			dwDifBigger++;
		}
		else
		{	//no ...
			dwDifSmaller++;
		}
	}

	//If the majority of times where the difference between the channels is smaller
	//than the actual channel value then both channels are considered simular
	return(dwDifSmaller > dwDifBigger);
}

//Scans wave for the largest result shift value...
BYTE GetWaveShiftValue(LPWORD lpwWaveData,DWORD dwNumSamples)
{
	DWORD i;
	WORD w1;
	BYTE btSmallest,btShift;

	btSmallest = 16;
	for(i=0;i<dwNumSamples;i++)
	{
		//read value
		w1=lpwWaveData[i];

		//how much can we shift by?
		btShift = 1;
		while( (btShift >= btSmallest) && (w1 == (w1 >> btShift)) )
		{
			btShift++;
		}

		btShift--;
		if(btShift < btSmallest)
		{
			btSmallest = btShift;
		}

		if(btSmallest == 0)
		{
			return 0;
		}
	}

	return btSmallest;
}

//main wave compressor
BOOL CompressWave(WAVEUNPACK * packinfo,LPWORD lpwWaveData,
				  DWORD dwNumSamples,BOOL bStereo)
{
	DWORD dwBlockCount,dwBlockSize,dwShift,dwLastBlockSize;
	DWORD dwResultShift,ixx,i,dwCount;
	BYTE btSumChannels;
	COMPRESSINFO compinfo1,compinfo2;

	//write a single 1 bit
	if(!PackBits(packinfo,1,1))
	{
		return FALSE;
	}
	
	//write size shifter ... used by decompressor to calculate block size
	//the default used by Buzz is 6
	dwShift = 6;
	if(!PackBits(packinfo,4,dwShift))
	{
		return FALSE;
	}

	//get result shifter value
	dwResultShift = GetWaveShiftValue(lpwWaveData, bStereo ? dwNumSamples * 2 : dwNumSamples);
	if(!PackBits(packinfo,4,dwResultShift))
	{
		return FALSE;
	}

	//get size of compressed blocks
	dwBlockSize = 1 << dwShift;

	//get number of compressed blocks
	dwBlockCount = dwNumSamples >> dwShift;

	//get size of last compressed block
	dwLastBlockSize = (dwBlockSize - 1) & dwNumSamples;

	//If there's a remainder... then handle number of blocks + 1
	dwCount = (dwLastBlockSize == 0) ? dwBlockCount : dwBlockCount +1;

	if(!bStereo)
	{	//COMPRESS MONO WAVE
		
		//zero internal compression values
		InitCompressionInfo(&compinfo1,dwBlockSize);

		while(dwCount > 0)
		{
			//check to see if we are handling the last block
			if((dwCount == 1) && (dwLastBlockSize != 0))
			{	//we are... set block size to size of last block
				dwBlockSize = dwLastBlockSize;
			}

			//copy / shift data into temporary area
			for(i=0;i<dwBlockSize;i++)
			{
				compinfo1.lpwTempData[i] = lpwWaveData[i] >> dwResultShift;
			}

			//compress this block
			if(!CompressBlock(packinfo,&compinfo1,dwBlockSize,compinfo1.lpwTempData))
			{
				return FALSE;
			}

			//proceed to next block...
			lpwWaveData += dwBlockSize;
			dwCount--;

		}

		//tidy
		TidyCompressionInfo(&compinfo1);
	}
	else
	{	//COMPRESS STEREO WAVE

		//check simularity between both channels
		// tvinger vi btSumChannels til 0, så klarer vi fint å loade 32bit float samples
		// som indikerer vi har en bug på btSumChannels - eller , 
		btSumChannels = ScanForStereoSimularity(lpwWaveData,dwNumSamples) ? 1 : 0;

		//store channel sum flag
		PackBits(packinfo,1,btSumChannels);
		
		//zero internal compression values and alloc some temporary space
		InitCompressionInfo(&compinfo1,dwBlockSize);
		InitCompressionInfo(&compinfo2,dwBlockSize);

		while(dwCount > 0)
		{
			//check to see if we are handling the last block
			if((dwCount == 1) && (dwLastBlockSize != 0))
			{	//we are... set block size to size of last block
				dwBlockSize = dwLastBlockSize;
			}

			//split channels and copy / shift into temporary areas
			for(i=0;i<dwBlockSize;i++)
			{
				//do first channel
				ixx = i*2;
				compinfo1.lpwTempData[i] = lpwWaveData[ixx] >> dwResultShift;

				//if channel sum flag is set..then subtract first channel from second
				if(btSumChannels == 1)
				{
					compinfo2.lpwTempData[i] = lpwWaveData[ixx+1] - lpwWaveData[ixx];
				}
				else
				{	//otherwise, just copy second channel directly
					compinfo2.lpwTempData[i] = lpwWaveData[ixx+1];
				}

				//apply result shift to second channel
				compinfo2.lpwTempData[i] = compinfo2.lpwTempData[i] >> dwResultShift;
			}

			//compress channel 1
			if(!CompressBlock(packinfo,&compinfo1,dwBlockSize,compinfo1.lpwTempData))
			{
				return FALSE;
			}

			//compress channel 2
			if(!CompressBlock(packinfo,&compinfo2,dwBlockSize,compinfo2.lpwTempData))
			{
				return FALSE;
			}

			//proceed to next block
			lpwWaveData += dwBlockSize * 2;
			dwCount--;

		}

		//tidy
		TidyCompressionInfo(&compinfo1);
		TidyCompressionInfo(&compinfo2);
	}

	return TRUE;
}
