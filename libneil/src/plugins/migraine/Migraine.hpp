#ifndef EDSCA_MIGRAINE_HPP
#define EDSCA_MIGRAINE_HPP

#include <stdint.h>

#include <zzub/signature.h>
#include <zzub/plugin.h>

struct Gvals {
  uint8_t paraIngain;
  uint8_t paraInclip;
  uint8_t paraCheb2;
  uint8_t paraCheb2Ret;
  uint8_t paraCheb3;
  uint8_t paraCheb3Ret;
  uint8_t paraCheb4;
  uint8_t paraCheb4Ret;
  uint8_t paraCheb5;
  uint8_t paraCheb5Ret;
  uint8_t paraCheb6;
  uint8_t paraCheb6Ret;
  uint8_t paraCheb7;
  uint8_t paraCheb7Ret;
  uint8_t paraCheb8;
  uint8_t paraCheb8Ret;
  uint8_t paraLeg2;
  uint8_t paraLeg2Ret;
  uint8_t paraLeg3;
  uint8_t paraLeg3Ret;
  uint8_t paraLeg4;
  uint8_t paraLeg4Ret;
  uint8_t paraLeg5;
  uint8_t paraLeg5Ret;
  uint8_t paraLeg6;
  uint8_t paraLeg6Ret;
  uint8_t paraLeg7;
  uint8_t paraLeg7Ret;
  uint8_t paraLeg8;
  uint8_t paraLeg8Ret;
  uint8_t paraDry;
  uint8_t paraWet;
  uint8_t paraOutclip;
} __attribute__((__packed__));

