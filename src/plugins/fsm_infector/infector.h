#define MAX_CHANNELS		24
#define MAX_TRACKS		12

#include <zzub/plugin.h>
#include <zzub/signature.h>

namespace fsm {

class fsm_infector;

extern const zzub::parameter *paraWaveformA;
extern const zzub::parameter *paraPWMRateA;
extern const zzub::parameter *paraPWMRangeA;
extern const zzub::parameter *paraPWOffsetA;
extern const zzub::parameter *paraWaveformB;
extern const zzub::parameter *paraPWMRateB;
extern const zzub::parameter *paraPWMRangeB;
extern const zzub::parameter *paraPWOffsetB;
extern const zzub::parameter *paraTranspose;
extern const zzub::parameter *paraDetune;
extern const zzub::parameter *paraOscMix;
extern const zzub::parameter *paraSubOscWave;
extern const zzub::parameter *paraSubOscvolume;
extern const zzub::parameter *paraGlide;
extern const zzub::parameter *paraFilterType;
extern const zzub::parameter *paraFilterCutoff;
extern const zzub::parameter *paraFilterResonance;
extern const zzub::parameter *paraFilterModulation;
extern const zzub::parameter *paraFilterAttack;
extern const zzub::parameter *paraFilterDecay;
extern const zzub::parameter *paraFilterSustain;
extern const zzub::parameter *paraFilterRelease;
extern const zzub::parameter *paraFilterShape;
extern const zzub::parameter *paraFilterInertia;
extern const zzub::parameter *paraFilterTrack;
extern const zzub::parameter *paraLFORate;
extern const zzub::parameter *paraLFOAmount1;
extern const zzub::parameter *paraLFOAmount2;
extern const zzub::parameter *paraLFOShape;
extern const zzub::parameter *paraLFO2Rate;
extern const zzub::parameter *paraLFO2Amount1;
extern const zzub::parameter *paraLFO2Amount2;
extern const zzub::parameter *paraLFO2Shape;
extern const zzub::parameter *paraAmpAttack;
extern const zzub::parameter *paraAmpDecay;
extern const zzub::parameter *paraAmpSustain;
extern const zzub::parameter *paraAmpRelease;
extern const zzub::parameter *paraLFOMode;
extern const zzub::parameter *paraNote;
extern const zzub::parameter *paraVelocity;
extern const zzub::parameter *paraLength;
extern const zzub::parameter *paraCommand1;
extern const zzub::parameter *paraArgument1;
extern const zzub::parameter *paraCommand2;
extern const zzub::parameter *paraArgument2;

// 7 attributes
extern const zzub::attribute *attrMIDIChannel;
extern const zzub::attribute *attrMIDIVelocity;
extern const zzub::attribute *attrHighQuality;
extern const zzub::attribute *attrCrispness;
extern const zzub::attribute *attrTheviderness;
extern const zzub::attribute *attrGlobalTuning;
extern const zzub::attribute *attrClipTable;

#pragma pack(1)

class gvals
{
public:
  unsigned char vWaveformA;
	unsigned char vPWMRateA;
	unsigned char vPWMRangeA;
	unsigned char vPWOffsetA;
  unsigned char vWaveformB;
	unsigned char vPWMRateB;
	unsigned char vPWMRangeB;
	unsigned char vPWOffsetB;
  unsigned char vTranspose;
  unsigned char vDetune;
	unsigned char vOscMix;     // 10
	unsigned char vSubOscWave;
	unsigned char vSubOscVol;
  unsigned char vGlide;

  unsigned char vFilterType; 
  unsigned char vFilterCutoff;
  unsigned char vFilterResonance;
  unsigned char vFilterModulation;
  unsigned char vFilterAttack;

  unsigned char vFilterDecay; 
	unsigned char vFilterSustain; // 20
	unsigned char vFilterRelease;
  unsigned char vFilterShape;
  unsigned char vFilterInertia;
	unsigned char vFilterTrack;

  unsigned char vLFORate;
  unsigned char vLFOAmount1; 
  unsigned char vLFOAmount2; 
  unsigned char vLFOShape;

  unsigned char vLFO2Rate;
  unsigned char vLFO2Amount1; 
  unsigned char vLFO2Amount2; 
  unsigned char vLFO2Shape;

  unsigned char vAmpAttack;
  unsigned char vAmpDecay;
	unsigned char vAmpSustain;
	unsigned char vAmpRelease;

