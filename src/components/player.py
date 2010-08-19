#encoding: latin-1

# Neil
# Modular Sequencer
# Copyright (C) 2006,2007,2008 The Neil Development Team
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

import zzub
from zzub import Player
import neil.com as com

import neil.common as common
import gobject
import os,sys
import time
from neil.utils import is_generator, is_effect, is_root, is_controller, is_streamer, \
        PropertyEventHandler, generate_ui_methods, refresh_gui
from config import get_plugin_aliases, get_plugin_blacklist

import gtk

DOCUMENT_UI = dict(
        # insert persistent members at this level, in the format
        #
        # <member name> = <descriptor dict>
        #
        # <member name> is the base name of the getter/setter/property values
        # and the private member variable.
        #
        # keys you can enter into the dict:
        #
        # default: initialization value. if omitted, a default will be deduced
        # from the value type.
        #
        # vtype: value type as type object. if omitted, the type will be deduced from
        #        the default value. allowed types are int, bool, str, float.
        #
        # doc: a doc string describing the meaning of the setting. name it so that
        #      it can be appended to "Returns ..." and "Sets ...". if omitted,
        #      a default will be used.
        #
        # event: name of the global event to be triggered. usually generated
        #        by default if not given.
        #
        # onset: an optional function to be called before the value is assigned.
        #            the function should have the signature
        #
        #        def func(value): -> value
        #
        # onget: an optional function to be called before the value is returned
        #        to the calling function.
        #
        #        def func(value): -> value
        #
        # for the setting below, active_plugins, you can access player.get_active_plugins(),
        # player.set_active_plugins(plugins), and player.active_plugins as a property.
        # when changed, the event active_plugins_changed will be triggered.
        active_plugins = dict(vtype=zzub.Plugin,list=True,doc="the list of active plugins."),
        active_patterns = dict(vtype=(zzub.Plugin,int),list=True,doc="the list of active patterns (zzub.Plugin, index)."),
        active_waves = dict(vtype=zzub.Wave,list=True,doc="the list of active waves."),
        octave = dict(vtype=int,default=4,doc="the current octave to be used for keyjazz."),
        autoconnect_target = dict(vtype=zzub.Plugin,doc="the plugin to connect to automatically when creating a new plugin."),
        sequence_step = dict(vtype=int,default=64,doc="the current step size for sequencers."),
        plugin_origin = dict(vtype=int,list=True,default=[0.0,0.0],doc="the origin position for new plugins."),
        solo_plugin = dict(vtype=zzub.Plugin,doc="if set, the plugin that is currently set to solo."),
        document_path = dict(vtype=str,doc="path to the current document."),
)

