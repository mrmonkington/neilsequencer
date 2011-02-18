// -*- c-basic-offset: 8 -*-
// dssi plugin adapter
// Copyright (C) 2006 Leonard Ritter (contact@leonard-ritter.com)
// Copyright (C) 2008 James McDermott (jamesmichaelmcdermott@gmail.com)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.


// dssidapter allows running DSSI plugins as zzub plugins


extern "C"
{
#include <lo/lo.h>
}

extern "C"
{
#include "utils.h"
}


void query_programs(struct dssidapter *instance);
void schedule_program_change(struct dssidapter *instance, unsigned long bank, unsigned long program);

void osc_error(int num, const char *m, const char *path);
int osc_message_handler(const char *path, const char *types, lo_arg **argv, int
			argc, void *data, void *user_data) ;
int osc_debug_handler(const char *path, const char *types, lo_arg **argv, int
		      argc, void *data, void *user_data) ;



static int verbose = 0;

lo_server_thread serverThread;

using namespace zzub;

struct avals {
     int channel;
};

struct tvals {
     unsigned char note;
     unsigned char volume;
};

struct tstate {
     unsigned char note;
	
     tstate() {
          note = zzub::note_value_none;
     }
};

static struct _param_note : parameter {
     _param_note() {
          set_note();
     }
} param_note;

static struct _param_volume : parameter {
     _param_volume() {
          set_byte();
          set_name("Volume");
          set_description("Volume (00-7F)");
          set_value_min(0);
          set_value_max(0x7F);
          set_value_none(0xFF);
          set_value_default(0);
     }
} param_volume;

struct dssi_info;

struct dssidapter : plugin, event_handler
{
     char *globals;
     int track_count;
     tvals trackvals[16];
     tstate trackstates[16];
     avals aval;
     const dssi_info *myinfo;
     const DSSI_Descriptor *desc;
     void *library;
     LADSPA_Handle handle;
     LADSPA_Data *data_values;
     float inputs[16][256];
     float outputs[16][256];
     snd_seq_event_t *events;
     long unsigned int eventcount;
     zzub_plugin_t* _metaplugin;
     char *tmp;
     char *osc_url;

     lo_address uiTarget;
     lo_address uiSource;
     int ui_initial_show_sent;
     int uiNeedsProgramUpdate;
     char *ui_osc_control_path;
     char *ui_osc_configure_path;
     char *ui_osc_program_path;
     char *ui_osc_quit_path;
     char *ui_osc_show_path;

     int pluginProgramCount;
     DSSI_Program_Descriptor *pluginPrograms;
     long currentBank;
     long currentProgram;
     int pendingBankLSB;
     int pendingBankMSB;
     int pendingProgramChange;

     unsigned long arc_program_idx, arc_key_count, arc_key_size;
     char arc_key[20][100], arc_value[20][100];
     char arc_key_str[1000];

     ~dssidapter();

     dssidapter(const dssi_info *_dssi_info);
	
     virtual bool invoke(zzub_event_data_t& data);

     virtual void destroy();

     virtual void init(archive *arc);

     void read_from_archive(archive *arc);

     virtual void process_events();
	
     virtual const char * describe_value(const unsigned int param, const int value);

     virtual void set_track_count(int ntracks);

     virtual void stop();

     virtual void schedule_note_off(int t);

     virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate);
		
     virtual bool process_stereo(float **pin, float **pout, int numsamples, int const mode);

     virtual void midi_note(int channel, int value, int velocity);

     virtual void midi_note_off(int channel, int value, int velocity);

     void load(zzub::archive *arc);

     void save(zzub::archive *arc);

     void set_parameter_value_dssi(int port, float value);

     virtual void process_controller_events();
     virtual void attributes_changed();
     virtual void command(int);
     virtual void mute_track(int);
     virtual bool is_track_muted(int) const;
     virtual void event(unsigned int);
     virtual const zzub::envelope_info** get_envelope_infos();
     virtual bool play_wave(int, int, float);
     virtual void stop_wave();
     virtual int get_wave_envelope_play_position(int);
     virtual const char* describe_param(int);
     virtual bool set_instrument(const char*);
     virtual void get_sub_menu(int, zzub::outstream*);
     virtual void add_input(const char*);
     virtual void delete_input(const char*);
     virtual void rename_input(const char*, const char*);
     virtual void input(float**, int, float);
     virtual void midi_control_change(int, int, int);
     virtual bool handle_input(int, int, int);
        
};