	unsigned char vLFOMode;
};

class tvals
{
public:
  unsigned char vNote;
  unsigned char vVelocity;
  unsigned char vLength;
	unsigned char vCommand1;
	unsigned short int vParam1;
	unsigned char vCommand2;
	unsigned short int vParam2;
};

class avals
{
public:
	int channel;
  int usevelocity;
  int hq;
  int crispness;
  int theviderness;
  int tuning;
	int cliptable;
};

#pragma pack()

class CTrack;

class CChannel
{
public:
  float Frequency;
  float PhaseOSC1;
  float PhaseOSC2;
  float PhaseOSC3;
	float Velocity;
  C6thOrderFilter Filter;
  CADSREnvelope FilterEnv;
  CADSREnvelope AmpEnv;
  int Phase1, Phase2;
  float Detune;
	
  CInertia inrKeyTrack;
  CInertia inrCutoff2;

  CTrack *pTrack;
  
  CChannel();
  void init();
	void ClearFX();
  bool IsFree() { return AmpEnv.m_nState==4; }
  void Reset();
  void NoteReset();
};
 
class CTrack
{
public:
  fsm_infector *pmi;
  int channel;
  unsigned char note,accent,length;
	unsigned char lastnote, lastaccent, lastlength;
  float DestFrequency, BaseFrequency, NotePortFrequency;
  float Detune;
	
  char Arps[3];
	int ArpPoint, ArpCount;
  int MidiNote;
	int RetrigCount, RetrigPoint, RetrigMode;
	float Vib1Phase,Vib2Phase;
	int ShuffleCounter, ShuffleMax, ShuffleAmount;
	int ShuffleData[16];
	float Vib1Rate,Vib1Depth,Vib2Rate,Vib2Depth;
  int NoTrig;
	float SlideCounter, SlideEnd, SlideRange;

  float LFOPhase, LFO2Phase;
  float CurLFO, CurLFO2;

  CInertia inrLFO1;
  CInertia inrLFO2;

  CTrack();
  unsigned char AllocChannel();
  CChannel *Chn();
  void PlayNote(unsigned char note, unsigned char accent, unsigned char length, zzub::master_info *_master_info);
	void CommandA(unsigned char cmd, unsigned short int param);
	void CommandB(unsigned char cmd, unsigned short int param);
	void ClearFX();
	int GetWakeupTime(int maxtime);
	void UseWakeupTime(int maxtime);
	void DoWakeup(fsm_infector *pmi);
  void DoLFO(fsm_infector *pmi, int c);
  void init();
  void Reset();
};
 
struct CWaveSource {
  int nSampleNo;
  int nPosition;
  int nStretch;
  int nSmoothing;
  int nClip;
  int nBend;
  int nGain;
  int nDummy1;
  int nDummy2;

  CWaveSource(): nSampleNo(0),nPosition(0),nStretch(72*16),nSmoothing(0),nClip(0),nBend(0),nGain(75),nDummy1(0),nDummy2(0) {}
};

class fsm_infector : public zzub::plugin
{
public:
	

