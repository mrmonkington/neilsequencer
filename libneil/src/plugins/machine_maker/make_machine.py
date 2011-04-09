includes =\
"""#include <zzub/signature.h>
#include <zzub/plugin.h>
#include <stdint.h>"""

class_definition=\
"""class %%%NAME%%% : public zzub::plugin {
private:
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

machine_info =\
"""struct %%%NAME%%%Info : zzub::info {
  %%%NAME%%%Info() {
    this->flags = 
      %%%FLAGS%%%;
    this->min_tracks = %%%MIN_TRACKS%%%;
    this->max_tracks = %%%MAX_TRACKS%%%;
    this->name = "%%%LONG_NAME%%%";
    this->short_name = "%%%NAME%%%";
    this->author = "%%%AUTHOR%%%";
    this->uri = "%%%URI%%%";
    %%%PARAMETERS%%%
  }
  virtual zzub::plugin* create_plugin() const { return new %%%NAME%%%(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;"""

implementation =\
"""#include "%%%NAME%%%.hpp"

%%%NAME%%%::%%%NAME%%%() {
  global_values = 0;
  attributes = 0;
  track_values = 0;
}

void %%%NAME%%%::init(zzub::archive *pi) {

}
	
void %%%NAME%%%::process_events() {

}

bool %%%NAME%%%::process_stereo(float **pin, float **pout, int n, int mode) {
  return true;
}

const char *%%%NAME%%%::describe_value(int param, int value) {
  return 0;
}
"""

sconscript =\
"""Import('pluginenv', 'build_plugin')

localenv = pluginenv.Clone()

files = ["%%%NAME%%%.cpp"]

build_plugin(localenv, "%%%LIB_NAME%%%", files)"""

class Machine:
    def __init__(self, author, name, uri, mtype, min_tracks, max_tracks):
        self.author = author
        self.name = name
        self.long_name = author + ' ' + name
        self.lib_name = author.lower() + '_' + name.lower()
        self.uri = uri
        self.mtype = mtype
        self.cpp_name = self.name + '.cpp'
        self.hpp_name = self.name + '.hpp'
        if mtype == 'Effect':
            self.flags = 'zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output'
        elif mtype == 'Generator':
            self.flags = 'zzub::plugin_flag_has_audio_output'
        elif mtype == 'Controller':
            self.flags = 'zzub::plugin_flag_has_event_output'
        self.min_tracks = min_tracks
        self.max_tracks = max_tracks
        self.parameter_setup = ""

    def header_file_id(self):
        id = self.author.upper() + '_' + self.name.upper() + '_HPP'
        return id

    def preprocessor(self, body):
        id = self.header_file_id()
        output = "#ifndef %s\n#define %s\n%s\n%s\n#endif // %s" %\
            (id, id, includes, body, id)
        return output

    def make_header(self):
        body = (class_definition.replace('%%%NAME%%%', self.name) + '\n\n' +
                machine_info.\
                    replace('%%%NAME%%%', self.name).\
                    replace('%%%LONG_NAME%%%', self.long_name).\
                    replace('%%%AUTHOR%%%', self.author).\
                    replace('%%%URI%%%', self.uri).\
                    replace('%%%FLAGS%%%', self.flags).\
                    replace('%%%MIN_TRACKS%%%', str(self.min_tracks)).\
                    replace('%%%MAX_TRACKS%%%', str(self.max_tracks)).\
                    replace('%%%PARAMETERS%%%', self.parameter_setup))
        return self.preprocessor(body)

    def make_implementation(self):
        body = implementation.replace('%%%NAME%%%', self.name)
        return body

    def make_sconscript(self):
        body = sconscript.\
            replace('%%%NAME%%%', self.name).\
            replace('%%%LIB_NAME%%%', self.lib_name)
        return body

    def add_global_parameter(self, id_, type_, name, description, min_, max_, none, default, state):
        para_name = 'para_' + id_
        description =\
            "%s = &add_global_parameter()\n" % para_name +\
            "  .set_%s()\n" % type_ +\
            "  .set_name(\"%s\")\n" % name +\
            "  .set_description(\"%s\")\n" % description +\
            "  .set_value_min(%s)\n" % min_ +\
            "  .set_value_max(%s)\n" % max_ +\
            "  .set_value_none(%s)\n" % none +\
            "  .set_value_default(%s)\n" % default
        if state == "yes":
            description += '  .set_state_flag();\n'
        self.parameter_setup += description

if __name__ == '__main__':
    import sys
    import ConfigParser
    config_file = sys.argv[1]
    output_dir = sys.argv[2]
    config = ConfigParser.RawConfigParser()
    config.read(config_file)
    try:
        min_tracks = config.get('General', 'min_tracks')
        max_tracks = config.get('General', 'max_tracks')
    except ConfigParser.NoOptionError:
        min_tracks = 1
        max_tracks = 1
    try:
        global_params = [p.strip() for p in config.get('General', 'globals').split(',')]
    except ConfigParser.NoOptionError:
        global_params = []
    machine = Machine(config.get('General', 'author'),
                      config.get('General', 'name'),
                      config.get('General', 'uri'),
                      config.get('General', 'type'),
                      min_tracks,
                      max_tracks)
    for param in global_params:
        machine.add_global_parameter(param,
                                     config.get(param, 'type'),
                                     config.get(param, 'name'),
                                     config.get(param, 'description'),
                                     config.get(param, 'min'),
                                     config.get(param, 'max'),
                                     config.get(param, 'none'),
                                     config.get(param, 'default'),
                                     config.get(param, 'state'))
    fhpp = open(output_dir + '/' + machine.hpp_name, 'w')
    fcpp = open(output_dir + '/' + machine.cpp_name, 'w')
    fscons = open(output_dir + '/SConscript', 'w')
    fhpp.write(machine.make_header())
    fcpp.write(machine.make_implementation())
    fscons.write(machine.make_sconscript())
    print "All files successfully written!"
