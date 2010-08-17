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
# Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA  02110-1301, USA.

"""
This module contains the context menu component for different zzub objects
such as plugins, patterns, and so on. based on the context object currently
selected, items can choose to append themselves or not.
"""

import gtk
import neil.common as common
from neil.com import com
import zzub
import os.path

from neil.utils import is_generator, is_root, is_controller, is_effect, \
        prepstr, Menu, new_theme_image, gettext
from neil.utils import PLUGIN_FLAGS_MASK, ROOT_PLUGIN_FLAGS, \
        GENERATOR_PLUGIN_FLAGS, EFFECT_PLUGIN_FLAGS, CONTROLLER_PLUGIN_FLAGS
from neil.utils import iconpath

class ContextMenu(Menu):
    __neil__ = dict(
            id = 'neil.core.contextmenu',
            singleton = False,
            categories = [
            ],
    )

    def __init__(self, contextid, context):
        Menu.__init__(self)
        self.__context_id = contextid
        self.__context = context

    def get_context(self):
        return self.__context

    def get_context_id(self):
        return self.__context_id

    context = property(get_context)
    context_id = property(get_context_id)

    def add_submenu(self, label, submenu=None):
        if not submenu:
            submenu = ContextMenu(self.__context_id, self.__context)
        return Menu.add_submenu(self, label, submenu)

    def popup(self, parent, event=None):
        for item in self.get_children():
            item.destroy()
        for item in com.get_from_category('contextmenu.handler'):
            item.populate_contextmenu(self)
        self.rootwindow = parent
        return Menu.popup(self, parent, event)

