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



//////////////////////////////////////////////////////////////////////////
// OSC functions
//////////////////////////////////////////////////////////////////////////

void osc_error(int num, const char *msg, const char *path)
{
     fprintf(stderr, "%s: liblo server error %d in path %s: %s\n",
             myName, num, path, msg);
}

int
osc_midi_handler(struct dssidapter *instance, lo_arg **argv)
{

     static snd_midi_event_t *alsaCoder = NULL;
     static snd_seq_event_t alsaEncodeBuffer[10];
     long count;
     snd_seq_event_t *ev = &alsaEncodeBuffer[0];

     if (verbose) {
          printf("%s: OSC: got midi request for %s "
                 "(%02x %02x %02x %02x)\n", myName, instance->myinfo->m_name.c_str(),
                 argv[0]->m[0], argv[0]->m[1], argv[0]->m[2], argv[0]->m[3]);
     }

     if (!alsaCoder) {
          if (snd_midi_event_new(10, &alsaCoder)) {
               fprintf(stderr, "%s: Failed to initialise ALSA MIDI coder!\n",
                       myName);
               return 0;
          }
     }

     snd_midi_event_reset_encode(alsaCoder);

     count = snd_midi_event_encode
          (alsaCoder, (argv[0]->m) + 1, 3, alsaEncodeBuffer); /* ignore OSC "port id" in argv[0]->m[0] */

     if (!count || !snd_seq_ev_is_channel_type(ev)) {
          return 0;
     }

     /* substitute correct MIDI channel */
     ev->data.note.channel = instance->aval.channel;
    
     if (ev->type == SND_SEQ_EVENT_NOTEON && ev->data.note.velocity == 0) {
          ev->type = SND_SEQ_EVENT_NOTEOFF;
     }

     if (ev->type == SND_SEQ_EVENT_NOTEOFF) {
          // We have a midi note-off.
          if (verbose) {
               fprintf(stderr, "%s: received a MIDI note-off %d %d %d\n", myName, 
                       ev->data.note.channel, ev->data.note.note, ev->data.note.velocity);
          }
          instance->midi_note_off(ev->data.note.channel, ev->data.note.note, ev->data.note.velocity);
          return 0;
     }
        
     if (ev->type == SND_SEQ_EVENT_CONTROLLER &&
         (ev->data.control.param == 0 || ev->data.control.param == 32)) {

          fprintf(stderr, "%s: Warning: %s UI sent bank select controller (should use /program OSC call), ignoring\n", myName, instance->myinfo->m_name.c_str());

     } else if (ev->type == SND_SEQ_EVENT_PGMCHANGE) {

          fprintf(stderr, "%s: Warning: %s UI sent program change (should use /program OSC call), ignoring\n", myName, instance->myinfo->m_name.c_str());

     } else {
          if (ev->type == SND_SEQ_EVENT_NOTEON) {
               // We have a midi note.
               if (verbose) {
                    fprintf(stderr, "%s: received a MIDI note %d %d %d\n", myName, 
                            ev->data.note.channel, ev->data.note.note, ev->data.note.velocity);
               }
               instance->midi_note(ev->data.note.channel, ev->data.note.note, ev->data.note.velocity);
          }
     }

     return 0;
}

int
osc_control_handler(struct dssidapter *instance, lo_arg **argv)
{
     int port = argv[0]->i;
     LADSPA_Data value = argv[1]->f;

     if (port < 0 || port >= instance->desc->LADSPA_Plugin->PortCount) {
          fprintf(stderr, "%s: OSC: %s port number (%d) is out of range\n",
                  myName, instance->myinfo->m_name.c_str(), port);
          return 0;
     }

     instance->data_values[instance->myinfo->portIdx2ControlInIdx[port]] = value;
     instance->set_parameter_value_dssi(port, value);

     if (verbose) { 
          printf("%s: OSC: %s port %d = %f\n", 
                 myName, instance->myinfo->m_name.c_str(), port, value); 
     }
     
     // we have altered a parameter value, so we no longer wish to keep
     // state using the Program method.
     instance->pendingBankMSB = -1;
     instance->pendingBankLSB = -1;
     instance->pendingProgramChange = -1;
     instance->arc_program_idx = -1;
    
     return 0;
}

int
osc_program_handler(struct dssidapter *instance, lo_arg **argv)
{
     int bank = argv[0]->i;
     int program = argv[1]->i;

     schedule_program_change(instance, bank, program);
     return 0;
}

void schedule_program_change(struct dssidapter *instance, unsigned long bank, unsigned long program) {

     int i;
     int found = 0;

     for (i = 0; i < instance->pluginProgramCount; ++i) {
          if (instance->pluginPrograms[i].Bank == bank &&
              instance->pluginPrograms[i].Program == program) {
               if (verbose) {
                    printf("%s: OSC: %s setting bank %d, program %d, name %s\n",
                           myName,
                           instance->myinfo->m_name.c_str(),
                           bank, program,
                           instance->pluginPrograms[i].Name);
               }
               found = 1;
               instance->arc_program_idx = i;
               break;
          }
     }

     if (!found) {
          printf("%s: OSC: %s UI requested unknown program: bank %d, program %d: sending to plugin anyway (plugin should ignore it)\n",
                 myName, 
                 instance->myinfo->m_name.c_str(),
                 bank, program);
     }

     instance->pendingBankMSB = bank / 128;
     instance->pendingBankLSB = bank % 128;
     instance->pendingProgramChange = program;
     return;
}

