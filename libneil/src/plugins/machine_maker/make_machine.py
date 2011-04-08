includes =\
"""#include <zzub/signature.h>
#include <zzub/plugin.h>
#include <stdint.h>"""

class_definition=\
"""class %%%NAME%%% : public zzub::plugin {
private:
  Gvals gval;
  Tvals tval;
  Cvals cval;
  Avals aval;
public:
  %%%NAME%%%();
  virtual ~%%%NAME%%%();
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
  virtual void event(unsigned int);
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
};"""


class Machine:
    author = None
    name = None
    global_parameters = []
    control_parameters = []
    attribute_parameters = []
    def __init__(self):
        pass

    def header_file_id(self):
        id = self.author.upper() + "_" + self.name.upper() + "_HPP"
        return id

    def preprocessor(self, body):
        id = self.header_file_id()
        output = "#ifndef %s\n#define %s\n%s\n%s\n#endif // %s" %\
            (id, id, includes, body, id)
        return output

    def make_header(self):
        body = class_definition.replace('%%%NAME%%%', self.name)
        return self.preprocessor(body)

if __name__ == '__main__':
    machine = Machine()
    machine.author = "SoMono"
    machine.name = "Gain"
    print machine.make_header()
