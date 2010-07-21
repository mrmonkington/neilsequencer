#ifndef SOMONO_M3_HPP
#define SOMONO_M3_HPP

#include <stdint.h>

#include <zzub/zzub.h>
#include <zzub/signature.h>
#include <zzub/plugin.h>

struct Tvals {
  uint8_t note;
} __attribute__((__packed__));

const zzub::parameter *paraWave1 = 0;
const zzub::parameter *paraPulseWidth1 = 0;
const zzub::parameter *paraWave2 = 0;
const zzub::parameter *paraPulseWidth2 = 0;
const zzub::parameter *paraDetuneSemi = 0;
const zzub::parameter *paraDetuneFine = 0;
const zzub::parameter *paraSync = 0;
const zzub::parameter *paraMixType = 0;
const zzub::parameter *paraMix = 0;
const zzub::parameter *paraSubOscWave = 0;
const zzub::parameter *paraSubOscVol = 0;
const zzub::parameter *paraPEGAttackTime = 0;
const zzub::parameter *paraPEGDecayTime = 0;
const zzub::parameter *paraPEnvMod = 0;
const zzub::parameter *paraGlide = 0;
const zzub::parameter *paraVolume = 0;
const zzub::parameter *paraAEGAttackTime = 0;
const zzub::parameter *paraAEGSustainTime = 0;
const zzub::parameter *paraAEGReleaseTime = 0;
const zzub::parameter *paraFilterType = 0;
const zzub::parameter *paraCutoff = 0;
const zzub::parameter *paraResonance = 0;
const zzub::parameter *paraFEGAttackTime = 0;
const zzub::parameter *paraFEGSustainTime = 0;
const zzub::parameter *paraFEGReleaseTime = 0;
const zzub::parameter *paraFEnvMod = 0;
const zzub::parameter *paraLFO1Dest = 0;
const zzub::parameter *paraLFO1Wave = 0;
const zzub::parameter *paraLFO1Freq = 0;
const zzub::parameter *paraLFO1Amount = 0;
const zzub::parameter *paraLFO2Dest = 0;
const zzub::parameter *paraLFO2Wave = 0;
const zzub::parameter *paraLFO2Freq = 0;
const zzub::parameter *paraLFO2Amount = 0;

const zzub::parameter *paraNote = 0;

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class M3 : public zzub::plugin {
private:
  uint8_t gval[34];
  Tvals tval;
  float EnvTime(int v);
  float LFOFreq(int v);
public:
  M3();
  virtual ~M3();
  virtual void init(zzub::archive* pi);
  virtual void process_events();
  virtual bool process_stereo(float **pin, float **pout, 
			      int numsamples, int mode);
  virtual bool process_offline(float **pin, float **pout, 
			       int *numsamples, int *channels, 
			       int *samplerate) { return false; }
  virtual const char * describe_value(int param, int value); 
  virtual void process_controller_events() {}
  virtual void destroy();
  virtual void stop();
  virtual void load(zzub::archive *arc) {}
  virtual void save(zzub::archive*) {}
  virtual void attributes_changed() {}
  virtual void command(int) {}
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
  virtual void add_input(const char*, zzub::connection_type) {}
  virtual void delete_input(const char*, zzub::connection_type) {}
  virtual void rename_input(const char*, const char*) {}
  virtual void input(float**, int, float) {}
  virtual void midi_control_change(int, int, int) {}
  virtual bool handle_input(int, int, int) { return false; }
  virtual void process_midi_events(zzub::midi_message* pin, int nummessages) {}
  virtual void get_midi_output_names(zzub::outstream *pout) {}
  virtual void set_stream_source(const char* resource) {}
  virtual const char* get_stream_source() { return 0; }
  virtual void play_pattern(int index) {}
  virtual void configure(const char *key, const char *value) {}
};

