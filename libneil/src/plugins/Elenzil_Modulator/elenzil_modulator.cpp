/*

  The Elenzil Amplitude Modulator	1.0

  20010303 Santa Cruz California

  This machine was written primarily because Goenik's Amplitude Modulator
  introduces small but noticable artifacts in the sound.  For instance,
  try tunning the M4 in the included demo song thru a single Goenik's
  and compare to a single Elenzil. You may need to listen with headphones.

  Since i used the AM far more than any other single effect,
  i finally got tired of fixing the artifacts up in cooledit,
  and went to write my own.

  My guess at the cause of goenik's artifacts is that he calculated
  the stereo spread only once per Work() routine, rather than for each Sample.
  My version does it for each sample. This is probably overkill, but it works for me.
  

  !!!
  This machine is Crap for Efficiency.
  I don't really care, as i usually have fast computers.
  --> I am using actual Sin() calls, not lookup tables!
  There is no justification for this except my own laziness.

  Also, i built this off of an example from the standard buzz dist,
  and havn't cleaned anything up.

  If you fix it up, god bless you.

  Okay,
  that's about it.
  Compiled w/ VC6.

  Orion Elenzil
  www.elenzil.com


*/


#include <zzub/signature.h>
#include <zzub/plugin.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <time.h>

typedef unsigned char byte;
typedef unsigned short word;
double const PI = 3.14159265358979323846;

class modulator;
class CTrack
{
public:
  double	Speed;
  int		Unit;
  int		Wave;
  int		WavePow;
  int		Floor;
  double	Spread;
  double	Slur;
  double	Gain;
  bool	Reset;

  double	GetFrequency();
};



modulator *pz;





double const SilentEnough = log(1.0 / 32768);

#define MAX_TAPS		1

#define UNIT_MHZ	0
#define UNIT_MS		1
#define UNIT_TICK	2
#define UNIT_256	3

#define	WAVE_SIN		0
#define	WAVE_SQUARE		1
#define	WAVE_TRIANGLE	2
#define	WAVE_SAW		3
#define	WAVE_INVSAW		4
#define	WAVE_CRAZY		5

const zzub::parameter *paraSpeed = 0;
const zzub::parameter *paraUnit = 0;
const zzub::parameter *paraWave = 0;
const zzub::parameter *paraWavePow = 0;
const zzub::parameter *paraFloor = 0;
const zzub::parameter *paraSpread = 0;
const zzub::parameter *paraSlur = 0;
const zzub::parameter *paraGain = 0;
const zzub::parameter *paraReset = 0;

#pragma pack(1)

class tvals
{
public:
  word	speed;
  byte	unit;
  byte	wave;
  byte	wavepow;
  byte	floor;
  word	spread;
  byte	slur;
  byte	gain;
  byte	reset;
};


#pragma pack()


class modulator: public zzub::plugin
{
public:
  modulator();
  virtual ~modulator();
  virtual void process_events();
  virtual void init(zzub::archive *);
  virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);
  virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) { return false; }
  virtual void command(int i) {}
  virtual void load(zzub::archive *arc) {}
  virtual void save(zzub::archive *) { }
  virtual const char * describe_value(int param, int value);
  virtual void OutputModeChanged(bool stereo) { }
  
  // ::zzub::plugin methods
  virtual void process_controller_events() {}
  virtual void destroy();
  virtual void stop();
  virtual void attributes_changed();
  virtual void set_track_count(int);
  virtual void mute_track(int) {}
  virtual bool is_track_muted(int) const { return false; }
  virtual void midi_note(int, int, int) {}
  virtual void event(unsigned int) {}
  virtual const zzub::envelope_info** get_envelope_infos() { return 0; }
  virtual bool play_wave(int, int, float) { return false; }
  virtual void stop_wave() {}
  virtual int get_wave_envelope_play_position(int) { return -1; }
  virtual const char* describe_param(int) { return 0; }
  virtual bool set_instrument(const char*) { return false; }
  virtual void get_sub_menu(int, zzub::outstream*) {}
  virtual void add_input(const char*) {}
  virtual void delete_input(const char*) {}
  virtual void rename_input(const char*, const char*) {}
  virtual void input(float**, int, float) {}
  virtual void midi_control_change(int, int, int) {}
  virtual bool handle_input(int, int, int) { return false; }
  
