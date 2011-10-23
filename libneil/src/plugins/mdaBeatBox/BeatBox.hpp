#ifndef MDA_BEATBOX_HPP
#define MDA_BEATBOX_HPP

#include <zzub/signature.h>
#include <zzub/plugin.h>
#include <stdint.h>

const zzub::parameter *para_hat_thresh = 0;
const zzub::parameter *para_hat_rate = 0;
const zzub::parameter *para_hat_mix = 0;
const zzub::parameter *para_kick_thresh = 0;
const zzub::parameter *para_kick_key = 0;
const zzub::parameter *para_kick_mix = 0;
const zzub::parameter *para_snare_thresh = 0;
const zzub::parameter *para_snare_key = 0;
const zzub::parameter *para_snare_mix = 0;
const zzub::parameter *para_dynamics = 0;
const zzub::parameter *para_record = 0;
const zzub::parameter *para_thru_mix = 0;

struct Gvals {
  uint16_t hat_thresh;
  uint16_t hat_rate;
  uint16_t hat_mix;
  uint16_t kick_thresh;
  uint16_t kick_key;
  uint16_t kick_mix;
  uint16_t snare_thresh;
  uint16_t snare_key;
  uint16_t snare_mix;
  uint16_t dynamics;
  uint16_t record;
  uint16_t thru_mix;
} __attribute__((__packed__));

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class BeatBox : public zzub::plugin {
private:
  Gvals gval;
  float fParam1;
  float fParam2;
  float fParam3;
  float fParam4;
  float fParam5;
  float fParam6;
  float fParam7;
  float fParam8;
  float fParam9;
  float fParam10;
  float fParam11;
  float fParam12;
  float hthr, hfil, sthr, kthr, kfil1, kfil2, mix;
  float klev, hlev, slev;
  float ww, wwx, sb1, sb2, sf1, sf2, sf3;
  float kww, kwwx, ksb1, ksb2, ksf1, ksf2;
  float dyne, dyna, dynr, dynm;
  float *hbuf;
  float *kbuf;
  float *sbuf, *sbuf2;
  int hbuflen, hbufpos, hdel;
  int sbuflen, sbufpos, sdel, sfx;
  int kbuflen, kbufpos, kdel, ksfx;
  int rec, recx, recpos;
  void synth();
public:
  BeatBox();
  virtual ~BeatBox() {}
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
  virtual void stop() {}
  virtual void load(zzub::archive *arc) {}
  virtual void save(zzub::archive*) {}
  virtual void attributes_changed() {}
  virtual void command(int) {}
  virtual void set_track_count(int) {}
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

struct BeatBoxInfo : zzub::info {
  BeatBoxInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
    this->min_tracks = 1;
    this->max_tracks = 1;
    this->name = "mda BeatBox";
    this->short_name = "BeatBox";
    this->author = "mda";
    this->uri = "@libneil/mda/effect/beatbox";
    para_hat_thresh = &add_global_parameter()
      .set_word()
      .set_name("Hat Thr")
      .set_description("Hi-Hat threshold")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(300)
      .set_state_flag();
    para_hat_rate = &add_global_parameter()
      .set_word()
      .set_name("Hat Rate")
      .set_description("Hi-Hat rate")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(450)
      .set_state_flag();
    para_hat_mix = &add_global_parameter()
      .set_word()
      .set_name("Hat Mix")
      .set_description("Hi-Hat mix")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_kick_thresh = &add_global_parameter()
      .set_word()
      .set_name("Kik Thr")
      .set_description("Kick threshold")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(460)
      .set_state_flag();
    para_kick_key = &add_global_parameter()
      .set_word()
      .set_name("Kik Trig")
      .set_description("Kick trigger frequency")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(150)
      .set_state_flag();
    para_kick_mix = &add_global_parameter()
      .set_word()
      .set_name("Kik Mix")
      .set_description("Kick mix")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_snare_thresh = &add_global_parameter()
      .set_word()
      .set_name("Snr Thr")
      .set_description("Snare threshold")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_snare_key = &add_global_parameter()
      .set_word()
      .set_name("Snr Trig")
      .set_description("Snare trigger frequency")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(700)
      .set_state_flag();
    para_snare_mix = &add_global_parameter()
      .set_word()
      .set_name("Snr Mix")
      .set_description("Snare mix")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(500)
      .set_state_flag();
    para_dynamics = &add_global_parameter()
      .set_word()
      .set_name("Dynamics")
      .set_description("Dynamics")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(0)
      .set_state_flag();
    para_record = &add_global_parameter()
      .set_word()
      .set_name("Record:")
      .set_description("Record")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(0)
      .set_state_flag();
    para_thru_mix = &add_global_parameter()
      .set_word()
      .set_name("Thru Mix")
      .set_description("Thru mix")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(65535)
      .set_value_default(0)
      .set_state_flag();
  }
  virtual zzub::plugin* create_plugin() const { return new BeatBox(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct BeatBox_PluginCollection : zzub::plugincollection {
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
  return new BeatBox_PluginCollection();
}

#endif // MDA_BEATBOX_HPP
