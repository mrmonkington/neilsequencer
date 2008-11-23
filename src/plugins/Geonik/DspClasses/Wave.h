
//#include <windows.h>

/*	CWaveBuffer
 *
 *		Loads a signed 16bit wave from a resource and creates a buffer of
 *		floats when first requested
 */

// Converted to use libsndfile instead of some windows-specific audio
// file loading code. This assumes a mono file throughout, I think. -- jmmcd

#define dspcFastD2I DspFastD2I

#include <sndfile.h>

#ifndef dspcWBuf_ExtraSamples
#define dspcWBuf_ExtraSamples 2
#endif


struct CWaveBuffer {
	short	*pWave;						// 16-bit signed data
	float	*pBuffer;					// floats with trailing silence
	int		 iWaveLength;				// in samples
	double	 fNormFactor;				// when converting from int to float

  CWaveBuffer() {
		pWave	= NULL;
		pBuffer	= NULL; }

	~CWaveBuffer() {
          delete[] pWave;
          delete[] pBuffer; }

  bool Init(const char *filename, int const len,double const norm=1.0) {
    SNDFILE *sndfile ;
    SF_INFO sfinfo ;

    memset (&sfinfo, 0, sizeof (sfinfo)) ;
    if (! (sndfile = sf_open (filename, SFM_READ, &sfinfo))) {
      puts (sf_strerror (NULL)) ;
      return false;
    }

    if (sfinfo.channels != 1) {
      printf ("Error : channels = %d.\n", sfinfo.channels) ;
      return false;
    }

    pWave = new short[sfinfo.frames];
    if (!pWave) {
      printf("Couldn't allocate memory for audio file\n");
      return false;
    }
    int readcount = sf_read_short (sndfile, pWave, sfinfo.frames);

    if (readcount != sfinfo.frames) {
      printf("Couldn't read the audio file\n");
      return false;
    }
    iWaveLength = sfinfo.frames;
    fNormFactor = norm;
    return true; 
  }

	int GetLength() {
		return iWaveLength; }

	float *GetBuffer() {
		if(!pBuffer) {
			assert(pWave);
			pBuffer = new float [iWaveLength + dspcWBuf_ExtraSamples];
			signed short	*pin	= pWave;
			float			*pout	= pBuffer;
			double const	 a		= fNormFactor;
			int				 i		= iWaveLength;
			do { *pout++ = (float)((*pin++)*a); } while(--i);
			i = dspcWBuf_ExtraSamples;
			do { *pout++ = 0; } while(--i); 
                }
		return pBuffer; } };


/*	CWave
 *
 *		Interpolated player of a buffered wave
 */

struct CWave {
	float	*pBuffer;
	int		 iWaveLength;
	double	 fRate;
	double	 fRate_i;
	double	 fPos;
	bool	 bPlaying;

	CWave() {
		pBuffer		= NULL;
		iWaveLength = 0;
		fRate		= 1.0;
		fRate_i		= 1.0;
		bPlaying	= false; }

	~CWave() { }

	bool SetWave(CWaveBuffer * const wb) {
		pBuffer		= wb->GetBuffer();
		iWaveLength	= wb->GetLength();
		return true; }

	void SetFrequency(double const freq) {
		fRate = iWaveLength * freq / dspcSampleRate; }

	void SetRate(double const rate) {
		fRate   = rate;
		fRate_i	= 1.0 / rate; }

	void Play() {
		fPos	 = 0;
		bPlaying = true; }

	virtual double Work() {
		if(!bPlaying) return 0;
		fPos += fRate;
		if(fPos >= iWaveLength) {
			bPlaying = false;
			return 0; }
		int iPos = dspcFastD2I(fPos);
		double f = fPos - iPos;
		double t = pBuffer[iPos];
		return t + (f*(pBuffer[iPos+1] - t)); }

	virtual void WorkSamples(float *pb, int numsamples) {
//		int FpCtrlWord = _control87(0,0);
//		_control87(_RC_DOWN,_MCW_RC);
		double	fp = fPos;
		int		cnt = __min(numsamples,dspcFastD2I(((double)iWaveLength - fp)*fRate_i));
		if(cnt > 0) {
			int			 ip;
			double const fr = fRate;
			numsamples -= cnt;
			do {
						ip	= dspcFastD2I(fp);
				double	f	= fp - ip;
				double	t	= pBuffer[ip];
				*pb++ = (float)(t + (f*(pBuffer[ip+1] - t)));
				fp += fr; } while(--cnt);
			fPos = fp; }
		else {
			bPlaying = false; }
		if(numsamples > 0)
			do { *pb++ = 0; } while(--numsamples);
//		_control87(FpCtrlWord,_MCW_RC);

/*	virtual void WorkSamples(float *pb, int numsamples) {
//		int FpCtrlWord = _control87(0,0);
//		_control87(_RC_DOWN,_MCW_RC);
		double	fp = fPos;
		int		ip = dspcFastD2I(((double)iWaveLength - fp)*fRate_i);
		double	fr = fRate;
		if(ip <= numsamples) {
			int cnt = numsamples - ip;
			if(cnt) {
				numsamples -= cnt;
				do {
							ip	= dspcFastD2I(fp);
					double	f	= fp - ip;
					double	t	= pBuffer[ip++];
					*pb++ = (float)(t + (f*(pBuffer[ip] - t)));
					fp += fr;
				} while(--cnt); }
			bPlaying = false;
			if(numsamples > 0)
				do { *pb++ = 0; } while(--numsamples); }
		else {
			do {
						ip	= dspcFastD2I(fp);
				double	f	= fp - ip;
				double	t	= pBuffer[ip++];
				*pb++ = (float)(t + (f*(pBuffer[ip] - t)));
				fp += fr;
			} while(--numsamples);
			fPos = fp; }
//		_control87(FpCtrlWord,_MCW_RC);*/
	} };


/*	CLoopWave
 *
 *		Interpolated player of a buffered looped wave
 */

struct CLoopWave : public CWave {
	double Work() {
		if(!bPlaying) return 0;
		fPos += fRate;
		while(fPos >= iWaveLength) fPos -= iWaveLength;
		int iPos = dspcFastD2I(fPos);
		double f = fPos - iPos;
		double t = pBuffer[iPos];
		return t + (f*(pBuffer[iPos+1] - t)); }

	void WorkSamples(float *pb, int numsamples) {
                //int FpCtrlWord = _control87(0,0);
		//_control87(_RC_DOWN,_MCW_RC);
		int		ip;
		double	fp = fPos;
		double	fr = fRate;
		int		cnt = __min(numsamples,dspcFastD2I(((double)iWaveLength - fp)/fRate));
		if(cnt > 0) {
			numsamples -= cnt;
			do {
						ip	= dspcFastD2I(fp);
				double	f	= fp - ip;
				double	t	= pBuffer[ip++];
				*pb++ = (float)(t + (f*(pBuffer[ip] - t)));
				fp += fr; } while(--cnt);
			fPos = fp; }
		else {
			bPlaying = false; }
		if(numsamples > 0)
			do { *pb++ = 0; } while(--numsamples);
		//_control87(FpCtrlWord,_MCW_RC); 
        } 
};