struct M3Info : zzub::info {
  M3Info() {
    this->flags = 
      zzub::plugin_flag_has_audio_output;
    this->min_tracks = 1;
    this->max_tracks = 8;
    this->name = "Makk M3";
    this->short_name = "M3";
    this->author = "Makk";
    this->uri = "@libneil/somono/generator/m3;1";
    paraWave1 = &add_global_parameter()
      .set_byte()
      .set_name("Osc1Wav")
      .set_description("Oscillator 1 Waveform")
      .set_value_min(0)
      .set_value_max(5)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraPulseWidth1 = &add_global_parameter()
      .set_byte()
      .set_name("PulseWidth1")
      .set_description("Oscillator 1 Pulse Width")
      .set_value_min(0)
      .set_value_max(127)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x40);
    paraWave2 = &add_global_parameter()
      .set_byte()
      .set_name("Osc2Wav")
      .set_description("Oscillator 2 Waveform")
      .set_value_min(0)
      .set_value_max(5)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraPulseWidth2 = &add_global_parameter()
      .set_byte()
      .set_name("PulseWidth2")
      .set_description("Oscillator 2 Pulse Width")
      .set_value_min(0)
      .set_value_max(127)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x40);
    paraDetuneSemi = &add_global_parameter()
      .set_byte()
      .set_name("Semi Detune")
      .set_description("Semi Detune in Halftones")
      .set_value_min(0)
      .set_value_max(127)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x40);
    paraDetuneFine = &add_global_parameter()
      .set_byte()
      .set_name("Fine Detune")
      .set_description("Fine Detune")
      .set_value_min(0)
      .set_value_max(127)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x40);
    paraSync = &add_global_parameter()
      .set_byte()
      .set_name("Oscs Synced")
      .set_description("Sync: Osc2 synced by Osc1")
      .set_value_min(0)
      .set_value_max(1)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraMixType = &add_global_parameter()
      .set_byte()
      .set_name("MixType")
      .set_description("Mix Type")
      .set_value_min(0)
      .set_value_max(8)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraMix = &add_global_parameter()
      .set_byte()
      .set_name("Osc Mix")
      .set_description("Mix Osc1 <-> Osc2")
      .set_value_min(0)
      .set_value_max(127)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x40);
    paraSubOscWave = &add_global_parameter()
      .set_byte()
      .set_name("SubOscWav")
      .set_description("Sub Oscillator Waveform")
      .set_value_min(0)
      .set_value_max(4)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraSubOscVol = &add_global_parameter()
      .set_byte()
      .set_name("SubOscVol")
      .set_description("Sub Oscillator Volume")
      .set_value_min(0)
      .set_value_max(127)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x40);
    paraPEGAttackTime = &add_global_parameter()
      .set_byte()
      .set_name("Pitch Env Attack")
      .set_description("Pitch Envelope Attack Time")
      .set_value_min(0)
      .set_value_max(127)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraPEGDecayTime = &add_global_parameter()
      .set_byte()
      .set_name("Pitch Env Release")
      .set_description("Pitch Envelope Release Time")
      .set_value_min(0)
      .set_value_max(127)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraPEnvMod = &add_global_parameter()
      .set_byte()
      .set_name("Pitch Env Mod")
      .set_description("Pitch Envelope Modulation")
      .set_value_min(0)
      .set_value_max(127)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x40);
    paraGlide = &add_global_parameter()
      .set_byte()
      .set_name("Glide")
      .set_description("Glide")
      .set_value_min(0)
      .set_value_max(127)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraVolume = &add_global_parameter()
      .set_byte()
      .set_name("Volume")
      .set_description("Volume (Sustain-Level)")
      .set_value_min(0)
      .set_value_max(127)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x40);
    paraAEGAttackTime = &add_global_parameter()
      .set_byte()
      .set_name("Amp Env Attack")
      .set_description("Amplitude Envelope Attack Time")
      .set_value_min(0)
      .set_value_max(127)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(10);
    paraAEGSustainTime = &add_global_parameter()
      .set_byte()
      .set_name("Amp Env Sustain")
      .set_description("Amplitude Envelope Sustain Time (ms)")
      .set_value_min(0)
      .set_value_max(127)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(50);
    paraAEGReleaseTime = &add_global_parameter()
      .set_byte()
      .set_name("Amp Env Release")
      .set_description("Amplitude Envelope Release Time (ms)")
      .set_value_min(0)
      .set_value_max(127)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(30);
    paraFilterType = &add_global_parameter()
      .set_byte()
      .set_name("FilterType")
      .set_description("Filter Type ... 0=LP 1=HP 2=BP 3=BR")
      .set_value_min(0)
      .set_value_max(3)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraCutoff = &add_global_parameter()
      .set_byte()
      .set_name("Cutoff")
      .set_description("Filter Cutoff Frequency")
      .set_value_min(0)
      .set_value_max(127)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(127);
    paraResonance = &add_global_parameter()
      .set_byte()
      .set_name("Res./Bandw.")
      .set_description("Filter Resonance/Bandwidth")
      .set_value_min(0)
      .set_value_max(127)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(32);
    paraFEGAttackTime = &add_global_parameter()
      .set_byte()
      .set_name("Filter Env Attack")
      .set_description("Filter Envelope Attack Time")
      .set_value_min(0)
      .set_value_max(127)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraFEGSustainTime = &add_global_parameter()
      .set_byte()
      .set_name("Filter Env Sustain")
      .set_description("Filter Envelope Sustain Time")
      .set_value_min(0)
      .set_value_max(127)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraFEGReleaseTime = &add_global_parameter()
      .set_byte()
      .set_name("Filter Env Release")
      .set_description("Filter Envelope Release Time")
      .set_value_min(0)
      .set_value_max(127)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);    
    paraFEnvMod = &add_global_parameter()
      .set_byte()
      .set_name("Filter Env Mod")
      .set_description("Filter Envelope Modulation")
      .set_value_min(0)
      .set_value_max(127)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x40);
    paraLFO1Dest = &add_global_parameter()
      .set_byte()
      .set_name("LFO1 Dest")
      .set_description("LFO1 Destination")
      .set_value_min(0)
      .set_value_max(15)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraLFO1Wave = &add_global_parameter()
      .set_byte()
      .set_name("LFO1 Wav")
      .set_description("LFO1 Waveform")
      .set_value_min(0)
      .set_value_max(4)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraLFO1Freq = &add_global_parameter()
      .set_byte()
      .set_name("LFO1 Freq")
      .set_description("LFO1 Frequency")
      .set_value_min(0)
      .set_value_max(127)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraLFO1Amount = &add_global_parameter()
      .set_byte()
      .set_name("LFO1 Amount")
      .set_description("LFO1 Amount")
      .set_value_min(0)
      .set_value_max(127)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraLFO2Dest = &add_global_parameter()
      .set_byte()
      .set_name("LFO2 Dest")
      .set_description("LFO2 Destination")
      .set_value_min(0)
      .set_value_max(15)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraLFO2Wave = &add_global_parameter()
      .set_byte()
      .set_name("LFO2 Wav")
      .set_description("LFO2 Waveform")
      .set_value_min(0)
      .set_value_max(4)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraLFO2Freq = &add_global_parameter()
      .set_byte()
      .set_name("LFO2 Freq")
      .set_description("LFO2 Frequency")
      .set_value_min(0)
      .set_value_max(127)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraLFO2Amount = &add_global_parameter()
      .set_byte()
      .set_name("LFO2 Amount")
      .set_description("LFO2 Amount")
      .set_value_min(0)
      .set_value_max(127)
      .set_value_none(255)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraNote = &add_global_parameter()
      .set_note();
  }
  virtual zzub::plugin* create_plugin() const { return new M3(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct SoMono_M3_PluginCollection : zzub::plugincollection {
  virtual void initialize(zzub::pluginfactory *factory) {
    factory->register_info(&MachineInfo);
  }
  virtual const zzub::info *get_info(const char *uri, zzub::archive *data) { 
    return 0; 
  }
  virtual void destroy() { 
    delete this; 
  }
  virtual const char *get_uri() { 
    return 0;
  }
  virtual void configure(const char *key, const char *value) {

  }
};

zzub::plugincollection *zzub_get_plugincollection() {
  return new SoMono_M3_PluginCollection();
}

#endif // SOMONO_M3_HPP
