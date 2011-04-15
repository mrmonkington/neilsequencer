includes =\
"""\n#include <zzub/signature.h>
#include <zzub/plugin.h>
#include <stdint.h>"""

class_definition=\
"""class %%%NAME%%% : public zzub::plugin {
private:
%%%PARAMS%%%
public:
  %%%NAME%%%();
  virtual ~%%%NAME%%%() {}
  virtual void init(zzub::archive* pi);
  virtual void process_events();
  virtual bool process_stereo(float **pin, float **pout, 
			      int numsamples, int mode);
  virtual bool process_offline(float **pin, float **pout, 
			       int *numsamples, int *channels, 
			       int *samplerate) { return false; }
  virtual const char * describe_value(int param, int value); 
  virtual void process_controller_events() {}
  virtual void destroy() {}
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
%%%PARAMETERS%%%  }
  virtual zzub::plugin* create_plugin() const { return new %%%NAME%%%(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;"""

implementation =\
"""#include "%%%NAME%%%.hpp"

%%%NAME%%%::%%%NAME%%%() {
%%%PARAMS%%%
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
  static char txt[20];
  switch (param) {
  default:
    return 0;
  }
  return txt;
}
"""

signature =\
"""

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

"""

collection =\
"""

struct %%%NAME%%%_PluginCollection : zzub::plugincollection {
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
  return new %%%NAME%%%_PluginCollection();
}

"""

sconscript =\
"""Import('pluginenv', 'build_plugin')

localenv = pluginenv.Clone()

files = ["%%%NAME%%%.cpp"]

build_plugin(localenv, "%%%LIB_NAME%%%", files)"""

class Machine:
    def __init__(self, author, name, longname, uri, mtype, min_tracks, max_tracks):
        self.author = author
        self.name = name
        self.long_name = longname
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
        self.param_declarations = ""
        self.gvals_struct = ""
        self.parameter_setup = ""

    def header_file_id(self):
        id = self.author.upper() + '_' + self.name.upper() + '_HPP'
        return id

    def preprocessor(self, body):
        id = self.header_file_id()
        output = "#ifndef %s\n#define %s\n%s\n%s\n\n#endif // %s" %\
            (id, id, includes, body, id)
        return output

    def make_header(self):
        gvals = ''
        param_declarations = ''
        if self.gvals_struct != '':
            gvals = "\nstruct Gvals {\n%s} __attribute__((__packed__));\n\n" %\
                self.gvals_struct
            param_declarations += '  Gvals gval;'
        body = ('\n' + self.param_declarations +
                gvals +
                signature +
                class_definition.\
                    replace('%%%NAME%%%', self.name).\
                    replace('%%%PARAMS%%%', param_declarations) + '\n\n' +
                machine_info.\
                    replace('%%%NAME%%%', self.name).\
                    replace('%%%LONG_NAME%%%', self.long_name).\
                    replace('%%%AUTHOR%%%', self.author).\
                    replace('%%%URI%%%', self.uri).\
                    replace('%%%FLAGS%%%', self.flags).\
                    replace('%%%MIN_TRACKS%%%', str(self.min_tracks)).\
                    replace('%%%MAX_TRACKS%%%', str(self.max_tracks)).\
                    replace('%%%PARAMETERS%%%', self.parameter_setup) +
                collection.\
                    replace('%%%NAME%%%', self.name))
        return self.preprocessor(body)

    def make_implementation(self):
        param_declarations = ''
        if self.gvals_struct != '':
            param_declarations += '  global_values = &gval;'
        body = implementation.\
            replace('%%%NAME%%%', self.name).\
            replace('%%%PARAMS%%%', param_declarations)
        return body

    def make_sconscript(self):
        body = sconscript.\
            replace('%%%NAME%%%', self.name).\
            replace('%%%LIB_NAME%%%', self.lib_name)
        return body

    def add_global_parameter(self, id_, type_, name, description, min_, max_, none, default, state):
        para_name = 'para_' + id_
        description =\
            "    %s = &add_global_parameter()\n" % para_name +\
            "      .set_%s()\n" % type_ +\
            "      .set_name(\"%s\")\n" % name +\
            "      .set_description(\"%s\")\n" % description +\
            "      .set_value_min(%s)\n" % min_ +\
            "      .set_value_max(%s)\n" % max_ +\
            "      .set_value_none(%s)\n" % none +\
            "      .set_value_default(%s)\n" % default
        if state == 'yes':
            description += '      .set_state_flag();\n'
        if type_ == 'word':
            self.gvals_struct += "  uint16_t %s;\n" % id_
        else:
            self.gvals_struct += "  uint8_t %s;\n" % id_
        self.param_declarations +=\
            "const zzub::parameter *para_%s = 0;\n" % id_
        self.parameter_setup += description

if __name__ == '__main__':
    import os
    import sys
    import ConfigParser
    try:
        config_file = sys.argv[1]
        output_dir = sys.argv[2]
    except IndexError:
        print "Usage: python make_machine.py machine.cfg directory"
        sys.exit()
    if not os.path.isdir(output_dir):
        print "Specified directory does not exist!"
        sys.exit()
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
                      config.get('General', 'longname'),
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