int
osc_configure_handler(struct dssidapter *instance, lo_arg **argv)
{
     if (verbose) printf("in osc_configure_handler\n");

     const char *key = (const char *)&argv[0]->s;
     const char *value = (const char *)&argv[1]->s;
     char *message;

     if (instance->desc->configure) {

          if (!strncmp(key, DSSI_RESERVED_CONFIGURE_PREFIX,
                       strlen(DSSI_RESERVED_CONFIGURE_PREFIX))) {
               fprintf(stderr, "%s: OSC: UI for plugin '%s' attempted to use reserved configure key \"%s\", ignoring\n", 
                       myName, instance->myinfo->m_name.c_str(), key);

               return 0;
          }

          message = instance->desc->configure(instance->handle, key, value);
          if (message) {
               printf("%s: on configure '%s' '%s', plugin '%s' returned error '%s'\n",
                      myName, key, value, instance->myinfo->m_name.c_str(), message);
               free(message);
          }

          /* configure invalidates bank and program information, so
             we should do this again now: */
          query_programs(instance);
     }

     // save the key and value
     if (!strcmp(key, "shm_attach") || !strcmp(key, "shm_detach")) {
          if (verbose) printf("not saving key <%s>\n", key);
          return 0;
     }
     int i;
     for (i = 0; i < instance->arc_key_count; i++) {
          if (!strcmp(key, instance->arc_key[i])) {
               if (verbose) printf("in configure handler, found key <%s>, saving value <%s>\n", key, value);
               strcpy(instance->arc_value[i], value);
               break;
          }
     }
     if (i == instance->arc_key_count) {
          // got past the end of the array: key not found
          if (verbose) printf("in configure handler, didn't find key <%s>, adding value <%s>\n", key, value);
          strcpy(instance->arc_key[i], key);
          strcpy(instance->arc_value[i], value);
          instance->arc_key_count++;
     }
     return 0;
}

int
osc_update_handler(struct dssidapter *instance, lo_arg **argv, lo_address source)
{

     if (verbose) printf("in osc_update_handler\n");

     const char *url = (char *)&argv[0]->s;
     const char *path;
     unsigned int i;
     char *host, *port;
     const char *chost, *cport;

     if (verbose) {
          printf("%s: OSC: got update request from <%s>\n", myName, url);
     }

     if (instance->uiTarget) lo_address_free(instance->uiTarget);
     host = lo_url_get_hostname(url);
     port = lo_url_get_port(url);
     instance->uiTarget = lo_address_new(host, port);
     free(host);
     free(port);

     if (instance->uiSource) lo_address_free(instance->uiSource);
     chost = lo_address_get_hostname(source);
     cport = lo_address_get_port(source);
     instance->uiSource = lo_address_new(chost, cport);

     path = lo_url_get_path(url);
     if (verbose) {
          printf("path is <%s>\n", path);
     }

     if (instance->ui_osc_control_path) free(instance->ui_osc_control_path);
     instance->ui_osc_control_path = (char *)malloc(strlen(path) + 10);
     sprintf(instance->ui_osc_control_path, "%s/control", path);

     if (instance->ui_osc_configure_path) free(instance->ui_osc_configure_path);
     instance->ui_osc_configure_path = (char *)malloc(strlen(path) + 12);
     sprintf(instance->ui_osc_configure_path, "%s/configure", path);

     if (instance->ui_osc_program_path) free(instance->ui_osc_program_path);
     instance->ui_osc_program_path = (char *)malloc(strlen(path) + 10);
     sprintf(instance->ui_osc_program_path, "%s/program", path);

     if (instance->ui_osc_quit_path) free(instance->ui_osc_quit_path);
     instance->ui_osc_quit_path = (char *)malloc(strlen(path) + 10);
     sprintf(instance->ui_osc_quit_path, "%s/quit", path);

     if (instance->ui_osc_show_path) free(instance->ui_osc_show_path);
     instance->ui_osc_show_path = (char *)malloc(strlen(path) + 10);
     sprintf(instance->ui_osc_show_path, "%s/show", path);

     free((char *)path);

     // if (projectDirectory) {
     //     lo_send(instance->uiTarget, instance->ui_osc_configure_path, "ss",
     //     	DSSI_PROJECT_DIRECTORY_KEY, projectDirectory);
     // }
     for (int i = 0; i < instance->arc_key_count; i++) {
          if (verbose) printf("in update_handler: lo_send to configure_path: key <%s> value <%s>\n", instance->arc_key[i], instance->arc_value[i]);
          lo_send(instance->uiTarget, instance->ui_osc_configure_path, "ss",
                  instance->arc_key[i], instance->arc_value[i]);
     }


     /* Send current bank/program  (-FIX- another race...) */
     if (instance->pendingProgramChange < 0) {
          unsigned long bank = instance->currentBank;
          unsigned long program = instance->currentProgram;
          instance->uiNeedsProgramUpdate = 0;
          if (instance->uiTarget) {
               lo_send(instance->uiTarget, instance->ui_osc_program_path, "ii", bank, program);
          }
     }
     ///XXXX
     /* Send control ports */
     for (int i = 0; i < instance->desc->LADSPA_Plugin->PortCount; i++) {

	  int port_idx = instance->myinfo->portIdx2ControlInIdx[i];
	  if (port_idx != -1) {
               if (verbose) printf("in update_handler: port %d, controlIdx %d, value %f\n", 
                                   i, port_idx, instance->data_values[port_idx]);
               lo_send(instance->uiTarget, instance->ui_osc_control_path, "if", i,
                       instance->data_values[port_idx]);
	  }
	  /* Avoid overloading the GUI if there are lots and lots of ports */
	  if ((i+1) % 50 == 0) usleep(300000);
     }

     /* Send 'show' */
     if (!instance->ui_initial_show_sent) {
          lo_send(instance->uiTarget, instance->ui_osc_show_path, "");
          instance->ui_initial_show_sent = 1;
     }

     return 0;
}

