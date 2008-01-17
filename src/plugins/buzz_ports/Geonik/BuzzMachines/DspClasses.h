/*
 *		Dsp Classes
 *
 *			Written by George Nicolaidis aka Geonik
 */

//#include <windows.h>


/*	General purpose functions
 *
 */

extern int dspcSampleRate;

inline int dspcFastD2I(double n) {
		double const d2i = (1.5 * (1 << 26) * (1 << 26));
		double n_ = n + d2i;
		return *(int *)&n_; }

inline double dspcFastRand() {
	static long stat = 0x16BA2118;
	stat = stat * 1103515245 + 12345;
	return (double)stat * (1.0 / 0x80000000); }

inline double dspcFastRand(double a) {
	return a * dspcFastRand(); }

inline double dpscCalcLogStep(double const from, double const to, double const invtime) {
	return pow(to / from, invtime); }

inline double dpscCalcLinStep(double const from, double const to, double const invtime) {
	return (to-from) * invtime; }


/*	CNoise
 *
 *		Fast white noise generator
 */

struct CNoise {
	double	fLastOut;

	CNoise() {
		fLastOut = 0.0; }

	void Clear() {
		fLastOut = 0.0; }

	double GetWhiteSample() {
		static long stat = 0x16BA2118;
		stat = stat * 1103515245 + 12345;
		return (double)stat * (1.0 / 0x80000000); }

	double GetBlackSample(double a) {
		return (fLastOut = fLastOut * a + GetWhiteSample() * (1.0-a)); } };


/*	CBufWave
 *
 *		16bit wavetable resource loader and interpolated player

class CBufWave {
	double	 fSampleRate_inv;
public:
	float	*pWave;
	int		 iWaveLength;
	double	 fRate;
	double	 fPos;

	CBufWave() {
		pWave = NULL; }

	~CBufWave() {
		delete[] pWave; }

	virtual bool Load(void *hModule, int const iResId,int const iLength) {
		HRSRC	 hResInfo;
		void	*hResource;
		short	*pRawWave;

		if(!(hResInfo = FindResource(NULL,(char *)iResId,"RawWave"))) return false;
		if(!(hResource = LoadResource(NULL,hResInfo))) return false;
		if(!(pRawWave = (short *)LockResource(hResource))) return false;
		iWaveLength = iLength;
		pWave = new float [iWaveLength+1];
		int i=iWaveLength; signed short *pin = pRawWave; float *pout = pWave; do {
			*pout++ = (float)*pin++;
			} while(--i);
		pWave[iWaveLength] = pWave[0];
		fPos	= 0;
		fRate	= 1.0;
		return true; }

	virtual void Normalize() {
		Normalize(1.0); }

	virtual void Normalize(double const fPeak) {
		double m = 0;
		for(int i=0; i <= iWaveLength; i++) m = __max(fabs(pWave[i]),m);
		if(m) m = fPeak/m;
		for(i=0; i <= iWaveLength; i++) pWave[i] *= (float)m; }

	virtual void SetSampleRate(double const fSps) {
		fSampleRate_inv = 1.0 / fSps; }

	virtual void SetFrequency(double const fFreq) {
		fRate = iWaveLength * fFreq * fSampleRate_inv; }

	virtual double Work() {
		fPos += fRate;
		while(fPos >= iWaveLength) fPos -= iWaveLength;
		int iPos = FastD2I(fPos);
		double f = fPos - iPos;
		double t = pWave[iPos];
		return t + (f*(pWave[iPos+1] - t)); }

	int FastD2I(double const n) {
		double const d2i = (1.5 * (1 << 26) * (1 << 26));
		double n_ = n + d2i;
		return *(int *)&n_; } };
 */


