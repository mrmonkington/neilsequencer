/******************************************************************************
DT Block FX - Buzz machine


MI_BlockFx
	Buzz machine interface

History
	Date       Version    Programmer         Comments
	16/2/03    1.0        Darrell Tam		Created
******************************************************************************/

#ifndef _DT_BLOCKFX_H_
#define _DT_BLOCKFX_H_

#include <rfftw.h>
#include <MachineInterface.h>
#include "cplx.h"
#include "params.h"
#include "misc_stuff.h"

#ifdef _DEBUG
extern ofstream debug;
#define DBG(a) a
#else
#define DBG(a) 
#endif

using namespace std;

class BlkFx;

//------------------------------------------------------------------------------------------
class MI_BlockFx : public CMachineInterface
{
public:
	MI_BlockFx();
	virtual ~MI_BlockFx();
	virtual void Init(CMachineDataInput * const pi);
	virtual void Tick();
	virtual char const *DescribeValue(int const param, int const value);
	virtual bool Work(float *, int , int const );
	virtual void SetNumTracks(int const n);
	virtual void Command(int const i);

	void processFFT(void);
	void mixToOutputBuffer(void);
	bool paramsRdy(void);
	bool blkRdy(void);
	void nextBlk(void);

	// phase randomizing state
	unsigned long rand_i;

	// plans for performing FFT/IFFT
	vector<rfftw_plan> plan_fft, plan_ifft;
	
	vector<float> x0;	// pre FFT buffer
	vector<float> x3;	// output FIFO

	long x0i_abs;		// absolute sample position of x0_i
	long x0_i;			// pre-fft buffer offset
	long x0_n;			// number of samples in the pre-fft buffer
	long x0_lastreal_abs;//final pre-fft position of data passed to Work()

	long x3o_abs;		// absolute sample position of x3_o/current sample
	long x3_o;			// output FIFO output index
	long x3_lastreal_abs;//final output position of data passed to Work()

	long prev_blkend_abs;// absolute end position of previous block written
	long out_delay_n;	// output delay of current block

	bool buffering_block;// false if waiting for next param

	long curr_blk_abs;	// output position for current block

	int prev_work_mode;	// mode passed to "Work()"

	// temporary variables used during Work()
	//float x1_absminnorm, x1_absmaxnorm;	// absolute limits of x1 (post-FFT)
	
	int plan_n;			// plan number to use (corresponds to blk_len_fft)
	long blk_len_fft;	// FFT block length
	float mix_back;		// current mixback fraction
	long fade_samps;	// current number of cross-fade samples

	long blk_len;		// actual length of block to process (same as blk_len_fft unless not enough data)
	long buf_n;			// current input buffer length
	float x2_scale;		// scaling to apply to x2 during mix back
	float total_out_pwr;// current output power
	
	vector<float> x1;	// FFT'd data (frequency-domain)
	vector<float> x2;	// IFFT'd data (time-domain)

	// temporary variables for the current blk effect
	long curr_start_bin, curr_stop_bin; // start & stop bin for this effect
	float curr_amp;		// current amplitude scaling
	float curr_fxval;	// current effect value

	// table of block effects
	vector<auto_ptr<BlkFx> > blk_fx;
	BlkFx& blkFx(int i) { return *blk_fx[limit_range(i, 0, blk_fx.size()-1)]; }

	// params vector
	ParamsVec params_vec;

	// samples = BLK_SZ_0 << getBlkLenDisp()
	long getBlkLenDisp(void);

	void reset(void);

	// Buzz fills in this structure
	Params params;

	// params carried over for display purposes
	Params params_disp;

	CMachine *this_machine;
};

#endif