const zzub::parameter *paraIngain = 0;
const zzub::parameter *paraInclip = 0;
const zzub::parameter *paraCheb2 = 0;
const zzub::parameter *paraCheb2Ret = 0;
const zzub::parameter *paraCheb3 = 0;
const zzub::parameter *paraCheb3Ret = 0;
const zzub::parameter *paraCheb4 = 0;
const zzub::parameter *paraCheb4Ret = 0;
const zzub::parameter *paraCheb5 = 0;
const zzub::parameter *paraCheb5Ret = 0;
const zzub::parameter *paraCheb6 = 0;
const zzub::parameter *paraCheb6Ret = 0;
const zzub::parameter *paraCheb7 = 0;
const zzub::parameter *paraCheb7Ret = 0;
const zzub::parameter *paraCheb8 = 0;
const zzub::parameter *paraCheb8Ret = 0;
const zzub::parameter *paraLeg2 = 0;
const zzub::parameter *paraLeg2Ret = 0;
const zzub::parameter *paraLeg3 = 0;
const zzub::parameter *paraLeg3Ret = 0;
const zzub::parameter *paraLeg4 = 0;
const zzub::parameter *paraLeg4Ret = 0;
const zzub::parameter *paraLeg5 = 0;
const zzub::parameter *paraLeg5Ret = 0;
const zzub::parameter *paraLeg6 = 0;
const zzub::parameter *paraLeg6Ret = 0;
const zzub::parameter *paraLeg7 = 0;
const zzub::parameter *paraLeg7Ret = 0;
const zzub::parameter *paraLeg8 = 0;
const zzub::parameter *paraLeg8Ret = 0;
const zzub::parameter *paraDry = 0;
const zzub::parameter *paraWet = 0;
const zzub::parameter *paraOutclip = 0;

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class Migraine : public zzub::plugin {
private:
  Gvals gval;
  float param_ingain;
  float param_cheb2, param_cheb3, param_cheb4, param_cheb5, param_cheb6;
  float param_cheb7, param_cheb8, param_dry;
  float param_cheb2ret, param_cheb3ret, param_cheb4ret, param_cheb5ret, 
    param_cheb6ret;
  float param_cheb7ret, param_cheb8ret;
  float param_leg2, param_leg3, param_leg4, param_leg5, param_leg6;
  float param_leg7, param_leg8;
  float param_leg2ret, param_leg3ret, param_leg4ret, param_leg5ret;
  float param_leg6ret, param_leg7ret, param_leg8ret, param_wet;
  int param_inclip, param_outclip;
  float coefs[9][14];
  float dcspeedL, dcposL, dcspeedR, dcposR;
  float temp;
public:
  Migraine();
  virtual ~Migraine();
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

struct MigraineInfo : zzub::info {
  MigraineInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
    this->name = "Edsca Migraine";
    this->short_name = "Migraine";
    this->author = "Edsca";
    this->uri = "@libneil/edsca/effect/Migraine";
    paraIngain = &add_global_parameter()
      .set_byte()
      .set_name("Input gain")
      .set_description("Input gain")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x7f);
    paraInclip = &add_global_parameter()
      .set_byte()
      .set_name("In->Soft")
      .set_description("Input clipping: 0 - hard, 1 - soft")
      .set_value_min(0x00)
      .set_value_max(0x01)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);
    paraCheb2 = &add_global_parameter()
      .set_byte()
      .set_name("Cheb2 Send")
      .set_description("Amount sent to Chebyshev Polynomial T2")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);
    paraCheb2Ret = &add_global_parameter()
      .set_byte()
      .set_name("Cheb2 Ret")
      .set_description("Return amount of Chebyshev Polynomial T2")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);
    paraCheb3 = &add_global_parameter()
      .set_byte()
      .set_name("Cheb3 Send")
      .set_description("Amount sent to Chebyshev Polynomial T3")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);
    paraCheb3Ret = &add_global_parameter()
      .set_byte()
      .set_name("Cheb3 Ret")
      .set_description("Return amount of Chebyshev Polynomial T3")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);
    paraCheb4 = &add_global_parameter()
      .set_byte()
      .set_name("Cheb4 Send")
      .set_description("Amount sent to Chebyshev Polynomial T4")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);
    paraCheb4Ret = &add_global_parameter()
      .set_byte()
      .set_name("Cheb4 Ret")
      .set_description("Return amount of Chebyshev Polynomial T4")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);
    paraCheb5 = &add_global_parameter()
      .set_byte()
      .set_name("Cheb5 Send")
      .set_description("Amount sent to Chebyshev Polynomial T5")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);
    paraCheb5Ret = &add_global_parameter()
      .set_byte()
      .set_name("Cheb5 Ret")
      .set_description("Return amount of Chebyshev Polynomial T5")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);
    paraCheb6 = &add_global_parameter()
      .set_byte()
      .set_name("Cheb6 Send")
      .set_description("Amount sent to Chebyshev Polynomial T6")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);
    paraCheb6Ret = &add_global_parameter()
      .set_byte()
      .set_name("Cheb6 Ret")
      .set_description("Return amount of Chebyshev Polynomial T6")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);
    paraCheb7 = &add_global_parameter()
      .set_byte()
      .set_name("Cheb7 Send")
      .set_description("Amount sent to Chebyshev Polynomial T7")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);
    paraCheb7Ret = &add_global_parameter()
      .set_byte()
      .set_name("Cheb7 Ret")
      .set_description("Return amount of Chebyshev Polynomial T7")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);
    paraCheb8 = &add_global_parameter()
      .set_byte()
      .set_name("Cheb8 Send")
      .set_description("Amount sent to Chebyshev Polynomial T8")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);
    paraCheb8Ret = &add_global_parameter()
      .set_byte()
      .set_name("Cheb8 Ret")
      .set_description("Return amount of Chebyshev Polynomial T8")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);
    paraLeg2 = &add_global_parameter()
      .set_byte()
      .set_name("Leg2 Send")
      .set_description("Amount sent to Legendre Polynomial P2")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);
    paraLeg2Ret = &add_global_parameter()
      .set_byte()
      .set_name("Leg2 Ret")
      .set_description("Return amount of Legendre Polynomial P2")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);
    paraLeg3 = &add_global_parameter()
      .set_byte()
      .set_name("Leg3 Send")
      .set_description("Amount sent to Legendre Polynomial P3")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);
    paraLeg3Ret = &add_global_parameter()
      .set_byte()
      .set_name("Leg3 Ret")
      .set_description("Return amount of Legendre Polynomial P3")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);
    paraLeg4 = &add_global_parameter()
      .set_byte()
      .set_name("Leg4 Send")
      .set_description("Amount sent to Legendre Polynomial P4")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);
    paraLeg4Ret = &add_global_parameter()
      .set_byte()
      .set_name("Leg4 Ret")
      .set_description("Return amount of Legendre Polynomial P4")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);
    paraLeg5 = &add_global_parameter()
      .set_byte()
      .set_name("Leg5 Send")
      .set_description("Amount sent to Legendre Polynomial P5")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);
    paraLeg5Ret = &add_global_parameter()
      .set_byte()
      .set_name("Leg5 Ret")
      .set_description("Return amount of Legendre Polynomial P5")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);
    paraLeg6 = &add_global_parameter()
      .set_byte()
      .set_name("Leg6 Send")
      .set_description("Amount sent to Legendre Polynomial P6")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);
    paraLeg6Ret = &add_global_parameter()
      .set_byte()
      .set_name("Leg6 Ret")
      .set_description("Return amount of Legendre Polynomial P6")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);
    paraLeg7 = &add_global_parameter()
      .set_byte()
      .set_name("Leg7 Send")
      .set_description("Amount sent to Legendre Polynomial P7")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);
    paraLeg7Ret = &add_global_parameter()
      .set_byte()
      .set_name("Leg7 Ret")
      .set_description("Return amount of Legendre Polynomial P7")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);
    paraLeg8 = &add_global_parameter()
      .set_byte()
      .set_name("Leg8 Send")
      .set_description("Amount sent to Legendre Polynomial P8")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);
    paraLeg8Ret = &add_global_parameter()
      .set_byte()
      .set_name("Leg8 Ret")
      .set_description("Return amount of Legendre Polynomial P8")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);
    paraDry = &add_global_parameter()
      .set_byte()
      .set_name("Dry Amp")
      .set_description("Amplitude of Dry Signal")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x7f);
    paraWet = &add_global_parameter()
      .set_byte()
      .set_name("Wet Amp")
      .set_description("Amplitude of Wet Signal")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x7f);
    paraOutclip = &add_global_parameter()
      .set_byte()
      .set_name("Out->Soft")
      .set_description("Output clipping: 0 - hard, 1 - soft")
      .set_value_min(0x00)
      .set_value_max(0x01)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x00);    
  }
  virtual zzub::plugin* create_plugin() const { return new Migraine(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct Edsca_Migraine_PluginCollection : zzub::plugincollection {
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
  return new Edsca_Migraine_PluginCollection();
}

#endif // EDSCA_MIGRAINE_HPP