class NeilPlayer(Player, PropertyEventHandler):
    __neil__ = dict(
            id = 'neil.core.player',
            singleton = True,
            categories = [
                    'pythonconsole.locals',
            ],
    )

    _exclude_event_debug_ = [
            zzub.zzub_event_type_parameter_changed,
    ]

    _event_types_ = dict(
            zzub_event_type_double_click = dict(args=None),
            zzub_event_type_new_plugin = dict(args='new_plugin'),
            zzub_event_type_delete_plugin = dict(args='delete_plugin'),
            zzub_event_type_pre_delete_plugin = dict(args='delete_plugin'),
            zzub_event_type_disconnect = dict(args='disconnect_plugin'),
            zzub_event_type_connect = dict(args='connect_plugin'),
            zzub_event_type_plugin_changed = dict(args='plugin_changed'),
            zzub_event_type_parameter_changed = dict(args='change_parameter'),
            zzub_event_type_set_tracks = dict(args=None),
            zzub_event_type_set_sequence_tracks = dict(args='set_sequence_tracks'),
            zzub_event_type_set_sequence_event = dict(args='set_sequence_event'),
            zzub_event_type_new_pattern = dict(args='new_pattern'),
            zzub_event_type_delete_pattern = dict(args='delete_pattern'),
            zzub_event_type_pre_delete_pattern = dict(args='delete_pattern'),
            zzub_event_type_edit_pattern = dict(args='edit_pattern'),
            zzub_event_type_pattern_changed = dict(args='pattern_changed'),
            zzub_event_type_pattern_insert_rows = dict(args='pattern_insert_rows'),
            zzub_event_type_pattern_remove_rows = dict(args='pattern_remove_rows'),
            zzub_event_type_sequencer_add_track = dict(args=None),
            zzub_event_type_sequencer_remove_track = dict(args=None),
            zzub_event_type_sequencer_changed = dict(args=None),
            zzub_event_type_pre_disconnect = dict(args=None),
            zzub_event_type_pre_connect = dict(args=None),
            zzub_event_type_post_connect = dict(args=None),
            zzub_event_type_pre_set_tracks = dict(args=None),
            zzub_event_type_post_set_tracks = dict(args=None),
            zzub_event_type_envelope_changed = dict(args=None),
            zzub_event_type_slices_changed = dict(args=None),
            zzub_event_type_wave_changed = dict(args='change_wave'),
            zzub_event_type_delete_wave = dict(args='delete_wave'),
            zzub_event_type_load_progress = dict(args=None),
            zzub_event_type_midi_control = dict(args='midi_message'),
            zzub_event_type_wave_allocated = dict(args='allocate_wavelevel'),
            zzub_event_type_player_state_changed = dict(args='player_state_changed'),
            zzub_event_type_osc_message = dict(args='osc_message'),
            zzub_event_type_vu = dict(args='vu'),
            zzub_event_type_custom = dict(args='custom'),
            zzub_event_type_all = dict(args='all'),
    )

    def __init__(self):
        Player.__init__(self, Player.create())
        self._cbtime = time.time()
        self._cbcalls = 0
        self._hevcalls = 0
        self._hevtime = 0
        self.__lazy_commits = False
        self.__event_stats = False
        # enumerate zzub_event_types and prepare unwrappers for the different types
        self.event_id_to_name = {}
        for enumname,cfg in self._event_types_.iteritems():
            val = getattr(zzub, enumname)
            assert val not in self.event_id_to_name, "value %s (%s) already registered." % (val,eventname)
            eventname = 'zzub_' + enumname[len('zzub_event_type_'):]
            membername = cfg.get('args',None)
            args = []
            if membername:
                union = None
                datatype = None
                ed = zzub.EventData()
                for argname in dir(ed):
                    if argname == membername:
                        datatype = getattr(ed, argname).__class__
                assert datatype, "couldn't find member %s in zzub_event_data_t" % membername
                for argname,argtype in datatype._fields_:
                    args.append(argname)
            self.event_id_to_name[val] = (eventname, membername, args)
            #print "'%s', # ( %s )" % (eventname, ','.join(args + ["..."]))
        config = com.get('neil.core.config')
        pluginpath = os.environ.get('NEIL_PLUGIN_PATH',None)
        if pluginpath:
            pluginpaths = pluginpath.split(os.pathsep)

        else:
            pluginpaths = []
            paths = os.environ.get('LD_LIBRARY_PATH',None) # todo or PATH on mswindows
            print paths
            if paths:
                paths = paths.split(os.pathsep)
            else:
                paths = []
            paths.extend([
                            '/usr/local/lib64',
                            '/usr/local/lib',
                            '/usr/lib64',
                            '/usr/lib',
                    ])
            for path in [os.path.join(path, 'zzub') for path in paths]:
                if os.path.exists(path) and not path in pluginpaths: pluginpaths.append(path)
            print pluginpaths
        for pluginpath in pluginpaths:
            print 'plugin path:', pluginpath
            self.add_plugin_path(pluginpath + os.sep)

        inputname, outputname, samplerate, buffersize = config.get_audiodriver_config()
        self.initialize(samplerate)
        self.init_lunar()
        self.__stream_ext_uri_mappings = {}
        self.__streamplayer = None
        self.__streamrecorder = None
        self.__loading = False
        self.enumerate_stream_plugins()
        self.playstarttime = time.time()
        self.document_unchanged()
        #self.spinbox_edit = False
        eventbus = com.get('neil.core.eventbus')
        eventbus.zzub_pre_delete_plugin += self.on_pre_delete_plugin
        eventbus.zzub_pre_delete_pattern += self.on_pre_delete_pattern
        self._callback = zzub.zzub_callback_t(self.handle_event)
        self.set_callback(self._callback, None)
        gobject.timeout_add(int(1000/50), self.on_handle_events)
        # event queue disabling count for overlapping disable calls
        self.__disable_level = 0

    def set_callback_state(self, enable):
        #self.set_event_queue_state(enable)
        #return
        if enable:
            if self.__disable_level == 1:
                #while self.get_next_event(): pass
                self.set_callback(self._callback, None)
                #self.set_event_queue_state(True)
                self.__disable_level -= 1
                return True
            self.__disable_level -= 1
        else:
            self.set_callback(None, None)
            #self.set_event_queue_state(False)
            self.__disable_level += 1
        return False

    def is_loading(self):
        return self.__loading

    def enumerate_stream_plugins(self):
        self.__stream_ext_uri_mappings = {}
        for pluginloader in self.get_pluginloader_list():
            if is_streamer(pluginloader):
                uri = pluginloader.get_uri()
                for j in xrange(pluginloader.get_stream_format_count()):
                    ext = '.' + pluginloader.get_stream_format_ext(j)
                    if ext in self.__stream_ext_uri_mappings:
                        print >> sys.stderr, "Found another mapping for " + ext + "! Skipping " + uri
                        continue
                    self.__stream_ext_uri_mappings[ext] = uri
        print "supported sample formats: " + ', '.join(sorted(self.__stream_ext_uri_mappings.keys()))

    def delete_stream_player(self):
        if not self.__streamplayer:
            return
        self.__streamplayer.destroy()
        self.__streamplayer = None

    def set_machine_non_song(self, plugin, enable):
        pi = common.get_plugin_infos().get(plugin)
        pi.songplugin = not enable

    def delete_stream_recorder(self):
        if not self.__streamrecorder:
            return
        self.__streamrecorder.destroy()
        self.__streamrecorder = None

    def create_stream_recorder(self):
        # create a recorder plugin
        if self.__streamrecorder:
            return
        loader = self.get_pluginloader_by_name('@zzub.org/recorder/file')
        if not loader:
            print >> sys.stderr, "Can't find file recorder plugin loader."
            return
        flags = zzub.zzub_plugin_flag_no_undo | zzub.zzub_plugin_flag_no_save
        self.__streamrecorder = zzub.Player.create_plugin(self, None, 0, "_RecorderPlugin", loader, flags)
        if not self.__streamrecorder:
            print >> sys.stderr, "Can't create file recorder plugin instance."
            return
        master = self.get_plugin(0)
        self.__streamrecorder.add_input(master, zzub.zzub_connection_type_audio)
        self.set_machine_non_song(self.__streamrecorder, True)
        self.flush(None,None)
        self.history_flush_last()

    def get_stream_recorder(self):
        self.create_stream_recorder()
        return self.__streamrecorder

    def create_stream_player(self, uri):
        # create a stream player plugin and keep it out of the undo buffer
        assert not self.__streamplayer

        loader = self.get_pluginloader_by_name(uri)
        if not loader:
            print >> sys.stderr, "Can't find streamplayer plugin loader."
            return

        flags = zzub.zzub_plugin_flag_no_undo | zzub.zzub_plugin_flag_no_save
        self.__streamplayer = zzub.Player.create_plugin(self, None, 0, "_PreviewPlugin", loader, flags)
        if not self.__streamplayer:
            print >> sys.stderr, "Can't create streamplayer plugin instance."
            return
        self.get_plugin(0).add_input(self.__streamplayer, zzub.zzub_connection_type_audio)
        self.set_machine_non_song(self.__streamplayer, True)

    def preview_file(self, filepath):
        base,ext = os.path.splitext(filepath)
        if not ext.lower() in self.__stream_ext_uri_mappings:
            return False
        uri = self.__stream_ext_uri_mappings[ext.lower()]
        self.play_stream((4 << 4)+1, uri, filepath)
        return True

    def preview_wave(self, w):
        self.play_stream((4 << 4)+1, "@zzub.org/stream/wavetable;1", str(w.get_index()+1))

    def stop_preview(self):
        if self.__streamplayer:
            self.__streamplayer.play_midi_note(zzub.zzub_note_value_off, (4 << 4)+1, 0)

    def load_wave(self, wave, filepath):
        stream = zzub.Input.open_file(filepath)
        if not stream:
            return False
        res = wave.load_sample(0, 0, 0, filepath, stream)
        stream.destroy()
        if res != 0:
            self.active_waves = [wave]
        return res != 0

    def save_wave(self, wave, filepath):
        stream = zzub.Output.create_file(filepath)
        res = wave.save_sample(0, stream)
        stream.destroy()

    def play_stream(self, note, plugin_uri, data_url):
        if self.__streamplayer:
            pluginloader = self.__streamplayer.get_pluginloader()
            if pluginloader.get_uri() != plugin_uri:
                self.delete_stream_player()
        if not self.__streamplayer:
            self.create_stream_player(plugin_uri)
        if self.__streamplayer:
            self.__streamplayer.set_stream_source(data_url)
        self.flush(None,None)
        if self.__streamplayer:
            self.__streamplayer.play_midi_note(note, 0, 0)
        self.history_flush_last()

    def register_locals(self, locs):
        locs.update(dict(
                lazy_commits = self._enable_lazy_commits,
                event_stats = self._enable_event_stats,
        ))

    def _enable_event_stats(self, enable=True):
        self.__event_stats = enable

    def _enable_lazy_commits(self, enable=True):
        """not to be used outside of tests."""
        self.__lazy_commits = enable

    def on_pre_delete_pattern(self, plugin, index):
        sel = self.active_patterns
        pair = (plugin,index)
        if pair in sel:
            sel.remove(pair)
            self.active_patterns = sel

    def on_pre_delete_plugin(self, plugin):
        sel = self.active_plugins
        if plugin in sel:
            sel.remove(plugin)
            self.active_plugins = sel
        sel = self.active_patterns
        for selplugin,index in sel:
            if selplugin == plugin:
                sel.remove((selplugin,index))
        self.active_patterns = sel

    def load_bmx(self, filename):
        self.clear()
        self.__loading = True
        res = zzub.Player.load_bmx(self, filename)
        self.__loading = False
        if not res:
            self.document_path = filename
            eventbus = com.get('neil.core.eventbus')
            eventbus.document_loaded()
        return res

    def load_ccm(self, filename):
        self.clear()
        self.__loading = True
        self.set_callback_state(False)
        res = zzub.Player.load_ccm(self, filename)
        self.set_callback_state(True)
        self.__loading = False
        if not res:
            self.document_path = filename
            eventbus = com.get('neil.core.eventbus')
            eventbus.document_loaded()
        self.active_plugins = [self.get_plugin(0)]
        return res

    def save_ccm(self, filename):
        self.delete_stream_player()
        self.delete_stream_recorder()
        self.flush(None, None)
        self.history_flush_last()
        res = zzub.Player.save_ccm(self, filename)
        if not res:
            self.document_path = filename
        return res

    def clear(self):
        self.delete_stream_player()
        self.delete_stream_recorder()
        self.flush(None,None)
        self.history_flush_last()
        zzub.Player.clear(self)
        self.document_path = ''

    def on_handle_events(self):
        """
        Handler triggered by the default timer. Asks the player to fill
        the event queue and fetches events from the queue to pass them to handle_event.
        """
        if not self.__loading and not self.__lazy_commits:
            ucopcount = self.history_get_uncomitted_operations()
            if ucopcount:
                # you should commit your actions
                import neil.errordlg
                msg = "%i operation(s) left uncommitted." % ucopcount
                neil.errordlg.error(None, "<b>Internal Program Error</b>", msg)
                self.history_commit("commit leak")
        player = com.get('neil.core.player')
        t1 = time.time()
        player.handle_events()
        t2 = time.time() - t1
        self._hevtime += t2
        self._hevcalls += 1
        t = time.time()
        if self.__event_stats and ((t - self._cbtime) > 1):
            print self._hevcalls, self._cbcalls, "%.2fms" % (self._hevtime * 1000)
            self._cbcalls = 0
            self._hevcalls = 0
            self._hevtime = 0
            self._cbtime = t
        return True

    def play(self):
        self.playstarttime = time.time()
        self.set_state(zzub.zzub_player_state_playing)

    def stop(self):
        if self.get_state() != zzub.zzub_player_state_playing:
            self.set_position(0)
        else:
            self.set_state(zzub.zzub_player_state_stopped)

    def get_plugin_list(self):
        for plugin in zzub.Player.get_plugin_list(self):
            if plugin == self.__streamplayer:
                continue
            if plugin == self.__streamrecorder:
                continue
            yield plugin

    def activate_wave(self, direction):
        """
        activates wave relative to the currently activated wave. if direction
        is -1, the preceeding wave will be chosen. if direction is 1,
        the following wave will be chosen.
        """
        waves = [w for w in self.get_wave_list() if w.get_level_count() >= 1]
        if not waves:
            return
        if direction == -1:
            failsafe = -1
            offset = -1
        elif direction == 1:
            failsafe = 0
            offset = 1
        if not self.active_waves:
            self.active_waves = [waves[failsafe]]
        else:
            pindex = waves.index(self.active_waves[0])
            self.active_waves = [waves[(pindex + offset) % len(waves)]]

    def activate_pattern(self, direction):
        """
        activates pattern with the name alphabetically relative to the currently
        activated pattern name. if direction is -1, the preceeding pattern
        will be chosen. if direction is 1, the following pattern will be chosen.
        """
        if not self.get_plugin_count():
            return
        if self.active_patterns:
            plugin,index = self.active_patterns[0]
        else:
            if not self.active_plugins:
                self.activate_plugin(direction)
            plugin = self.active_plugins[0]
            index = None
        def cmp_func(a,b):
            aname = a[0].get_pattern_name(a[1])
            bname = b[0].get_pattern_name(b[1])
            return cmp(aname.lower(), bname.lower())
        patterns = sorted([(plugin,i) for i in xrange(plugin.get_pattern_count())],cmp_func)
        if not patterns:
            return
        if direction == -1:
            failsafe = -1
            offset = -1
        elif direction == 1:
            failsafe = 0
            offset = 1
        if index == None:
            self.active_patterns = [patterns[failsafe]]
        else:
            pindex = patterns.index(self.active_patterns[0])
            self.active_patterns = [patterns[(pindex+offset)%len(patterns)]]

    def activate_plugin(self, direction):
        """
        activates plugin with the name alphabetically relative to the currently
        activated plugin name. if direction is -1, the preceeding plugin
        will be chosen. if direction is 1, the following plugin will be chosen.
        """
        def cmp_func(a,b):
            return cmp(a.get_name().lower(), b.get_name().lower())
        plugins = sorted(list(self.get_plugin_list()), cmp_func)
        if not plugins:
            return
        if direction == -1:
            failsafe = -1
            offset = -1
        elif direction == 1:
            failsafe = 0
            offset = 1
        if not self.active_plugins:
            self.active_plugins = [plugins[failsafe]]
        else:
            pindex = plugins.index(self.active_plugins[0])
            self.active_plugins = [plugins[(pindex+offset)%len(plugins)]]

    def handle_event(self, player, plugin, data, tag):
        """
        Default handler for ui events sent by zzub.

        @param data: event data.
        @type data: zzub_event_data_t
        """
        eventbus = com.get('neil.core.eventbus')
        data = data.contents
        # prepare arguments for the specific callback
        eventname,membername,argnames = self.event_id_to_name[data.type]
        args = []
        if membername:
            specdata = getattr(data,membername)
            for argname in argnames:
                value = getattr(specdata, argname)
                if hasattr(value, 'contents'):
                    class_ = value.contents.__class__
                    if hasattr(class_, '_wrapper_'):
                        value = class_._wrapper_._new_from_handle(value)
                elif 'contents' in dir(value):
                    value = None
                args.append(value)
        if not data.type in self._exclude_event_debug_:
            pass
            #print "[%s](%s)" % (eventname,','.join([('%s=%r' % (a,b)) for a,b in zip(argnames,args)]))
        result = getattr(eventbus, eventname)(*args) or False
        self._cbcalls += 1
        if self.__loading:
            refresh_gui()
        return result

    def document_unchanged(self):
        self.last_history_pos = self.history_get_position()

    def can_undo(self):
        pos = self.history_get_position()
        historysize = self.history_get_size()
        return (pos > 0) and ((pos-1) < historysize)

    def can_redo(self):
        pos = self.history_get_position()
        historysize = self.history_get_size()
        return (pos < historysize)

    def document_changed(self):
        return self.last_history_pos != self.history_get_position()

    def get_track_list(self):
        """
        Returns a list of sequences
        """
        return [self.get_sequence(i) for i in xrange(self.get_sequence_track_count())]

    def get_current_sequencer(self):
        return self

    def init_lunar(self):
        """
        Initializes the lunar dsp scripting system
        """
        pc = zzub.Plugincollection.get_by_uri(self, "@zzub.org/plugincollections/lunar")

        # return if lunar is missing
        if not pc:
            print >> sys.stderr, "lunar plugin collection not found, not supporting lunar."
            return

        config = com.get('neil.core.config')
        userlunarpath = os.path.join(config.get_settings_folder(),'lunar')
        if not os.path.isdir(userlunarpath):
            print "folder %s does not exist, creating..." % userlunarpath
            os.makedirs(userlunarpath)
        pc.configure("local_storage_dir", userlunarpath)

    def solo(self, plugin):

        if not plugin or plugin == self.solo_plugin:
            # soloing deactived so apply muted states
            self.solo_plugin = None
            for plugin in self.get_plugin_list():
                info = common.get_plugin_infos().get(plugin)
                plugin.set_mute(info.muted)
                info.reset_plugingfx()
        elif is_generator(plugin):
            # mute all plugins except solo plugin
            self.solo_plugin = plugin
            for plugin in self.get_plugin_list():
                info = common.get_plugin_infos().get(plugin)
                if plugin != self.solo_plugin and is_generator(plugin):
                    plugin.set_mute(True)
                    info.reset_plugingfx()
                elif plugin == self.solo_plugin:
                    plugin.set_mute(info.muted)
                    info.reset_plugingfx()

    def toggle_mute(self, plugin):
        pi = common.get_plugin_infos().get(plugin)
        pi.muted = not pi.muted
        # make sure a machine muted by solo is not unmuted manually
        if not self.solo_plugin or plugin == self.solo_plugin or is_effect(plugin):
            plugin.set_mute(pi.muted)
        pi.reset_plugingfx()

    def create_plugin(self, pluginloader, connection=None, plugin=None):
        # find an unique name for the new plugin
        basename = pluginloader.get_short_name()
        name = pluginloader.get_short_name()
        basenumber = 2
        while True:
            found = False
            for mp in self.get_plugin_list():
                if mp.get_name() == name:
                    found = True
                    name = "%s%i" % (basename, basenumber)
                    basenumber += 1
                    break
            if not found:
                break

        # create the new plugin
        mp = zzub.Player.create_plugin(self, None, 0, name, pluginloader)
        assert mp

        active_plugins = []
        active_patterns = []

        # if it is a generator and has global or track parameters,
        # create a new default pattern.
        if is_generator(mp) and \
                (pluginloader.get_parameter_count(1) or pluginloader.get_parameter_count(2)):

            pattern = mp.create_pattern(self.sequence_step)
            pattern.set_name('00')
            mp.add_pattern(pattern)
            active_plugins = [mp]
            active_patterns = [(mp, 0)]
            t=self.create_sequence(mp, zzub.zzub_sequence_type_pattern)
            t.set_event(0,16)
            if self.autoconnect_target:
                self.autoconnect_target.add_input(mp, zzub.zzub_connection_type_audio)

        # position the plugin at the default location
        mp.set_position(*self.plugin_origin)

        ##Following code is only needed for dragging machines from
        ##search plugins context box onto target effects.

        if plugin and not self.autoconnect_target:
            if not is_generator(mp):
                # if we have a context plugin, prepend connections
                inplugs = []
                # first, record all connections
                for index in xrange(plugin.get_input_connection_count()):
                    if plugin.get_input_connection_type(index) != zzub.zzub_connection_type_audio:
                        continue
                    input = plugin.get_input_connection_plugin(index)
                    amp = plugin.get_parameter_value(zzub.zzub_parameter_group_connection, index, 0)
                    pan = plugin.get_parameter_value(zzub.zzub_parameter_group_connection, index, 1)
                    inplugs.append((input,amp,pan))
                # then, disconnect all inputs and restore to new plugin
                for inplug,amp,pan in inplugs:
                    plugin.delete_input(inplug,zzub.zzub_connection_type_audio)
                    mp.add_input(inplug,zzub.zzub_connection_type_audio)
                    index = mp.get_input_connection_count()-1
                    mp.set_parameter_value(zzub.zzub_parameter_group_connection, index, 0, amp, False)
                    mp.set_parameter_value(zzub.zzub_parameter_group_connection, index, 1, pan, False)
            plugin.add_input(mp, zzub.zzub_connection_type_audio)

        if connection:
            # if we have a context connection, replace that one
            plugin, index = connection
            if plugin.get_input_connection_type(index) == zzub.zzub_connection_type_audio:
                # note target, amp, pan
                target = plugin.get_input_connection_plugin(index)
                amp = plugin.get_parameter_value(zzub.zzub_parameter_group_connection, index, 0)
                pan = plugin.get_parameter_value(zzub.zzub_parameter_group_connection, index, 1)
                # remove connection
                plugin.delete_input(target, zzub.zzub_connection_type_audio)
                # connect our new plugin to the target and restore parameters
                mp.add_input(target, zzub.zzub_connection_type_audio)
                index = mp.get_input_connection_count()-1
                mp.set_parameter_value(zzub.zzub_parameter_group_connection, index, 0, amp, False)
                mp.set_parameter_value(zzub.zzub_parameter_group_connection, index, 1, pan, False)
                # connect the source plugin to our new plugin
                plugin.add_input(mp, zzub.zzub_connection_type_audio)

        self.history_commit("new plugin")
        if active_plugins:
            self.active_plugins = active_plugins
        if active_patterns:
            self.active_patterns = active_patterns

    def delete_plugin(self, plugin):
        # add plugin information
        common.get_plugin_infos().add_plugin(plugin)
        # remove midi mappings
        for i in range(self.get_midimapping_count()):
            mapping =  self.get_midimapping(i)
            self.remove_midimapping(plugin, mapping.get_group(), mapping.get_track(), mapping.get_column())
        inplugs = []
        outplugs = []
        # record all input connections
        for index in xrange(plugin.get_input_connection_count()):
            if plugin.get_input_connection_type(index) != zzub.zzub_connection_type_audio:
                continue
            input = plugin.get_input_connection_plugin(index)
            amp = plugin.get_parameter_value(zzub.zzub_parameter_group_connection, index, 0)
            pan = plugin.get_parameter_value(zzub.zzub_parameter_group_connection, index, 1)
            inplugs.append((input, amp, pan))
        # record all output connections
        for index in xrange(plugin.get_output_connection_count()):
            if plugin.get_output_connection_type(index) != zzub.zzub_connection_type_audio:
                continue
            output = plugin.get_output_connection_plugin(index)
            index = output.get_input_connection_by_type(plugin, zzub.zzub_connection_type_audio)
            amp = output.get_parameter_value(zzub.zzub_parameter_group_connection, index, 0)
            pan = output.get_parameter_value(zzub.zzub_parameter_group_connection, index, 1)
            outplugs.append((output, amp, pan))
        # destroy the plugin
        del common.get_plugin_infos()[plugin]
        plugin.destroy()
        # restore all connections
        for inplug, iamp, ipan in inplugs:
            for outplug, oamp, opan in outplugs:
                newamp = (iamp * oamp) / 16384
                newpan = ipan
                outplug.add_input(inplug, zzub.zzub_connection_type_audio)
                index = outplug.get_input_connection_count() - 1
                outplug.set_parameter_value(zzub.zzub_parameter_group_connection, index, 0, newamp, False)
                outplug.set_parameter_value(zzub.zzub_parameter_group_connection, index, 1, newpan, False)
        self.history_commit("delete plugin")

generate_ui_methods(NeilPlayer, DOCUMENT_UI)

__neil__ = dict(
        classes = [
                NeilPlayer,
        ],
)

if __name__ == '__main__':
    com.load_packages()
    player = com.get('neil.core.player')
    player.octave = 3