	static struct info : zzub::info {
		info() {
			this->flags = zzub::plugin_flag_has_audio_output;
			this->min_tracks = 1;
			this->max_tracks = MAX_TRACKS;
			this->name = "FSM Infector";
			this->short_name = "Infector";
			this->author = "Krzysztof Foltman";
			this->uri = "@krzysztof_foltman/generator/infector;1";
			
			// 45 variables
			paraWaveformA = &add_global_parameter()
				.set_byte()
				.set_name("OSC1 Wave")
				.set_description("OSC1 Waveform")
				.set_value_min(0)
				.set_value_max(22)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(3
			);


			paraPWMRateA = &add_global_parameter()
				.set_byte()
				.set_name(" - PWM Rate")
				.set_description("OSC1 Pulse Width Modulation Rate")
				.set_value_min(0)
				.set_value_max(239)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(80
			);


			paraPWMRangeA = &add_global_parameter()
				.set_byte()
				.set_name(" - PWM Depth")
				.set_description("OSC1 Pulse Width Modulation Range")
				.set_value_min(0)
				.set_value_max(239)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(100
			);


			paraPWOffsetA = &add_global_parameter()
				.set_byte()
				.set_name(" - PW Offset")
				.set_description("OSC1 Pulse Width Modulation Offset")
				.set_value_min(0)
				.set_value_max(240)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(180
			);


			paraWaveformB = &add_global_parameter()
				.set_byte()
				.set_name("OSC2 Wave")
				.set_description("OSC2 Waveform")
				.set_value_min(0)
				.set_value_max(22)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(3
			);


			paraPWMRateB = &add_global_parameter()
				.set_byte()
				.set_name(" - PWM Rate")
				.set_description("OSC1 Pulse Width Modulation Rate")
				.set_value_min(0)
				.set_value_max(239)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(80
			);


			paraPWMRangeB = &add_global_parameter()
				.set_byte()
				.set_name(" - PWM Depth")
				.set_description("OSC2 Pulse Width Modulation Range")
				.set_value_min(0)
				.set_value_max(239)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(180
			);


			paraPWOffsetB = &add_global_parameter()
				.set_byte()
				.set_name(" - PW Offset")
				.set_description("OSC2 Pulse Width Modulation Offset")
				.set_value_min(0)
				.set_value_max(240)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(120
			);


			paraTranspose = &add_global_parameter()
				.set_byte()
				.set_name(" - Transpose")
				.set_description("OSC2 Transpose")
				.set_value_min(0)
				.set_value_max(72)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(36+12
			);


			paraDetune = &add_global_parameter()
				.set_byte()
				.set_name(" - Detune")
				.set_description("OSC Detune")
				.set_value_min(0)
				.set_value_max(240)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(8
			);


			paraOscMix = &add_global_parameter()
				.set_byte()
				.set_name("OSC Mix")
				.set_description("OSC Mix")
				.set_value_min(0)
				.set_value_max(240)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(60
			);


			paraSubOscWave = &add_global_parameter()
				.set_byte()
				.set_name("SubOsc Wave")
				.set_description("Sub Oscillator Wave")
				.set_value_min(0)
				.set_value_max(20)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(1
			);


			paraSubOscvolume = &add_global_parameter()
				.set_byte()
				.set_name("SubOsc Vol")
				.set_description("Sub Oscillator volume")
				.set_value_min(0)
				.set_value_max(240)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(240
			);


			paraGlide = &add_global_parameter()
				.set_byte()
				.set_name("Glide")
				.set_description("Glide")
				.set_value_min(0)
				.set_value_max(240)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(0
			);


			paraFilterType = &add_global_parameter()
				.set_byte()
				.set_name("Flt Type")
				.set_description("Filter Type")
				.set_value_min(0)
				.set_value_max(17)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(1
			);


			paraFilterCutoff = &add_global_parameter()
				.set_byte()
				.set_name(" - Cutoff")
				.set_description("Filter Cutoff")
				.set_value_min(0)
				.set_value_max(240)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(60
			);


			paraFilterResonance = &add_global_parameter()
				.set_byte()
				.set_name(" - Reso")
				.set_description("Filter Resonance")
				.set_value_min(0)
				.set_value_max(240)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(0
			);


			paraFilterModulation = &add_global_parameter()
				.set_byte()
				.set_name(" - EnvMod")
				.set_description("Filter Modulation")
				.set_value_min(0)
				.set_value_max(240)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(200
			);


			paraFilterAttack = &add_global_parameter()
				.set_byte()
				.set_name(" - Attack")
				.set_description("Filter Attack")
				.set_value_min(0)
				.set_value_max(240)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(50
			);


			paraFilterDecay = &add_global_parameter()
				.set_byte()
				.set_name(" - Decay")
				.set_description("Filter Decay")
				.set_value_min(0)
				.set_value_max(240)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(80
			);


			paraFilterSustain = &add_global_parameter()
				.set_byte()
				.set_name(" - Sustain")
				.set_description("Filter Sustain")
				.set_value_min(0)
				.set_value_max(240)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(40
			);


			paraFilterRelease = &add_global_parameter()
				.set_byte()
				.set_name(" - Release")
				.set_description("Filter Release")
				.set_value_min(0)
				.set_value_max(240)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(10
			);


			paraFilterShape = &add_global_parameter()
				.set_byte()
				.set_name(" - Mod Shp")
				.set_description("Filter Modulation Shape")
				.set_value_min(0)
				.set_value_max(240)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(100
			);


			paraFilterInertia = &add_global_parameter()
				.set_byte()
				.set_name(" - Inertia")
				.set_description("Filter Intertia")
				.set_value_min(0)
				.set_value_max(240)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(90
			);


			paraFilterTrack = &add_global_parameter()
				.set_byte()
				.set_name(" - KTrack")
				.set_description("Filter Key Tracking")
				.set_value_min(0)
				.set_value_max(240)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(180
			);


			paraLFORate = &add_global_parameter()
				.set_byte()
				.set_name("LFO1 Rate")
				.set_description("LFO1 Rate")
				.set_value_min(0)
				.set_value_max(254)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(80
			);


			paraLFOAmount1 = &add_global_parameter()
				.set_byte()
				.set_name(" - To Cutoff")
				.set_description("LFO1->Cutoff")
				.set_value_min(0)
				.set_value_max(240)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(140
			);


			paraLFOAmount2 = &add_global_parameter()
				.set_byte()
				.set_name(" - To Env")
				.set_description("LFO1->EnvMod")
				.set_value_min(0)
				.set_value_max(240)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(120
			);


			paraLFOShape = &add_global_parameter()
				.set_byte()
				.set_name(" - Shape")
				.set_description("LFO1 Shape")
				.set_value_min(0)
				.set_value_max(16)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(0
			);


			paraLFO2Rate = &add_global_parameter()
				.set_byte()
				.set_name("LFO2 Rate")
				.set_description("LFO2 Rate")
				.set_value_min(0)
				.set_value_max(254)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(80
			);


			paraLFO2Amount1 = &add_global_parameter()
				.set_byte()
				.set_name(" - To Cutoff")
				.set_description("LFO2->Cutoff")
				.set_value_min(0)
				.set_value_max(240)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(120
			);


			paraLFO2Amount2 = &add_global_parameter()
				.set_byte()
				.set_name(" - To Res")
				.set_description("LFO2->Res")
				.set_value_min(0)
				.set_value_max(240)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(120
			);


			paraLFO2Shape = &add_global_parameter()
				.set_byte()
				.set_name(" - Shape")
				.set_description("LFO2 Shape")
				.set_value_min(0)
				.set_value_max(16)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(0
			);


			paraAmpAttack = &add_global_parameter()
				.set_byte()
				.set_name("Amp Attack")
				.set_description("Amplitude Attack")
				.set_value_min(0)
				.set_value_max(240)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(40
			);


			paraAmpDecay = &add_global_parameter()
				.set_byte()
				.set_name(" - Decay")
				.set_description("Amplitude Decay")
				.set_value_min(0)
				.set_value_max(240)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(40
			);


			paraAmpSustain = &add_global_parameter()
				.set_byte()
				.set_name(" - Sustain")
				.set_description("Amplitude Sustain")
				.set_value_min(0)
				.set_value_max(240)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(180
			);


			paraAmpRelease = &add_global_parameter()
				.set_byte()
				.set_name(" - Release")
				.set_description("Amplitude Rel")
				.set_value_min(0)
				.set_value_max(240)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(20
			);


			paraLFOMode = &add_global_parameter()
				.set_byte()
				.set_name("Mode flags")
				.set_description("Mode flags")
				.set_value_min(0)
				.set_value_max(127)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(0
			);


			paraNote = &add_track_parameter()
				.set_note()
				.set_name("Note")
				.set_description("Note")
				.set_value_min(0)
				.set_value_max(240)
				.set_value_none(0)
				.set_flags(0)
				.set_value_default(0
			);


			paraVelocity = &add_track_parameter()
				.set_byte()
				.set_name("Velocity")
				.set_description("Velocity")
				.set_value_min(0)
				.set_value_max(240)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(224
			);


			paraLength = &add_track_parameter()
				.set_byte()
				.set_name("Length")
				.set_description("Length")
				.set_value_min(0)
				.set_value_max(240)
				.set_value_none(255)
				.set_flags(zzub::parameter_flag_state)
				.set_value_default(40
			);


			paraCommand1 = &add_track_parameter()
				.set_byte()
				.set_name("Command 1")
				.set_description("Command 1")
				.set_value_min(0)
				.set_value_max(255)
				.set_value_none(255)
				.set_flags(0)
				.set_value_default(0
			);


			paraArgument1 = &add_track_parameter()
				.set_word()
				.set_name("Argument 1")
				.set_description("Argument 1")
				.set_value_min(0)
				.set_value_max(65535)
				.set_value_none(0)
				.set_flags(0)
				.set_value_default(0
			);


			paraCommand2 = &add_track_parameter()
				.set_byte()
				.set_name("Command 2")
				.set_description("Command 2")
				.set_value_min(0)
				.set_value_max(255)
				.set_value_none(255)
				.set_flags(0)
				.set_value_default(0
			);


			paraArgument2 = &add_track_parameter()
				.set_word()
				.set_name("Argument 2")
				.set_description("Argument 2")
				.set_value_min(0)
				.set_value_max(65535)
				.set_value_none(0)
				.set_flags(0)
				.set_value_default(0);
				
				
			// 7 attributes
			attrMIDIChannel = &add_attribute()
				.set_name("MIDI Channel (0=off)")
				.set_value_min(0)
				.set_value_max(16)
				.set_value_default(0);


			attrMIDIVelocity = &add_attribute()
				.set_name("MIDI Use Velocity")
				.set_value_min(0)
				.set_value_max(1)
				.set_value_default(0);


			attrHighQuality = &add_attribute()
				.set_name("High quality")
				.set_value_min(0)
				.set_value_max(3)
				.set_value_default(1);


			attrCrispness = &add_attribute()
				.set_name("Crispness factor")
				.set_value_min(0)
				.set_value_max(3)
				.set_value_default(0);


			attrTheviderness = &add_attribute()
				.set_name("Theviderness factor")
				.set_value_min(0)
				.set_value_max(50)
				.set_value_default(20);


			attrGlobalTuning = &add_attribute()
				.set_name("Global tuning (cents)")
				.set_value_min(-100)
				.set_value_max(100)
				.set_value_default(0);


			attrClipTable = &add_attribute()
				.set_name("Colour")
				.set_value_min(0)
				.set_value_max(3)
				.set_value_default(0);
		}
		