class PluginContextMenu(gtk.Menu):
    __neil__ = dict(id='neil.core.popupmenu',
                      singleton=True,
                      categories=['contextmenu.handler'])

    plugin_tree = {'@neil/lunar/generator/ArguruSynth2;1' :
                       ['Synthesizers', 'Subtractive'],
                   '@krzysztof_foltman/generator/infector;1' :
                       ['Synthesizers', 'Subtractive'],
                   'jamesmichaelmcdermott@gmail.com/generator/primifun;1' :
                       ['Synthesizers', 'Subtractive'],
                   '@cameron_foale/generator/green_milk;1' :
                       ['Synthesizers', 'Subtractive'],
                   '@makk.org/M4wII;1' :
                       ['Synthesizers', 'Subtractive'],
                   'jamesmichaelmcdermott@gmail.com/generator/4fm2f;1' :
                       ['Synthesizers', 'FM'],
                   '@libneil/somono/generator/fm303;1' :
                       ['Synthesizers', 'FM'],
                   'jamesmichaelmcdermott@gmail.com/generator/pluckedstring;1' :
                       ['Synthesizers', 'Physical Modelling'],
                   'jamesmichaelmcdermott@gmail.com/generator/dynamite6;1' :
                       ['Synthesizers', 'Physical Modelling'],
                   '@mda-vst/epiano;1' :
                       ['Synthesizers', 'Physical Modelling'],
                   '@libneil/fsm/generator/kick_xp' :
                       ['Synthesizers', 'Percussive'],
                   '@libneil/somono/generator/cloud;1' :
                       ['Synthesizers', 'Granular'],
                   '@rift.dk/generator/Matilde+Tracker;1.5' :
                       ['Samplers'],
                   '@trac.zeitherrschaft.org/aldrin/lunar/effect/delay;1' :
                       ['Effects', 'Time based'],
                   '@trac.zeitherrschaft.org/aldrin/lunar/effect/phaser;1' :
                       ['Effects', 'Time based'],
                   '@trac.zeitherrschaft.org/aldrin/lunar/effect/reverb;1' :
                       ['Effects', 'Time based'],
                   '@mda/effect/mdaThruZero;1' :
                       ['Effects', 'Time based'],
                   '@libneil/somono/effect/chebyshev;1' :
                       ['Effects', 'Distortion'],
                   '@libneil/arguru/effect/distortion' :
                       ['Effects', 'Distortion'],
                   'graue@oceanbase.org/effect/softsat;1' :
                       ['Effects', 'Distortion'],
                   '@mda/effect/mdaBandisto;1' :
                       ['Effects', 'Distortion'],
                   '@neil/lunar/effect/bitcrusher;1' :
                       ['Effects', 'Distortion'],
                   '@bblunars/effect/mdaDegrade' :
                       ['Effects', 'Distortion'],
                   '@bigyo/frequency+shifter;1' :
                       ['Effects', 'Modulation'],
                   'jamesmichaelmcdermott@gmail.com/effect/btdsys_ringmod;1' :
                       ['Effects', 'Modulation'],
                   'jamesmichaelmcdermott@gmail.com/effect/modulator;1' :
                       ['Effects', 'Modulation'],
                   '@libneil/arguru/effect/compressor' :
                       ['Effects', 'Dynamics'],
                   '@binarywerks.dk/multi-2;1' :
                       ['Effects', 'Dynamics'],
                   '@FireSledge.org/ParamEQ;1' :
                       ['Effects', 'Filter'],
                   '@trac.zeitherrschaft.org/aldrin/lunar/effect/philthy;1' :
                       ['Effects', 'Filter'],
                   'jamesmichaelmcdermott@gmail.com/effect/dffilter;1' :
                       ['Effects', 'Filter'],
                   '@libneil/somono/effect/filter' :
                       ['Effects', 'Filter'],
                   '@libneil/somono/controller/lfnoise;1' :
                       ['Control'],
                   '@neil/lunar/controller/Controller;1' :
                       ['Control'],
                   '@trac.zeitherrschaft.org/aldrin/lunar/controller/LunarLFO;1' :
                       ['Control']}

    def populate_contextmenu(self, menu):
        if menu.context_id == 'plugin':
            self.populate_pluginmenu(menu)
        elif menu.context_id == 'connection':
            self.populate_connectionmenu(menu)
        elif menu.context_id == 'router':
            self.populate_routermenu(menu)

    def create_add_machine_submenu(self, menu, connection=False):
        def get_icon_name(pluginloader):
            uri = pluginloader.get_uri()
            #if uri.startswith('@zzub.org/dssidapter/'):
            #    return iconpath("scalable/dssi.svg")
            #if uri.startswith('@zzub.org/ladspadapter/'):
            #    return iconpath("scalable/ladspa.svg")
            #if uri.startswith('@psycle.sourceforge.net/'):
            #    return iconpath("scalable/psycle.svg")
            filename = pluginloader.get_name()
            filename = filename.strip().lower()
            for c in '():[]/,.!"\'$%&\\=?*#~+-<>`@ ':
                filename = filename.replace(c, '_')
            while '__' in filename:
                filename = filename.replace('__','_')
            filename = filename.strip('_')
            return "%s.svg" % iconpath("scalable/" + filename)
        def add_path(tree, path, loader):
            if len(path) == 1:
                tree[path[0]] = loader
                return tree
            elif path[0] not in tree:
                tree[path[0]] = add_path({}, path[1:], loader)
                return tree
            else:
                tree[path[0]] = add_path(tree[path[0]], path[1:], loader)
                return tree
        def populate_from_tree(menu, tree):
            for key, value in tree.iteritems():
                if type(value) is not type({}):
                    icon = gtk.Image()
                    filename = get_icon_name(value)
                    if os.path.isfile(filename):
                        icon.set_from_file(get_icon_name(value))
                    item = gtk.ImageMenuItem(prepstr(key, fix_underscore=True))
                    item.set_image(icon)
                    item.connect('activate', create_plugin, value, connection)
                    menu.add(item)
                else:
                    item, submenu = menu.add_submenu(key)
                    populate_from_tree(submenu, value)
        def create_plugin(item, loader, connection=False):
            player = com.get('neil.core.player')
            if connection:
                player.create_plugin(loader, connection=menu.context)
            else:
                player.plugin_origin = menu.context
                player.create_plugin(loader)
        player = com.get('neil.core.player')
        plugins = {}
        tree = {}
        item, add_machine_menu = menu.add_submenu("Add machine")
        for pluginloader in player.get_pluginloader_list():
            plugins[pluginloader.get_uri()] = pluginloader
        for uri, loader in plugins.iteritems():
            try:
                path = self.plugin_tree[uri]
                if connection and path[0] != "Effects":
					continue
                path = path + [loader.get_name()]
                tree = add_path(tree, path, loader)
            except KeyError:
                pass
        populate_from_tree(add_machine_menu, tree)

    def populate_routermenu(self, menu):
        self.create_add_machine_submenu(menu)
        menu.add_separator()
        menu.add_item("Unmute All", self.on_popup_unmute_all)

    def populate_connectionmenu(self, menu):
        mp, index = menu.context
        conntype = mp.get_input_connection_type(index)
        if conntype == zzub.zzub_connection_type_audio:
            self.create_add_machine_submenu(menu, connection=True)
            menu.add_separator()
        menu.add_item("Disconnect plugins", self.on_popup_disconnect, mp, index)
        if conntype == zzub.zzub_connection_type_event:
            # Connection connects a control plug-in to it's destination.
            # menu.add_separator()
            mi = mp.get_input_connection_plugin(index).get_pluginloader()
            for i in range(mi.get_parameter_count(3)):
                param = mi.get_parameter(3, i)
                print param.get_name()

    def populate_pluginmenu(self, menu):
        mp = menu.context
        player = com.get('neil.core.player')
        menu.add_check_item("_Mute", common.get_plugin_infos().get(mp).muted,
                            self.on_popup_mute, mp)
        if is_generator(mp):
            menu.add_check_item("_Solo", player.solo_plugin == mp,
                                self.on_popup_solo, mp)
        menu.add_separator()
        menu.add_item("_Parameters...", self.on_popup_show_params, mp)
        menu.add_item("_Attributes...", self.on_popup_show_attribs, mp)
        menu.add_item("P_resets...", self.on_popup_show_presets, mp)
        menu.add_separator()
        menu.add_item("_Rename...", self.on_popup_rename, mp)
        if not is_root(mp):
            menu.add_item("_Delete plugin", self.on_popup_delete, mp)
        if is_effect(mp) or is_root(mp):
            menu.add_separator()
            menu.add_check_item("Default Target",
                                player.autoconnect_target == mp,
                                self.on_popup_set_target, mp)
        commands = mp.get_commands().split('\n')
        if commands != ['']:
            menu.add_separator()
            submenuindex = 0
            for index in range(len(commands)):
                cmd = commands[index]
                if cmd.startswith('/'):
                    item, submenu = menu.add_submenu(prepstr(cmd[1:], fix_underscore=True))
                    subcommands = mp.get_sub_commands(index).split('\n')
                    submenuindex += 1
                    for subindex in range(len(subcommands)):
                        subcmd = subcommands[subindex]
                        submenu.add_item(prepstr(subcmd, fix_underscore=True),
                                         self.on_popup_command, mp,
                                         submenuindex, subindex)
                else:
                    menu.add_item(prepstr(cmd), self.on_popup_command,
                                  mp, 0, index)

    def on_popup_rename(self, widget, mp):
        text = gettext(self, "Enter new plugin name:", prepstr(mp.get_name()))
        if text:
            player = com.get('neil.core.player')
            mp.set_name(text)
            player.history_commit("rename plugin")

    def on_popup_solo(self, widget, mp):
        """
        Event handler for the "Mute" context menu option.

        @param event: Menu event.
        @type event: wx.MenuEvent
        """
        player = com.get('neil.core.player')
        if player.solo_plugin != mp:
            player.solo(mp)
        else:
            player.solo(None)

    def on_popup_mute(self, widget, mp):
        """
        Event handler for the "Mute" context menu option.

        @param event: Menu event.
        @type event: wx.MenuEvent
        """
        player = com.get('neil.core.player')
        player.toggle_mute(mp)

    def on_popup_delete(self, widget, mp):
        """
        Event handler for the "Delete" context menu option.
        """
        player = com.get('neil.core.player')
        player.delete_plugin(mp)

    def on_popup_disconnect(self, widget, mp, index):
        """
        Event handler for the "Disconnect" context menu option.

        @param event: Menu event.
        @type event: wx.MenuEvent
        """
        plugin = mp.get_input_connection_plugin(index)
        conntype = mp.get_input_connection_type(index)
        mp.delete_input(plugin,conntype)
        player = com.get('neil.core.player')
        player.history_commit("disconnect")

    def on_popup_show_attribs(self, widget, mp):
        """
        Event handler for the "Attributes..." context menu option.

        @param event: Menu event.
        @type event: wx.MenuEvent
        """
        dlg = com.get('neil.core.attributesdialog',mp,self)
        dlg.run()
        dlg.destroy()


    def on_popup_show_presets(self, widget, plugin):
        """
        Event handler for the "Presets..." context menu option.

        @param event: Menu event.
        @type event: wx.MenuEvent
        """
        manager = com.get('neil.core.presetdialog.manager')
        manager.show(plugin, widget)

    def on_popup_show_params(self, widget, mp):
        """
        Event handler for the "Parameters..." context menu option.

        @param event: Menu event.
        @type event: wx.MenuEvent
        """
        manager = com.get('neil.core.parameterdialog.manager')
        manager.show(mp, widget)

    def on_popup_new_plugin(self, widget, pluginloader, kargs={}):
        """
        Event handler for "new plugin" context menu options.
        """
        player = com.get('neil.core.player')
        if 'conn' in kargs:
            conn = kargs['conn']
        else:
            conn = None
        if 'plugin' in kargs:
            plugin = kargs['plugin']
        else:
            plugin = None
        player.create_plugin(pluginloader, connection=conn, plugin=plugin)

    def on_popup_unmute_all(self, widget):
        """
        Event handler for unmute all menu option
        """
        player = com.get('neil.core.player')
        for mp in reversed(list(player.get_plugin_list())):
            info = common.get_plugin_infos().get(mp)
            info.muted=False
            mp.set_mute(info.muted)
            info.reset_plugingfx()

    def on_popup_command(self, widget, plugin, subindex, index):
        """
        Event handler for plugin commands
        """
        plugin.command((subindex<<8) | index)

    def on_popup_set_target(self, widget, plugin):
        """
        Event handler for menu option to set machine as target for default connection
        """
        player = com.get('neil.core.player')
        if player.autoconnect_target==plugin:
            player.autoconnect_target = None
        else:
            player.autoconnect_target = plugin

__neil__ = dict(
        classes = [
                ContextMenu,
                PluginContextMenu,
        ],
)