int
osc_exiting_handler(struct dssidapter *instance, lo_arg **argv)
{
     if (verbose) {
          printf("%s: OSC: got exiting notification\n", myName);
     }

     if (instance->uiTarget) {
          lo_address_free(instance->uiTarget);
          instance->uiTarget = NULL;
     }

     if (instance->uiSource) {
          lo_address_free(instance->uiSource);
          instance->uiSource = NULL;
     }
     instance->ui_initial_show_sent = 0;

     return 0;
}

int osc_debug_handler(const char *path, const char *types, lo_arg **argv,
                      int argc, void *data, void *user_data)
{
     int i;

     printf("%s: got unhandled OSC message:\npath: <%s>\n", myName, path);
     for (i=0; i<argc; i++) {
          printf("%s: arg %d '%c' ", myName, i, types[i]);
          lo_arg_pp((lo_type) types[i], argv[i]);
          printf("\n");
     }
     printf("%s:\n", myName);

     return 1;
}

int osc_message_handler(const char *path, const char *types, lo_arg **argv,
                        int argc, void *data, void *user_data)
{
     int i;
     struct dssidapter *instance = (struct dssidapter *) user_data;
     const char *method;
     unsigned int flen = 0;
     lo_message message;
     lo_address source;
     int send_to_ui = 0;


     if (strncmp(path, "/dssi/", 6))
          return osc_debug_handler(path, types, argv, argc, data, user_data);

     method = path + 5 + flen;
     if (*method != '/' || *(method + 1) == 0)
          return osc_debug_handler(path, types, argv, argc, data, user_data);
     method++;

     message = (lo_message)data;
     source = lo_message_get_source(message);

     if (instance->uiSource && instance->uiTarget) {
          if (strcmp(lo_address_get_hostname(source),
                     lo_address_get_hostname(instance->uiSource)) ||
              strcmp(lo_address_get_port(source),
                     lo_address_get_port(instance->uiSource))) {
               /* This didn't come from our known UI for this plugin,
                  so send an update to that as well */
               send_to_ui = 1;
          }
     }

     if (!strcmp(method, "configure") && argc == 2 && !strcmp(types, "ss")) {

          if (send_to_ui) {
               lo_send(instance->uiTarget, instance->ui_osc_configure_path, "ss",
                       &argv[0]->s, &argv[1]->s);
          }

          return osc_configure_handler(instance, argv);

     } else if (!strcmp(method, "control") && argc == 2 && !strcmp(types, "if")) {

          if (send_to_ui) {
               lo_send(instance->uiTarget, instance->ui_osc_control_path, "if",
                       argv[0]->i, argv[1]->f);
          }

          return osc_control_handler(instance, argv);

     } else if (!strcmp(method, "midi") && argc == 1 && !strcmp(types, "m")) {

          return osc_midi_handler(instance, argv);

     } else if (!strcmp(method, "program") && argc == 2 && !strcmp(types, "ii")) {

          if (send_to_ui) {
               lo_send(instance->uiTarget, instance->ui_osc_program_path, "ii",
                       argv[0]->i, argv[1]->i);
          }
	
          return osc_program_handler(instance, argv);

     } else if (!strcmp(method, "update") && argc == 1 && !strcmp(types, "s")) {

          return osc_update_handler(instance, argv, source);

     } else if (!strcmp(method, "exiting") && argc == 0) {

          return osc_exiting_handler(instance, argv);
     }

     return osc_debug_handler(path, types, argv, argc, data, user_data);
}