private:

  void	InitTrack(int const i);
  void	ResetTrack(int const i);

  void	TickTrack(CTrack *pt, tvals *ptval);
  int		MSToSamples(double const ms);

private:
  int		IdleCount;	// in samples
  bool	IdleMode;
  int		pos;
  int		pos2;

  long	m_Tick;
  double	m_Second;
  double	m_PrevF1;
  double	m_PrevF2;

private:
  int numTracks;
  CTrack Tracks[MAX_TAPS];

public:
  tvals tval[MAX_TAPS];

};

// Return frequency in seconds.
inline	double	CTrack::GetFrequency()
{
  switch (Unit)
    {
    default:
    case	UNIT_MHZ:
      return Speed * 0.001;
      break;
    case	UNIT_MS:
      if (Speed != 0.0)
	return 1000.0 / Speed;
      else
	return 1000.0 / 0.5;
      break;
    case	UNIT_TICK:
      if (Speed != 0.0)
	return pz->_master_info -> ticks_per_second / Speed;
      else
	return pz->_master_info -> ticks_per_second;
      break;
    case	UNIT_256:
      if (Speed != 0.0)
	return 256.0 * pz->_master_info -> ticks_per_second / Speed;
      else
	return 256.0 * pz->_master_info -> ticks_per_second;
      break;
    }
}


char*	CommaPrint(char* txt, int const value)
{
  char	tmp[32];
  int		i, inum;
  int		j, k;
  sprintf(tmp, "%d", value);
  inum	=	strlen(tmp);
  j		=	inum	% 3;
  k		=	0;
  for (i = 0; i < inum; i++)
    {
      txt[k++] = tmp[i];
      j--;
      if (j == 0 && i < inum - 1)
	{
	  txt[k++] = ',';
	  j = 3;
	}
    }
  txt[k] = 0;

  return txt;
}





int modulator::MSToSamples(double const ms)
{
  return (int)(pz->_master_info->samples_per_second * ms * (1.0 / 1000.0));
}


void modulator::TickTrack(CTrack *pt, tvals *ptval)
{
  if (ptval->speed	!=	paraSpeed	->value_none)
    pt->Speed		=	(double)ptval -> speed;

  if (ptval->unit		!=	paraUnit	->value_none)
    pt->Unit		=			ptval -> unit;

  if (ptval->wave		!=	paraWave	->value_none)
    pt->Wave		=			ptval -> wave;

  if (ptval->wavepow	!=	paraWavePow	->value_none)
    pt->WavePow		=			ptval -> wavepow;

  if (ptval->floor	!=	paraFloor	->value_none)
    pt->Floor		=			ptval -> floor;

  if (ptval->spread	!=	paraSpread	->value_none)
    pt->Spread		=	(double)ptval -> spread	/ (double)paraSpread->value_max * 2.0 - 1.0;

  if (ptval->slur		!=	paraSlur	->value_none)
    pt->Slur		=	(double)ptval -> slur	/ (double)paraSlur->value_max;

  if (ptval->gain		!=	paraGain	->value_none)
    pt->Gain		=	(double)ptval -> gain	/ (double)paraGain->value_default;

  if (ptval->reset	!=	paraReset	->value_none)
    pt->Reset		=	(bool)	ptval -> reset;
}




modulator::modulator() {
  pz = this;
  //global_values = &gval;
  track_values = tval;
}

modulator::~modulator() {
}

void modulator::process_events() {
  m_Tick++;

  for (int c = 0; c < numTracks; c++)
    TickTrack(Tracks + c, tval+c);

}