		virtual zzub::plugin* create_plugin() const { return new fsm_infector(); }
		virtual bool store_info(zzub::archive *data) const { return false; }

	} fsm_infector_info;

	fsm_infector();
	virtual ~fsm_infector();
	
	virtual void destroy() { delete this; }
	
	virtual void stop();
	virtual void process_controller_events() {}
	virtual void load(zzub::archive *arc) {}
	virtual void save(zzub::archive *arc);
	virtual void attributes_changed();
	virtual void command(int index) {}
	virtual void set_track_count(int count);
	virtual void mute_track(int index) {}
	virtual bool is_track_muted(int index) const { return false; }
	virtual void midi_note(int channel, int value, int velocity);
	virtual void event(unsigned int data)  {}
	virtual const char * describe_value(int param, int value);
	virtual const zzub::envelope_info ** get_envelope_infos() { return 0; }
	virtual bool play_wave(int wave, int note, float volume) { return false; }
	virtual void stop_wave() {}
	virtual int get_wave_envelope_play_position(int env) { return -1; }

	// these have been in zzub::plugin2 before
	virtual const char* describe_param(int param) { return 0; }
	virtual bool set_instrument(const char *name) { return false; }
	virtual void get_sub_menu(int index, zzub::outstream *os) {}
	virtual void add_input(const char *name, zzub::connection_type) {}
	virtual void delete_input(const char *name, zzub::connection_type) {}
	virtual void rename_input(const char *oldname, const char *newname) {}
	virtual void input(float **samples, int size, float amp) {}
	virtual void midi_control_change(int ctrl, int channel, int value) {}
	virtual bool handle_input(int index, int amp, int pan) { return false; }
	virtual void process_midi_events(zzub::midi_message* pin, int nummessages) {}
	virtual void get_midi_output_names(zzub::outstream *pout) {}
	virtual void set_stream_source(const char* resource) {}
	virtual const char* get_stream_source() { return 0; }