void modulator::init(zzub::archive *arc) {
  numTracks = 1;

  for (int c = 0; c < MAX_TAPS; c++)
    {
      Tracks[c].Speed		= 1000.0;
      Tracks[c].Unit		= UNIT_MHZ;
      Tracks[c].Wave		= WAVE_SIN;
      Tracks[c].WavePow	= 1;
      Tracks[c].Floor		= 0;
      Tracks[c].Spread	= 1.0;
      Tracks[c].Slur		= 0.94;
      Tracks[c].Gain		= 1.0;
      Tracks[c].Reset		= false;
    }

  IdleMode	=	true;
  IdleCount	=	0;
  pos			=	0;
  pos2		=	0;
	
  m_Tick		=	0;
  m_Second	=	0.0;

  m_PrevF1	=	1.0;
  m_PrevF2	=	1.0;


}
bool modulator::process_stereo(float **pin, float **pout, int numsamples, int mode) {

  if (mode == zzub::process_mode_write || mode == zzub::process_mode_no_io)
    return false;
  if (mode == zzub::process_mode_read) // _thru_
    return true;

  int	i, inum;
  int	j;
  int	p, pnum;

  inum	=	numsamples;
  j		=	0;

  int		op;
  op		=	pos;
  pos		+=	pz->_master_info ->	tick_position - pos2;
  pos2	=	pz->_master_info ->	tick_position;

  double	f1;
  double	f2;
  double	SecondsPerSample;
  double	floor;
  double	one_minus_floor;
  double	Spread;
  double	Slur, OneMinusSlur;
  double	Gain;

  floor				=	(double)(Tracks[0].Floor) / (double)paraFloor->value_max;
  one_minus_floor		=	1.0 - floor;

  SecondsPerSample	=	1.0 / (double)pz->_master_info -> samples_per_second;
  SecondsPerSample	*=	Tracks[0].GetFrequency();

  Spread				=	Tracks[0].Spread;

  pnum				=	Tracks[0].WavePow;

  Slur				=	Tracks[0].Slur;
  OneMinusSlur		=	1.0 - Slur;

  Gain				=	Tracks[0].Gain;

  if (Tracks[0].Reset)
    {
      m_Second		=	0;
      Tracks[0].Reset	=	false;
    }


  switch (Tracks[0].Wave)
    {
    default:
    case	WAVE_SIN:
      Spread			*=	PI * 0.5;
      for (i = 0; i < inum; i++)
	{
	  f1			=	sin(m_Second * PI * 2.0 - Spread) * 0.5 + 0.5;
	  f2			=	sin(m_Second * PI * 2.0 + Spread) * 0.5 + 0.5;
	  m_Second	+=	SecondsPerSample;
	  // This prevents the math from overflowing.
	  if (m_Second > 1.0)
	    m_Second -= 1.0;

	  for (p = 1; p < pnum; p++)
	    {
	      f1		*=	f1;
	      f2		*=	f2;
	    }

	  f1			=	f1 * one_minus_floor + floor;
	  f2			=	f2 * one_minus_floor + floor;

	  f1			*=	Gain;
	  f2			*=	Gain;

	  f1			=	OneMinusSlur * f1 + Slur * m_PrevF1;
	  f2			=	OneMinusSlur * f2 + Slur * m_PrevF2;
	  m_PrevF1	=	f1;
	  m_PrevF2	=	f2;

	  pout[0][j]		=	pin[0][i] * (float)f1;
	  pout[1][j]		=	pin[1][i] * (float)f2;
	  j++;
	}
      break;

    case	WAVE_SQUARE:
      Spread			*=	0.25;
      for (i = 0; i < inum; i++)
	{
	  f1			=	m_Second - Spread + 0.25;
	  if (f1 < 0.0)
	    f1 += 1.0;
	  else if (f1 > 1.0)
	    f1 -= 1.0;
	  f2			=	m_Second + Spread + 0.25;
	  if (f2 < 0.0)
	    f2 += 1.0;
	  else if (f2 > 1.0)
	    f2 -= 1.0;
	  if (f1 < 0.5)
	    f1	=	0.0;
	  else
	    f1	=	1.0;
	  if (f2 < 0.5)
	    f2	=	0.0;
	  else
	    f2	=	1.0;

	  m_Second	+=	SecondsPerSample;
	  // This prevents the math from overflowing.
	  if (m_Second > 1.0)
	    m_Second -= 1.0;

	  f1			=	f1 * one_minus_floor + floor;
	  f2			=	f2 * one_minus_floor + floor;

	  f1			*=	Gain;
	  f2			*=	Gain;

	  f1			=	OneMinusSlur * f1 + Slur * m_PrevF1;
	  f2			=	OneMinusSlur * f2 + Slur * m_PrevF2;
	  m_PrevF1	=	f1;
	  m_PrevF2	=	f2;

	  pout[0][j]		=	pin[0][i] * (float)f1;
	  pout[1][j]		=	pin[1][i] * (float)f2;
	  j++;
	}
      break;

    case	WAVE_TRIANGLE:
      Spread			*=	0.25;
      for (i = 0; i < inum; i++)
	{
	  f1			=	m_Second - Spread + 0.25;
	  if (f1 < 0.0)
	    f1 += 1.0;
	  else if (f1 > 1.0)
	    f1 -= 1.0;
	  f2			=	m_Second + Spread + 0.25;
	  if (f2 < 0.0)
	    f2 += 1.0;
	  else if (f2 > 1.0)
	    f2 -= 1.0;

	  f1	*=	2.0;
	  f2	*=	2.0;

	  if (f1 > 1.0)
	    f1	=	2.0 - f1;
	  if (f2 > 1.0)
	    f2	=	2.0 - f2;

	  m_Second	+=	SecondsPerSample;
	  // This prevents the math from overflowing.
	  if (m_Second > 1.0)
	    m_Second -= 1.0;

	  for (p = 1; p < pnum; p++)
	    {
	      f1		*=	f1;
	      f2		*=	f2;
	    }

	  f1			=	f1 * one_minus_floor + floor;
	  f2			=	f2 * one_minus_floor + floor;

	  f1			*=	Gain;
	  f2			*=	Gain;

	  f1			=	OneMinusSlur * f1 + Slur * m_PrevF1;
	  f2			=	OneMinusSlur * f2 + Slur * m_PrevF2;
	  m_PrevF1	=	f1;
	  m_PrevF2	=	f2;

	  pout[0][j]		=	pin[0][i] * (float)f1;
	  pout[1][j]		=	pin[1][i] * (float)f2;
	  j++;
	}
      break;

    case	WAVE_SAW:
      Spread			*=	0.25;
      for (i = 0; i < inum; i++)
	{
	  f1			=	m_Second - Spread + 0.25;
	  if (f1 < 0.0)
	    f1 += 1.0;
	  else if (f1 > 1.0)
	    f1 -= 1.0;
	  f2			=	m_Second + Spread + 0.25;
	  if (f2 < 0.0)
	    f2 += 1.0;
	  else if (f2 > 1.0)
	    f2 -= 1.0;

	  m_Second	+=	SecondsPerSample;
	  // This prevents the math from overflowing.
	  if (m_Second > 1.0)
	    m_Second -= 1.0;

	  for (p = 1; p < pnum; p++)
	    {
	      f1		*=	f1;
	      f2		*=	f2;
	    }

	  f1			=	f1 * one_minus_floor + floor;
	  f2			=	f2 * one_minus_floor + floor;

	  f1			*=	Gain;
	  f2			*=	Gain;

	  f1			=	OneMinusSlur * f1 + Slur * m_PrevF1;
	  f2			=	OneMinusSlur * f2 + Slur * m_PrevF2;
	  m_PrevF1	=	f1;
	  m_PrevF2	=	f2;

	  pout[0][j]		=	pin[0][i] * (float)f1;
	  pout[1][j]		=	pin[1][i] * (float)f2;
	  j++;
	}
      break;
    case	WAVE_INVSAW:
      Spread			*=	0.25;
      for (i = 0; i < inum; i++)
	{
	  f1			=	m_Second - Spread + 0.25;
	  if (f1 < 0.0)
	    f1 += 1.0;
	  else if (f1 > 1.0)
	    f1 -= 1.0;
	  f2			=	m_Second + Spread + 0.25;
	  if (f2 < 0.0)
	    f2 += 1.0;
	  else if (f2 > 1.0)
	    f2 -= 1.0;

	  f1	=	1.0 - f1;
	  f2	=	1.0 - f2;

	  m_Second	+=	SecondsPerSample;
	  // This prevents the math from overflowing.
	  if (m_Second > 1.0)
	    m_Second -= 1.0;

	  for (p = 1; p < pnum; p++)
	    {
	      f1		*=	f1;
	      f2		*=	f2;
	    }

	  f1			=	f1 * one_minus_floor + floor;
	  f2			=	f2 * one_minus_floor + floor;

	  f1			*=	Gain;
	  f2			*=	Gain;

	  f1			=	OneMinusSlur * f1 + Slur * m_PrevF1;
	  f2			=	OneMinusSlur * f2 + Slur * m_PrevF2;
	  m_PrevF1	=	f1;
	  m_PrevF2	=	f2;

	  pout[0][j]		=	pin[0][i] * (float)f1;
	  pout[1][j]		=	pin[1][i] * (float)f2;
	  j++;
	}
      break;
    }



  return true;
}