	virtual void init(zzub::archive *arc);
	virtual void process_events();
	virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);
	virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) { return false; }

	short const *GetOscillatorTab(int const waveform);
  void DoPlay();

	void GenerateUserWaves(int Slot);
  void Reset();
  void ClearFX();

private:
	void InitTrack(int const i);
	void ResetTrack(int const i);

	void process_eventsTrack(CTrack *pt, tvals *ptval);
	bool WorkTrack(CTrack *pt, float *pin, float *pout, int numsamples, int const mode);


public:
  CInertia inrCutoff;
  CInertia inrResonance;
  CInertia inrModulation;
  CInertia inrModShape;
  CInertia inrLFO1Dest1;
  CInertia inrLFO1Dest2;
  CInertia inrLFO2Dest1;
  CInertia inrLFO2Dest2;

	gvals gvalAct;
	CChannel Channels[MAX_CHANNELS];
	int numTracks;
	CTrack Tracks[MAX_TRACKS+1];
  float CurCutoff, CurRes;
  avals aval;

	CBandlimitedTable usertables[8];
  float userwaves[8][2048];
  CWaveSource usersources[8];
	int PWMBuffer1[256];
	int PWMBuffer2[256];
  CPWMLFO Osc1PWM, Osc2PWM;

private:
  int nCurChannel;

	gvals gval;
	tvals tval[MAX_TRACKS];

  //zzub::metaplugin *ThisMachine;
};

#define LFOPAR2TIME(value) (0.03*pow(600.0,value/240.0))
#define GETENVTIME(value) (0.08*pow(150.0,value/240.0))
#define GETENVTIME2(value) (0.005*pow(2400.0,value/240.0))

} // namespace fsm