const char * modulator::describe_value(int param, int value) {
  static char txt[25] = "";
  char	tmp[9];
  int		val;

  val	=	value;
  switch(param)
    {
    case 0:		// freq
      switch(Tracks[0].Unit)
	{
	default:
	case	UNIT_MS:
	case	UNIT_MHZ:
	  CommaPrint(txt, val);
	  sprintf(tmp, " (%.4x)", val);
	  strcat(txt, tmp);
	  break;
	}
      break;
    case 1:		// unit
      switch(val)
	{
	case 0: return "mHz (00)";
	case 1: return "ms (01)";
	case 2: return "tick (02)";
	case 3: return "tick/256 (03)";
	}
      break;
    case 2:		// wave
      const char*	s;
      switch(val)
	{
	case WAVE_SIN		: s = "sin"			; break;
	case WAVE_SQUARE	: s = "square"		; break;
	case WAVE_TRIANGLE	: s = "triangle"	; break;
	case WAVE_SAW		: s = "saw"			; break;
	case WAVE_INVSAW	: s = "inv. saw"	; break;
	case WAVE_CRAZY		: s = "crazy"		; break;
	default:
	  s = "???";
	  break;
	}
      sprintf(txt, "%s (%.2x)", s, val);
      break;
    case 3:		// wave power
      sprintf(txt, "^%d", val);
      break;
    case 4:		// floor
      sprintf(txt, "%d%c (%.2x)", val * 100 / paraFloor->value_max, '%', val);
      break;
    case 5:		// spread
      sprintf(txt, "%.3f (%.4x)", (double)val / (double)paraSpread->value_max * 2.0 - 1.0, val);
      break;
    case 6:		// slur
      sprintf(txt, "%.3f (%.2x)", (double)val / (double)paraSlur->value_max, val);
      break;
    case 7:		// gain
      sprintf(txt, "%.2f (%.2x)", (double)val / (double)paraGain->value_default, val);
      break;
    case 8:		// reset
      sprintf(txt, "1 = reset oscillation");
      break;
    default:
      return NULL;
    }

  return txt;

}


void modulator::set_track_count(int n) {
  if (numTracks < n)
    {
      for (int c = numTracks; c < n; c++)
	InitTrack(c);
    }
  else if (n < numTracks)
    {
      for (int c = n; c < numTracks; c++)
	ResetTrack(c);
	
    }
  numTracks = n;
}

void modulator::InitTrack(int const i)
{
}

void modulator::ResetTrack(int const i)
{
}


void modulator::stop() {
}

void modulator::destroy() { 
  delete this; 
}

void modulator::attributes_changed() {
  for (int c = 0; c < numTracks; c++)
    InitTrack(c);

}


const char *zzub_get_signature() { return ZZUB_SIGNATURE; }


struct modulator_plugin_info : zzub::info {
  modulator_plugin_info() {
    this->flags = zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
    this->min_tracks = 1;
    this->max_tracks = 1;
    this->name = "Elenzil Modulator";
    this->short_name = "Modulator";
    this->author = "Elenzil";
    this->uri = "jamesmichaelmcdermott@gmail.com/effect/modulator;1";


    paraSpeed = &add_track_parameter()
      .set_word()
      .set_name("Speed")
      .set_description("Oscillation Speed")
      .set_value_min(0)
      .set_value_max(65534)
      .set_value_none(65535)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(1000);
    paraUnit = &add_track_parameter()
      .set_byte()
      .set_name("Speed Unit")
      .set_description("Speed unit (0 = mHz, 1 = ms, 2 = tick, 3 = 256th of tick)")
      .set_value_min(0)
      .set_value_max(3)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(UNIT_MHZ);
    paraWave = &add_track_parameter()
      .set_byte()
      .set_name("Wave Type")
      .set_description("Wave Type (0 = sin, 1 = square, 2 = triangle 3 = saw, 4 = inv. saw)")
      .set_value_min(0)
      .set_value_max(4)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(WAVE_SIN);
    paraWavePow = &add_track_parameter()
      .set_byte()
      .set_name("Wave Power")
      .set_description("Wave Power (1 = 1, 2 = ^2, 3 = ^3 etc)")
      .set_value_min(0)
      .set_value_max(13)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(1);
    paraFloor = &add_track_parameter()
      .set_byte()
      .set_name("Floor")
      .set_description("Floor 00 - fe")
      .set_value_min(0)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraSpread = &add_track_parameter()
      .set_word()
      .set_name("Phase")
      .set_description("Phase x0000 - x0800")
      .set_value_min(0x0000)
      .set_value_max(0x0800)
      .set_value_none(0xffff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x0400); // this is a better default, IMHO -- jmmcd
    paraSlur = &add_track_parameter()
      .set_byte()
      .set_name("Slur")
      .set_description("Slur x00 - xfe")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0xf2);
    paraGain = &add_track_parameter()
      .set_byte()
      .set_name("Gain")
      .set_description("Gain x00 - xfe")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x12);
    paraReset = &add_track_parameter()
      .set_switch()
      .set_name("Reset")
      .set_description("Reset 1 = Restart Oscillator")
      .set_value_min(0x01)
      .set_value_max(0x01)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_event_on_edit)
      .set_value_default(0xff);

  } 
  virtual zzub::plugin* create_plugin() const { return new modulator(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} modulator_info;

struct modulatorplugincollection : zzub::plugincollection {
  virtual void initialize(zzub::pluginfactory *factory) {
    factory->register_info(&modulator_info);
  }
  
  virtual const zzub::info *get_info(const char *uri, zzub::archive *data) { return 0; }
  virtual void destroy() { delete this; }
  // Returns the uri of the collection to be identified,
  // return zero for no uri. Collections without uri can not be 
  // configured.
  virtual const char *get_uri() { return 0; }
  
  // Called by the host to set specific configuration options,
  // usually related to paths.
  virtual void configure(const char *key, const char *value) {}
};

zzub::plugincollection *zzub_get_plugincollection() {
  return new modulatorplugincollection();
}
