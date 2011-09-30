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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, 
# USA.

"""
Contains all classes and functions needed to render the pattern
editor and its associated dialogs.
"""

if __name__ == '__main__':
    import os
    os.system('../../bin/neil-combrowser neil.core.patternpanel')
    raise SystemExit

import neil.com as com
import os
import gtk
import gobject
import pango
import itertools

from neil.utils import prepstr, filepath, get_item_count
from neil.utils import get_clipboard_text, set_clipboard_text, question
from neil.utils import error, get_new_pattern_name, new_liststore
from neil.utils import new_combobox, db2linear, make_menu_item
from neil.utils import make_check_item, ObjectHandlerGroup, AcceleratorMap
from neil.utils import Menu, padded_partition, is_generator, is_effect
from neil.utils import new_stock_image_button, show_machine_manual
from neil.utils import filenameify

import zzub
import time
import random
import neil.common as common

MARGIN = common.MARGIN
MARGIN2 = common.MARGIN2
MARGIN3 = common.MARGIN3
MARGIN0 = common.MARGIN0

from neil.utils import NOTES, roundint
PATLEFTMARGIN = 48
CONN = 0
GLOBAL = 1
TRACK = 2

patternsizes = [
    1, 4, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 512
]

class PatternDialog(gtk.Dialog):
    """
    Pattern Dialog Box.

    This dialog is used to create a new pattern or a copy of a pattern,
    and to modify existent patterns.
    """
    def __init__(self, parent):
        """
        Initialization.
        """
        gtk.Dialog.__init__(self,
                "Pattern Properties",
                parent.get_toplevel(),
                gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT,
                None
        )
        vbox = gtk.VBox(False, MARGIN)
        vbox.set_border_width(MARGIN)
        self.btnok = self.add_button(gtk.STOCK_OK, gtk.RESPONSE_OK)
        self.btncancel = self.add_button(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL)
        self.namelabel = gtk.Label("Name")
        self.edtname = gtk.Entry()
        self.lengthlabel = gtk.Label("Length")
        self.lengthbox = gtk.combo_box_entry_new_text()
        self.chkswitch = gtk.CheckButton('Switch to new pattern')
        for size in patternsizes:
            self.lengthbox.append_text(str(size))
        self.rowslabel = gtk.Label("Rows")
        sgroup1 = gtk.SizeGroup(gtk.SIZE_GROUP_HORIZONTAL)
        sgroup2 = gtk.SizeGroup(gtk.SIZE_GROUP_HORIZONTAL)
        def add_row(c1, c2):
            row = gtk.HBox(False, MARGIN)
            c1.set_alignment(1, 0.5)
            row.pack_start(c1, expand=False)
            row.pack_start(c2)
            sgroup1.add_widget(c1)
            sgroup2.add_widget(c2)
            vbox.pack_start(row, expand=False)
        add_row(self.namelabel, self.edtname)
        add_row(self.rowslabel, self.lengthbox)
        vbox.pack_start(self.chkswitch, expand=False)
        self.edtname.connect('activate', self.on_enter)
        self.lengthbox.child.connect('activate', self.on_enter)
        self.vbox.add(vbox)
        self.show_all()

    def on_enter(self, widget):
        self.response(gtk.RESPONSE_OK)

# pattern dialog modes
DLGMODE_NEW = 0
DLGMODE_COPY = 1
DLGMODE_CHANGE = 2

def show_pattern_dialog(parent, name, length, dlgmode, letswitch=True):
    """
    Shows the pattern creation/modification dialog.

    @param parent: Parent container
    @type parent: gtk.Widget
    @param name: Pattern name
    @type name: string
    @param length: Pattern name
    @type length: int
    @param dlgmode: Dialog mode (DLGMODE_NEW: create new pattern,
    DLGMODE_COPY: create copy of pattern, DLGMODE_CHANGE: modify pattern)
    @type dlgmode: int
    @return: Tuple containing pattern name, length, and whether to switch to new pattern or not
    @rtype: (string, int, int)
    """
    dlg = PatternDialog(parent)
    dlg.dlgmode = dlgmode
    if dlgmode == DLGMODE_NEW:
        dlg.set_title("New Pattern")
        if not letswitch:
            dlg.chkswitch.set_sensitive(False)
    elif dlgmode == DLGMODE_COPY:
        dlg.set_title("Create copy of pattern")
        dlg.lengthbox.set_sensitive(False)
    elif dlgmode == DLGMODE_CHANGE:
        dlg.set_title("Pattern Properties")
        dlg.chkswitch.set_sensitive(False)
    dlg.edtname.set_text(name)
    dlg.chkswitch.set_active(config.get_config().get_default_int('SwitchToNewPattern', 1))
    dlg.lengthbox.child.set_text(str(length))
    dlg.edtname.select_region(0, -1)
    dlg.edtname.grab_focus()
    response = dlg.run()
    dlg.hide_all()
    result = None
    if response == gtk.RESPONSE_OK:
        switch = int(dlg.chkswitch.get_active())
        config.get_config().set_default_int('SwitchToNewPattern', switch)
        length = int(dlg.lengthbox.child.get_text())
        result = str(dlg.edtname.get_text()), length, switch
    dlg.destroy()
    return result

class PatternToolBar(gtk.HBox):
    """
    Pattern Toolbar

    Contains lists of the plugins, patterns, waves and octaves available.
    """

    def __init__(self, pattern_view):
        """
        Initialization.
        """
        player = com.get('neil.core.player')
        gtk.HBox.__init__(self, False, MARGIN)
        self.pattern_view = pattern_view
        self.set_border_width(MARGIN)
        eventbus = com.get('neil.core.eventbus')

        self.pluginselect = gtk.combo_box_new_text()
        self.pluginselect.set_size_request(100, 0)
        self.pluginselect_handler =\
            self.pluginselect.connect('changed', self.set_plugin_sel)
        self.pluginselect.set_tooltip_text("Machine to edit a pattern for")
        eventbus.zzub_new_plugin += self.pluginselect_update
        eventbus.zzub_delete_plugin += self.pluginselect_update
        eventbus.document_loaded += self.pluginselect_update
        eventbus.active_plugins_changed += self.pluginselect_update

        self.patternlabel = gtk.Label()
        self.patternlabel.set_text_with_mnemonic("_Patt")
        self.patternselect = gtk.combo_box_new_text()
        self.patternselect.set_tooltip_text("The pattern to edit")
        self.patternselect.set_size_request(100, 0)
        self.patternselect_handler =\
            self.patternselect.connect('changed', self.set_pattern_sel)
        eventbus.active_plugins_changed += self.get_pattern_source
        eventbus.active_patterns_changed += self.get_pattern_source
        eventbus.zzub_delete_pattern += self.get_pattern_source
        eventbus.zzub_new_pattern += self.get_pattern_source
        eventbus.zzub_pattern_changed += self.get_pattern_source
        self.patternlabel.set_mnemonic_widget(self.patternselect)

        # Wave selector combo box.
        self.wavelabel = gtk.Label()
        self.wavelabel.set_text_with_mnemonic("_Wave")
        self.waveselect = gtk.combo_box_new_text()
        self.waveselect.set_tooltip_text("Which wave to use")
        self.waveselect.set_size_request(100, 0)
        self.waveselect_handler =\
            self.waveselect.connect('changed', self.set_wave_sel)
        eventbus.active_waves_changed += self.waveselect_update
        eventbus.zzub_wave_allocated += self.waveselect_update
        eventbus.zzub_wave_changed += self.waveselect_update
        eventbus.zzub_delete_wave += self.waveselect_update
        eventbus.document_loaded += self.waveselect_update
        self.wavelabel.set_mnemonic_widget(self.waveselect)

        # An octave selector combo box.
        self.octavelabel = gtk.Label()
        self.octavelabel.set_text_with_mnemonic("_Oct")
        self.octaveselect = gtk.combo_box_new_text()
        self.octaveselect.set_tooltip_text("Choose which octave you can enter notes from")
        self.octavelabel.set_mnemonic_widget(self.octaveselect)
        for octave in range(1, 9):
            self.octaveselect.append_text(str(octave))
        self.octaveselect.set_active(player.octave - 1)
        def octave_set(event):
            player.octave = int(self.octaveselect.get_active_text())
            self.pattern_view.grab_focus()
        self.octaveselect.connect('changed', octave_set)
        eventbus.octave_changed += self.octave_update
        
        # An edit step selector combo box.
        self.edit_step_label = gtk.Label()
        self.edit_step_label.set_text_with_mnemonic("_Step")
        self.edit_step_box = gtk.combo_box_new_text()
        self.edit_step_box.set_tooltip_text("Set how many rows the cursor will jump when editting")
        for step in range(12):
            self.edit_step_box.append_text(str(step + 1))
        self.edit_step_box.set_active(0)
        self.edit_step_box.connect('changed', self.edit_step_changed)

        self.playnotes = gtk.CheckButton(label="_Play")
        self.playnotes.set_active(True)
        self.playnotes.set_tooltip_text("If checked, the notes will be played as you enter them in the editor")
        self.playnotes.connect('clicked', self.on_playnotes_click)

        self.btnhelp = new_stock_image_button(gtk.STOCK_HELP)
	self.btnhelp.set_tooltip_text("Machine help page")
        self.btnhelp.connect('clicked', self.on_button_help)

        self.pack_start(self.pluginselect, expand=False)
        self.pack_start(self.patternselect, expand=False)
        self.pack_start(self.waveselect, expand=False)
        self.pack_start(gtk.VSeparator(), expand=False)
        self.pack_start(self.octavelabel, expand=False)
        self.pack_start(self.octaveselect, expand=False)
        self.pack_start(self.edit_step_label, expand=False)
        self.pack_start(self.edit_step_box, expand=False)
        self.pack_start(self.playnotes, expand=False)
        self.pack_start(gtk.VSeparator(), expand=False)
        self.pack_start(self.btnhelp, expand=False)

    def on_button_help(self, *args):
        player = com.get('neil.core.player')
        if len(player.active_plugins) < 1:
            return
        name = filenameify(player.active_plugins[0].get_pluginloader().get_name())
	if not show_machine_manual(name):
	    info = gtk.MessageDialog(self.get_toplevel(), flags=0, type=gtk.MESSAGE_INFO, buttons=gtk.BUTTONS_OK, message_format="Sorry, there's no help for this plugin yet")
	    info.run()
	    info.destroy()

    def pluginselect_update(self, *args):
        player = com.get('neil.core.player')
        self.pluginselect.handler_block(self.pluginselect_handler)
        plugins = self.get_plugin_source()
        active = -1
        if player.active_plugins != []:
            for plugin, i in zip(plugins, range(len(plugins))):
                if plugin[1] == player.active_plugins[0]:
                    active = i
        model = self.pluginselect.get_model()
        model.clear()
        for plugin in plugins:
            self.pluginselect.append_text(plugin[0])
        if active != -1:
            self.pluginselect.set_active(active)
        self.pluginselect.handler_unblock(self.pluginselect_handler)

    def octave_update(self, *args):
        """
        This function is called when the current octave for entering
        notes is changed somewhere.
        """
        player = com.get('neil.core.player')
        self.octaveselect.set_active(player.octave - 1)

    def waveselect_update(self, *args):
        """
        This function is called whenever it is decided that
        the wave list in the combox box is outdated.
        """
        self.waveselect.handler_block(self.waveselect_handler)
        player = com.get('neil.core.player')
        sel = player.active_waves
        active = self.waveselect.get_active()
        model = self.waveselect.get_model()
        model.clear()
        for i in range(player.get_wave_count()):
            w = player.get_wave(i)
            if w.get_level_count() >= 1:
                self.waveselect.append_text("%02X. %s" % (i + 1, w.get_name()))
        if sel != []:
            index = sel[0].get_index()
            self.waveselect.set_active(index)
        else:
            self.waveselect.set_active(0)
        self.waveselect.handler_unblock(self.waveselect_handler)

    def edit_step_changed(self, event):
        step = int(self.edit_step_box.get_active_text())
        self.pattern_view.edit_step = step
        self.pattern_view.grab_focus()

    def on_playnotes_click(self, event):
        self.pattern_view.play_notes = self.playnotes.get_active()
        self.pattern_view.grab_focus()

    def get_plugin_source(self):
        player = com.get('neil.core.player')
        def cmp_func(a, b):
            return cmp(a.get_name().lower(), b.get_name().lower())
        plugins = sorted(list(player.get_plugin_list()), cmp_func)
        return [(plugin.get_name(), plugin) for plugin in plugins]

    def get_plugin_sel(self):
        player = com.get('neil.core.player')
        sel = player.active_plugins
        sel = sel and sel[0] or None
        return sel

    def set_plugin_sel(self, *args):
        sel_index = self.pluginselect.get_active()
        plugins = self.get_plugin_source()
        sel = plugins[sel_index][1]
        if sel:
            player = com.get('neil.core.player')
            player.active_plugins = [sel]
            if sel.get_pattern_count() > 0:
                player.active_patterns = [(sel, 0)]
            else:
                player.active_patterns = []
        self.pattern_view.grab_focus()

    def get_pattern_source(self, *args):
        print "get_pattern_source()"
        player = com.get('neil.core.player')
        plugin = self.get_plugin_sel()
        print plugin
        print player.active_patterns
        if not plugin:
            self.patternselect.get_model().clear()
            return
        #def cmp_func(a,b):
        #    aname = a[0].get_pattern_name(a[1])
        #    bname = b[0].get_pattern_name(b[1])
        #    return cmp(aname.lower(), bname.lower())
        #patterns = sorted([(plugin, i) for i in 
        #                   xrange(plugin.get_pattern_count())], cmp_func)
        self.patternselect.get_model().clear()
        names = [(i, plugin.get_pattern_name(i)) for i in range(plugin.get_pattern_count())]
        for i, name in names:
            self.patternselect.append_text("%d %s" % (i, name))
        # Block signal handler to avoid infinite recursion.
        self.patternselect.handler_block(self.patternselect_handler)
        if len(player.active_patterns) > 0:
            self.patternselect.set_active(player.active_patterns[0][1])
        self.patternselect.handler_unblock(self.patternselect_handler)
        
    def get_pattern_sel(self):
        player = com.get('neil.core.player')
        sel = player.active_patterns
        return sel and sel[0] or None

    def set_pattern_sel(self, sel):
        player = com.get('neil.core.player')
        try:
            sel = (player.active_plugins[0], 
                   self.patternselect.get_active())
        except IndexError:
            return
        if sel[1] >= 0:
            player.active_patterns = [sel]
        self.pattern_view.grab_focus()

    def set_wave_sel(self, *args):
        player = com.get('neil.core.player')
        sel = player.get_wave(self.waveselect.get_active())
        if sel:
            player = com.get('neil.core.player')
            player.active_waves = [sel]
        self.pattern_view.grab_focus()

    def activate_wave(self, w):
        player = com.get('neil.core.player')
        if w and w.get_level_count() >= 1:
            player.preview_wave(w)
        else:
            player.stop_preview()

class PatternPanel(gtk.VBox):
    """
    Panel containing the pattern toolbar and pattern view.
    """
    __neil__ = dict(
            id = 'neil.core.patternpanel',
            singleton = True,
            categories = [
                    'neil.viewpanel',
                    'view',
            ]
    )

    __view__ = dict(
                    label = "Patterns",
                    stockid = "neil_pattern",
                    shortcut = 'F2',
                    order = 2,
    )

    def __init__(self):
        """
        Initialization.
        """
        gtk.VBox.__init__(self)
        self.statusbar = gtk.HBox(False, MARGIN)
        self.statusbar.set_border_width(MARGIN0)
        vscroll = gtk.VScrollbar()
        hscroll = gtk.HScrollbar()
        self.statuslabels = []

        label = gtk.Label()
        self.statuslabels.append(label)
        self.statusbar.pack_start(label, expand=False)
        self.statusbar.pack_start(gtk.VSeparator(), expand=False)

        label = gtk.Label()
        self.statuslabels.append(label)
        self.statusbar.pack_start(label, expand=False)
        self.statusbar.pack_start(gtk.VSeparator(), expand=False)

        label = gtk.Label()
        self.statuslabels.append(label)
        self.statusbar.pack_start(label, expand=False)
        self.statusbar.pack_start(gtk.VSeparator(), expand=False)

        label = gtk.Label()
        self.statuslabels.append(label)
        self.statusbar.pack_end(label, expand=False)

        self.view = PatternView(self, hscroll, vscroll)
        self.viewport = gtk.Viewport()
        self.viewport.add(self.view)
        self.toolbar = PatternToolBar(self.view)
        self.view.statusbar = self.statusbar
        self.pack_start(self.toolbar, expand=False)
        scrollwin = gtk.Table(2,2)
        scrollwin.attach(self.viewport, 0, 1, 0, 1, gtk.FILL|gtk.EXPAND, gtk.FILL|gtk.EXPAND)
        scrollwin.attach(vscroll, 1, 2, 0, 1, 0, gtk.FILL)
        scrollwin.attach(hscroll, 0, 1, 1, 2, gtk.FILL, 0)
        self.pack_start(scrollwin)
        self.pack_end(self.statusbar, expand=False)

        self.view.grab_focus()
        eventbus = com.get('neil.core.eventbus')
        eventbus.edit_pattern_request += self.on_edit_pattern_request

    def on_edit_pattern_request(self, plugin, index):
        player = com.get('neil.core.player')
        player.active_plugins = [plugin]
        player.active_patterns = [(plugin, index)]
        framepanel = com.get('neil.core.framepanel')
        framepanel.select_viewpanel(self)

    def handle_focus(self):
        player = com.get('neil.core.player')
        #print 'pattern', self.view.plugin, self.view.pattern
        #print 'active', player.active_patterns
        # check if active patterns match the pattern view settings
        if not (self.view.plugin, self.view.pattern) in player.active_patterns:
            if player.active_plugins:
                plugin = player.active_plugins[0]
                # make sure there is a pattern of selected machine to edit
                if plugin.get_pattern_count() > 0:
                    player.active_patterns = [(plugin, 0)]
                else:
                    player.active_patterns = []
                self.view.plugin, self.view.pattern  = plugin, 0
            self.view.init_values()
        try:
            self.view.show_cursor_right()
        except AttributeError: #no pattern in current machine
            pass
        self.view.needfocus = True

from neil.utils import fixbn, bn2mn, mn2bn, note2str, switch2str, byte2str, word2str

t2c = [
        note2str,
        switch2str,
        byte2str,
        word2str,
]
t2w = [3,1,2,4]
t2si = [2, 1, 2, 4]
t2siofs = [[0,2], [0], [0,1], [0,1,2,3]]


import config

def key_to_note(k):
    """
    uses the active keymap to determine note and
    octave from a pressed key.

    @param k: Pressed key.
    @type k: int
    @return: a tuple consisting of octave and note or None.
    @rtype: (int,int)
    """
    keymap = config.get_config().get_keymap()
    rows = keymap.split('|')
    if k<128:
        k = chr(k).lower().upper()
    else:
        k = chr(k)
    for row,index in zip(rows,range(len(rows))):
        if k in row:
            note = row.index(k)
            return index+(note/12), note%12
    return None

sc2note = {
        90:(0,0),
        83:(0,1),
        88:(0,2),
        68:(0,3),
        67:(0,4),
        86:(0,5),
        71:(0,6),
        66:(0,7),
        72:(0,8),
        78:(0,9),
        74:(0,10),
        77:(0,11),
        44:(1,0),
        81:(1,0),
        50:(1,1),
        87:(1,2),
        51:(1,3),
        69:(1,4),
        82:(1,5),
        53:(1,6),
        84:(1,7),
        54:(1,8),
        89:(1,9),
        55:(1,10),
        85:(1,11),
        73:(2,0),
        57:(2,1),
        79:(2,2),
        48:(2,3),
        80:(2,4),
}

def get_str_from_param(p,v):
    """
    Extracts a string representation from value in context of a parameter.

    @param p: Parameter.
    @type p: zzub.Parameter
    @param v: Value.
    @type v: int
    """
    return t2c[p.get_type()](p,v)

def get_length_from_param(p):
    """
    Gets length of a parameter.

    @param p: Parameter.
    @type p: zzub.Parameter
    """
    return t2w[p.get_type()]

def get_subindexcount_from_param(p):
    """
    Gets subindex count of a parameter.

    @param p: Parameter.
    @type p: zzub.Parameter
    """
    return t2si[p.get_type()]

def get_subindexoffsets_from_param(p):
    return t2siofs[p.get_type()]

# selection modes: column, track, tracks, all
SEL_COLUMN = 0
SEL_TRACK = 1
SEL_GROUP = 2
SEL_ALL = 3
SEL_COUNT = 4

class PatternView(gtk.DrawingArea):
    """
    Pattern viewer class.
    """
    CLIPBOARD_MAGIC = "PATTERNDATA"

    class Selection:
        """
        Selection class.

        Container for selection range and the selection mode.
        """
        begin = -1
        end = -1
        group = 0
        track = 0
        index = 0
        mode = SEL_COLUMN

    def __init__(self, panel, hscroll, vscroll):
        """
        Initialization.
        """
        self.edit_step = 1
        self.statuslabels = panel.statuslabels
        self.panel = panel
        self.needfocus = True
        self.hscroll = hscroll
        self.vscroll = vscroll
        self.statusbar = None
        self.jump_to_note = False
        self.patternsize = 16
        self.index = None
        self.pattern = -1
        self.plugin = None
        self.row = 0
        self.index = 0
        self.track = 0
        self.group = 0
        self.subindex = 0
        self.start_row = 0
        self.selection = None
        self.playpos = 0
        self.keystartselect = None
        self.keyendselect = None
        self.selection_start = None
        self.dragging = False
        self.shiftselect = None
        self.clickpos = None
        self.track_width = [0, 0, 0]
        self.plugin_info = common.get_plugin_infos()
        self.factors = None
        self.play_notes = True
        self.current_plugin = ""
        gtk.DrawingArea.__init__(self)
        # "Bitstream Vera Sans Mono"
        self.update_font()
        # implements horizontal scrolling
        self.start_col = 0
        self.add_events(gtk.gdk.ALL_EVENTS_MASK)
        self.set_property('can-focus', True)

        self.accel_map = AcceleratorMap()
        self.accel_map.add_accelerator('<Control><Shift>Return',
                                       self.on_popup_create_copy)
        self.accel_map.add_accelerator('<Shift>ISO_Left_Tab', self.tab_left)
        self.accel_map.add_accelerator('Tab', self.tab_right)
        self.accel_map.add_accelerator('<Control>Return',
                                       self.on_popup_create_pattern)
        self.accel_map.add_accelerator('<Control>BackSpace',
                                       self.on_popup_properties)
        self.accel_map.add_accelerator('<Control>Delete',
                                       self.on_popup_remove_pattern)
        self.accel_map.add_accelerator('<Shift>KP_Add',
                                       self.transpose_selection, None, 1)
        self.accel_map.add_accelerator('<Shift>asterisk',
                                       self.transpose_selection, None, 1)
        self.accel_map.add_accelerator('<Shift>plus',
                                       self.transpose_selection, None, 1)
        self.accel_map.add_accelerator('<Shift>KP_Subtract',
                                       self.transpose_selection, None, -1)
        self.accel_map.add_accelerator('<Shift>minus',
                                       self.transpose_selection, None, -1)
        self.accel_map.add_accelerator('<Shift>underscore',
                                       self.transpose_selection, None, -1)
        #~self.accel_map.add_accelerator('<Control>Page_Up',
        #~                               self.change_resolution, True)
        #~self.accel_map.add_accelerator('<Control>Page_Down',
        #~                               self.change_resolution, False)
        self.accel_map.add_accelerator('<Control>c', self.copy)
        self.accel_map.add_accelerator('<Control>v', self.paste)
        self.accel_map.add_accelerator('<Control>x', self.cut)
        self.accel_map.add_accelerator('<Control>i', self.interpolate_selection)
        self.accel_map.add_accelerator('<Control>l', self.on_popup_solo)
        self.accel_map.add_accelerator('<Control>a', self.select_all)
        self.accel_map.add_accelerator('<Control>KP_Add',
                                       self.on_popup_add_track)
        self.accel_map.add_accelerator('<Control>plus',
                                       self.on_popup_add_track)
        self.accel_map.add_accelerator('<Control>KP_Subtract',
                                       self.on_popup_delete_track)
        self.accel_map.add_accelerator('<Control>minus',
                                       self.on_popup_delete_track)
        self.connect('key-press-event', self.accel_map.handle_key_press_event)
        self.connect("expose_event", self.expose)
        self.connect('key-press-event', self.on_key_down)
        self.connect('key-release-event', self.on_key_up)
        self.connect('button-press-event', self.on_button_down)
        self.connect('button-release-event', self.on_button_up)
        self.connect('motion-notify-event', self.on_motion)
        self.connect('scroll-event', self.on_mousewheel)
        gobject.timeout_add(100, self.update_position)
        self.hscroll.connect('change-value', self.on_hscroll_window)
        self.vscroll.connect('change-value', self.on_vscroll_window)
        eventbus = com.get('neil.core.eventbus')
        eventbus.active_patterns_changed += self.on_active_patterns_changed
        eventbus.active_plugins_changed += self.on_active_patterns_changed
        eventbus.zzub_pattern_changed += self.on_pattern_changed
        eventbus.zzub_edit_pattern += self.on_edit_pattern
        eventbus.zzub_pattern_insert_rows += self.on_pattern_insert_rows
        eventbus.zzub_pattern_remove_rows += self.on_pattern_remove_rows
        eventbus.zzub_parameter_changed += self.on_zzub_parameter_changed
        eventbus.document_loaded += self.update_all
        self.pattern_changed()

    def on_zzub_parameter_changed(self, plugin, group, track, param, value):
        """
        called when a parameter changes in zzub. checks whether this parameter
        is related to master bpm or tpb and updates the view.
        """
        pass

    def on_pattern_insert_rows(self, plugin, index, row, rows, column_indices, indices):
        self.on_pattern_changed(plugin, index)

    def on_pattern_remove_rows(self, plugin, index, row, rows, column_indices, indices):
        self.on_pattern_changed(plugin, index)

    def on_edit_pattern(self, plugin, index, group, track, column, row, value):
        if plugin != self.plugin:
            return
        if index != self.pattern:
            return
        self.update_line(row)
        self.redraw()

    def on_pattern_changed(self, plugin, index):
        if plugin != self.plugin:
            return
        if index != self.pattern:
            return
        self.pattern_changed()

    def update_font(self):
        pctx = self.get_pango_context()
        desc = pango.FontDescription(config.get_config().get_pattern_font()) #.get_font_description()
        pctx.set_font_description(desc)
        self.fontdesc = desc
        self.font = pctx.load_font(desc)
        metrics = self.font.get_metrics(None)
        fh = (metrics.get_ascent() + metrics.get_descent()) / pango.SCALE
        fw = metrics.get_approximate_digit_width() / pango.SCALE
        self.row_height = fh # row height
        self.top_margin = fh # top margin
        self.column_width = fw # column width

    def tab_to_note_column(self):
        """
        Tab to group 1 track 0 -- usually the first note column
        """
        if self.move_track_right():
            self.set_index(0)
            self.set_subindex(0)
            self.show_cursor_right()
            self.refresh_view()

    def plugin_created(self, *args):
        self.jump_to_note = True # mark that we want tab_to_note_column on next update

    def on_copy(self, widget):
        """
        Sent when the copy function is selected from the menu.
        """
        self.copy()

    def on_cut(self, widget):
        """
        Sent when the cut function is selected from the menu.
        """
        self.cut()

    def on_paste(self, widget):
        """
        Sent when the paste function is selected from the menu.
        """
        self.paste()

    def on_context_menu(self, event):
        """
        Callback that constructs and displays the popup menu
        """
        player = com.get('neil.core.player')
        if self.selection != None and self.selection.begin >= 0:
            sel_sensitive = True
        else:
            sel_sensitive = False
        if get_clipboard_text().startswith(self.CLIPBOARD_MAGIC):
            paste_sensitive = True
        else:
            paste_sensitive = False

        menu = Menu()
        menu.add_item("Add track", self.on_popup_add_track)
        menu.add_item("Remove last track", self.on_popup_delete_track)
        menu.add_separator()
        menu.add_item("New pattern...", self.on_popup_create_pattern)
        menu.add_item("Pattern properties...", self.on_popup_properties)
        menu.add_item("Clone pattern...", self.on_popup_create_copy)
        menu.add_item("Remove pattern", self.on_popup_remove_pattern)
        menu.add_separator()
        menu.add_item("Cut", self.on_popup_cut).set_sensitive(sel_sensitive)
        menu.add_item("Copy", self.on_popup_copy).set_sensitive(sel_sensitive)
        menu.add_item("Paste", self.on_popup_paste).set_sensitive(paste_sensitive)
        menu.add_item("Delete", self.on_popup_delete).set_sensitive(sel_sensitive)
        menu.add_separator()
        menu.add_item("Double", self.on_popup_double)
        menu.add_item("Halve", self.on_popup_halve)
        menu.add_separator()
        label, transform = menu.add_submenu("Transform")
        transform.add_item("Transpose +1", self.transpose_selection, 1).set_sensitive(sel_sensitive)
        transform.add_item("Transpose -1", self.transpose_selection, -1).set_sensitive(sel_sensitive)
        transform.add_item("Transpose +12", self.transpose_selection, 12).set_sensitive(sel_sensitive)
        transform.add_item("Transpose -12", self.transpose_selection, -12).set_sensitive(sel_sensitive)
        transform.add_item("Interpolate", self.interpolate_selection).set_sensitive(sel_sensitive)
        transform.add_item("Reverse", self.reverse_selection).set_sensitive(sel_sensitive)

        # Pattern effects menu
        label, effects_menu = menu.add_submenu("Pattern effects")
        effects = com.get_from_category('patternfx')
        for effect in effects:
            effects_menu.add_item(effect.name, self.on_pattern_effect, effect).\
                set_sensitive(sel_sensitive) 

        # User script menu
        # label, scripts_menu = menu.add_submenu("User scripts")
        # import sys
        # import os
        # home_folder = os.getenv('HOME') + '/.neil'
        # if not (home_folder in sys.path):
        #     sys.path.append(home_folder)
        # try:
        #     from neil_scripts import scripts
        #     for key in scripts.iterkeys():
        #         scripts_menu.add_item(key, self.on_activate_user_script, 
        #                               scripts[key]).set_sensitive(sel_sensitive)
        # except ImportError:
        #     pass
        envelope_effect = com.get('neil.core.patternfx.envelope')
        menu.add_item("Envelope", self.on_pattern_effect, envelope_effect).set_sensitive(sel_sensitive)
        menu.add_item("Expression", self.on_expression)
        menu.add_separator()

        issolo = player.solo_plugin == self.get_plugin()
        menu.add_check_item("Solo Plugin", issolo, self.on_popup_solo)
        menu.show_all()
        menu.attach_to_widget(self, None)
        menu.popup(self, event)

    def on_pattern_effect(self, item, effect):
        values = {}
        for row, group, track, index in self.selection_range():
            value = self.plugin.get_pattern_value(self.pattern, group, 
                                                  track, index, row)
            try:
                values[(group, track, index)] = (values[(group, track, index)] +
                                                 [(row, value)])
            except KeyError:
                values[(group, track, index)] = [(row, value)]
        for key in values.iterkeys():
            group = key[0]
            track = key[1]
            index = key[2]
            param = self.plugin.get_parameter(group, track, index)
            output = effect.transform([value[1] for value in values[key]], 
                                      param)
            for (row, value), i in zip(values[key], range(len(output))):
                self.plugin.set_pattern_value(self.pattern, group, track, index,
                                              row, output[i])
            player = com.get('neil.core.player')
            player.history_commit('pattern effect')

    def on_expression(self, item):
        expression = com.get('neil.core.expression')
        expression.transform(self.plugin, self.pattern, self.selection_range())
        player = com.get('neil.core.player')
        player.history_commit('expression applied')

    def update_position(self):
        """
        Updates the position.
        """
        # TODO: find some other means to find out visibility
#               if self.rootwindow.get_current_panel() != self.panel:
#                       return True
        player = com.get('neil.core.player')
        playpos = player.get_position()
        if self.playpos != playpos:
            self.draw_playpos_xor()
            self.playpos = playpos
            self.draw_playpos_xor()
        return True

    def get_new_pattern_name(self, m=None):
        """
        Finds an unused pattern name.
        """
        if not m:
            m = self.get_plugin()
        return get_new_pattern_name(m)

    def init_values(self):
        """
        Initializes pattern storage and information.
        """
        # plugin
        self.plugin = None
        self.pattern = -1
        # parameter count
        self.parameter_count = [0,0,0]
        self.parameter_width = [[],[],[]]
        self.lines = None
        self.levels = {}
        self.factor_sources = {}
        self.row_count = 0
        self.parameter_type = None
        self.subindex_count = None
        self.group_position = [0,0,0]
        self.group_track_count = [0,0,0]
        datasource = self.get_datasource()
        if datasource:
            # plugin loader, pattern data
            self.plugin, self.pattern = datasource
            self.row, self.group, self.track, self.index, self.subindex =\
                      self.plugin_info.get(self.plugin).pattern_position
            self.selection = self.plugin_info.get(self.plugin).selection
            self.input_connection_count =\
                self.get_plugin().get_input_connection_count()
            # track count
            track_count = self.plugin.get_track_count()
            # global track not considered a track
            # track count by group
            self.group_track_count =\
                [self.input_connection_count, 1, track_count]
            self.parameter_count = []
            self.parameter_width = []
            self.parameter_type = []
            self.track_width = []
            for group in xrange(3):
                if (group == 0) and not self.plugin.get_input_connection_count():
                    group_parameter_count = 0
                else:
                    # parameter counts
                    group_parameter_count = self.plugin.get_parameter_count(group,0)
                self.parameter_count.append(group_parameter_count)
                # parameter widths in columns
                widths = []
                types = []
                for group_param in xrange(group_parameter_count):
                    param = self.plugin.get_parameter(group,0,group_param)
                    paramtype = param.get_type()
                    paramwidth = get_length_from_param(param)
                    types.append(paramtype)
                    widths.append(paramwidth)
                self.track_width.append(sum(widths) + group_parameter_count)
                self.parameter_width.append(widths)
                self.parameter_type.append(types)
            # group positions
            self.group_position = [
                    0,
                    (self.track_width[0]*self.group_track_count[0]),
                    (self.track_width[0]*self.group_track_count[0])+(self.track_width[1]*self.group_track_count[1])
            ]
            # parameter positions, relative to track start
            self.parameter_position = []
            for g in range(3):
                pp = []
                x = 0
                for i in range(self.parameter_count[g]):
                    pp.append(x)
                    x += get_length_from_param(self.plugin.get_parameter(g,0,i)) + 1
                self.parameter_position.append(pp)
            # sub index counts
            self.subindex_count = [[get_subindexcount_from_param(self.plugin.get_parameter(g,0,i)) \
                    for i in range(self.parameter_count[g])] for g in range(3)]
            # sub index offsets
            self.subindex_offset = [[get_subindexoffsets_from_param(self.plugin.get_parameter(g,0,i)) \
                    for i in range(self.parameter_count[g])] for g in range(3)]
            self.row_count = self.plugin.get_pattern_length(self.pattern) # row count
            self.prepare_textbuffer()
            self.adjust_scrollbars()
        # set the pattern position
        self.set_row(self.row)
        self.set_group(self.group)
        self.set_track(self.track)
        self.set_index(self.index)
        self.set_subindex(self.subindex)
        self.refresh_view()

    def get_client_size(self):
        rect = self.get_allocation()
        return rect.width, rect.height

    def adjust_scrollbars(self):
        w, h = self.get_client_size()
        vw, vh = self.get_virtual_size()
        vh += 1
        vw += 1
        pw, ph = int((w - PATLEFTMARGIN) / float(self.column_width) + 0.5),\
                 int((h - self.row_height) / float(self.row_height) + 0.5)
        hrange = vw - pw
        vrange = vh - ph
        if hrange <= 0:
            self.hscroll.hide()
        else:
            self.hscroll.show()
        if vrange <= 0:
            self.vscroll.hide()
        else:
            self.vscroll.show()
        adj = self.hscroll.get_adjustment()
        adj.set_all(self.start_col, 0, vw, 1, 1, pw)
        adj = self.vscroll.get_adjustment()
        adj.set_all(self.start_row, 0, vh, 1, 1, ph)

    def set_octave(self, o):
        """
        Sets the octave.

        @param o: Octave
        @type o: int
        """
        player = com.get('neil.core.player')
        player.octave = min(max(o,0), 9)

    def set_index(self, i):
        """
        Sets the current index position.

        @param i: Index position.
        @type i: int
        """
        self.index = min(max(i,0), self.parameter_count[self.group] - 1)

    def set_track(self, t):
        """
        Sets the current track position.

        @param t: Track position.
        @type t: int
        """
        self.track = min(max(t,0), self.group_track_count[self.group] - 1)

    def set_group(self, g):
        """
        Sets the current group position.

        @param g: Group position.
        @type g: int
        """
        g = min(max(g,0), 2)
        # skip empty groups
        if g < self.group:
            while g >= 0:
                if self.parameter_count[g] * self.group_track_count[g]:
                    self.group = g
                    return True
                g -= 1
        elif g >= self.group:
            while g <= 2:
                if self.parameter_count[g] * self.group_track_count[g]:
                    self.group = g
                    return True
                g += 1
        return False

    def show_row(self, r):
        """
        Makes a row in the editor visible.

        @param r: Row position.
        @type r: int
        """
        row = min(max(r,0), self.row_count - 1)
        if row >= 0:
            w,h = self.get_client_size()
            endrow = (((h - self.top_margin) / self.row_height * 1) + self.start_row) - 1
            if (row < self.start_row):
                self.start_row = row
                self.redraw()
            elif (row >= endrow):
                self.start_row = row - (endrow - self.start_row)
                self.redraw()

    def set_row(self, r):
        """
        Sets the current row position.

        @param r: Row position.
        @type r: int
        """
        self.row = min(max(r,0), self.row_count - 1)
        self.show_row(self.row)

    def set_subindex(self, si):
        """
        Sets the current subindex position.

        @param si: Subindex position.
        @type si: int
        """
        if not self.subindex_count:
            self.subindex = 0
            return
        self.subindex = min(max(si,0), self.subindex_count[self.group][self.index] - 1)

    def expose(self, widget, *args):
        self.context = widget.window.cairo_create()
        if self.current_plugin!=self.get_plugin():
            self.pattern_changed()
            self.current_plugin=self.get_plugin()
        #player = com.get('neil.core.player')
        #if not(player.spinbox_edit):
        #    self.grab_focus()
        if (self.needfocus):
            self.grab_focus()
            self.needfocus = False
        self.draw(self.context)
        return False

    def redraw(self, *args):
        if self.window:
            w, h = self.get_client_size()
            self.window.invalidate_rect((0, 0, w, h), False)

    def on_active_patterns_changed(self, selpatterns):
        if self.window:
            self.pattern_changed()

    def pattern_changed(self, *args):
        """
        Loads and redraws the pattern view after the pattern has been changed.
        """
        self.init_values()
        self.show_cursor_left()
        plugin = self.get_plugin()
        if plugin:
            self.plugin_info.get(plugin).reset_patterngfx()
        if self.jump_to_note:
            self.tab_to_note_column()
            self.jump_to_note = False

    def move_up(self, step = 1):
        """
        Moves the cursor up.

        @param step: Amount the cursor is moved up.
        @type step: int
        """
        self.set_row(self.row - step)
        self.update_statusbar()
        self.redraw()

    def move_down(self, step = 1):
        """
        Moves the cursor down.

        @param step: Amount the cursor is moved down.
        @type step: int
        """
        #for i in range(step+2):
        #       self.update_line(self.row+i+1)
        self.set_row(self.row + step)
        self.update_statusbar()
        self.redraw()

    def move_track_left(self):
        """
        Moves the cursor one track position left.
        """
        if (self.track == 0):
            if (self.group == CONN) or not self.set_group(self.group - 1):
                return False
            self.set_track(self.group_track_count[self.group] - 1)
        else:
            self.set_track(self.track - 1)
        return True

    def move_index_left(self):
        """
        Moves the cursor one index position left.
        """
        if self.index == 0:
            if not self.move_track_left():
                return False
            self.set_index(self.parameter_count[self.group] - 1)
        else:
            self.set_index(self.index - 1)
        return True

    def move_subindex_left(self):
        """
        Moves the cursor one subindex position left.
        """
        if self.subindex == 0:
            if not self.move_index_left():
                return False
            self.set_subindex(self.subindex_count[self.group][self.index] - 1)
        else:
            self.set_subindex(self.subindex - 1)
        return True

    def move_track_right(self):
        """
        Moves the cursor one track position right.
        """
        if (self.track == self.group_track_count[self.group]-1):
            if (self.group == TRACK) or not self.set_group(self.group + 1):
                return False
            self.set_track(0)
        else:
            self.set_track(self.track + 1)
        return True

    def move_index_right(self):
        """
        Moves the cursor one index position right.
        """
        if self.index == (self.parameter_count[self.group] - 1):
            if not self.move_track_right():
                return False
            self.set_index(0)
        else:
            self.set_index(self.index + 1)
        return True

    def move_subindex_right(self):
        """
        Moves the cursor one subindex position right.
        """
        if self.subindex == (self.subindex_count[self.group][self.index] - 1):
            if not self.move_index_right():
                return False
            self.set_subindex(0)
        else:
            self.set_subindex(self.subindex + 1)
        return True

    def show_cursor_left(self):
        """
        Puts the cursor into visible frame after a jump to the left.
        """
        w, h = self.get_charbounds()
        x, y = self.pattern_to_charpos(self.row, self.group,
                                       self.track, self.index, self.subindex)
        if x < self.start_col:
            self.start_col = max(x - (w / 3), 0)
            self.redraw()

    def show_cursor_right(self):
        """
        Puts the cursor into visible frame after a jump to the right.
        """
        w, h = self.get_charbounds()
        vw, vh = self.get_virtual_size()
        x, y = self.pattern_to_charpos(self.row, self.group,
                                       self.track, self.index, self.subindex)
        if x > w:
            self.start_col = min(self.start_col + x - w + (w / 3),
                                 vw - w + self.start_col)
            self.redraw()

    def move_left(self):
        """
        Moves the cursor left.
        """
        if self.pattern == -1:
            return
        self.move_subindex_left()
        self.show_cursor_left()
        self.update_statusbar()
        self.redraw()

    def move_right(self):
        """
        Moves the cursor right.
        """
        if self.pattern == -1:
            return
        self.move_subindex_right()
        self.show_cursor_right()
        self.update_statusbar()
        self.redraw()

    def adjust_selection(self):
        """
        Adjusts the selection variables according to the selection mode.
        """
        if self.selection.mode > SEL_GROUP:
            self.selection.group = 0
        else:
            self.selection.group = self.group
        if self.selection.mode > SEL_TRACK:
            self.selection.track = 0
        else:
            self.selection.track = self.track
        if self.selection.mode > SEL_COLUMN:
            self.selection.index = 0
        else:
            self.selection.index = self.index

    def selection_range(self):
        """
        Iterator that moves through the current selection.

        @return: Tuple pair of the next position (row, group, track, index)
        @rtype: (int, int, int, int)
        """
        if not self.selection:
            yield (self.row, self.group, self.track, self.index)
        elif self.selection.mode == SEL_COLUMN:
            for row in range(self.selection.begin, self.selection.end):
                yield (row, self.group, self.track, self.index)
        elif self.selection.mode == SEL_TRACK:
            for row in range(self.selection.begin, self.selection.end):
                for index in range(0, self.parameter_count[self.group]):
                    yield (row, self.group, self.track, index)
        elif self.selection.mode == SEL_GROUP:
            for row in range(self.selection.begin, self.selection.end):
                for track in range(0, self.group_track_count[self.group]):
                    for index in range(0, self.parameter_count[self.group]):
                        yield (row, self.group, track, index)
        elif self.selection.mode == SEL_ALL:
            for row in range(self.selection.begin, self.selection.end):
                for group in range(3):
                    tc = self.group_track_count[group]
                    for track in range(0, tc):
                        for index in range(0, self.parameter_count[group]):
                            yield (row, group, track, index)

    def pattern_range(self):
        """
        Iterator that moves through the entire pattern.

        @return: Tuple pair of the next position (row, group, track, index)
        @rtype: (int, int, int, int)
        """
        for row in range(0, self.row_count):
            for group in range(3):
                tc = self.group_track_count[group]
                for track in range(0, tc):
                    for index in range(0, self.parameter_count[group]):
                        yield (row, group, track, index)

    def reverse_selection(self, widget):
        """
        Reverse the current selection (retrograde).
        """
        values = []
        for row, group, track, index in self.selection_range():
            value = self.plugin.get_pattern_value(self.pattern, group, 
                                                  track, index, row)
            values.append([row, group, track, index, value])
        rows = [entry[0] for entry in values]
        rows.reverse()
        values = [[row] + entry[1:] for (row, entry) in zip(rows, values)]
        for row, group, track, index, value in values:
            self.plugin.set_pattern_value(self.pattern, group, track, 
                                          index, row, value)
        player = com.get('neil.core.player')
        player.history_commit("reverse")

    def transpose_selection(self, widget, offset):
        """
        Transposes the current selection by an offset.

        @param offset: The amount that the values is incremented.
        @type offset: int
        """
        for r, g, t, i in self.selection_range():
            if r > self.plugin.get_pattern_length(self.pattern) - 1:
                break
            if r < 0:
                continue
            p = self.plugin.get_parameter(g, t, i)
            v = self.plugin.get_pattern_value(self.pattern, g, t, i, r)
            if v != p.get_value_none():
                if (p.get_name() == "Note"):
                    if v != zzub.zzub_note_value_off:
                        v = max(min(mn2bn(bn2mn(v) + offset), 
                                    p.get_value_max()), p.get_value_min())
                # Why would you want to transpose something that's not
                # a note??:
                #
                #else:
                #    v = max(min(v + offset, p.get_value_max()), 
                #            p.get_value_min())
                self.plugin.set_pattern_value(self.pattern, g, t, i, r, v)
        tmp_sel = self.selection
        player = com.get('neil.core.player')
        player.history_commit("transpose")
        self.selection = tmp_sel

    def interpolate_selection(self, widget=None):
        """
        Fills the current selection with values interpolated
        from selection start to selection end.
        """
        player = com.get('neil.core.player')
        #player.set_callback_state(False)
        if not self.selection:
            return
        if self.selection.end == self.selection.begin + 1:
            return
        step = 1
        for r, g, t, i in self.selection_range():
            if r > self.plugin.get_pattern_length(self.pattern) - 1:
                break
            if r < 0:
                continue
            p = self.plugin.get_parameter(g, t, i)
            v1 = self.plugin.get_pattern_value(self.pattern, g, t, i, self.selection.begin)
            v2 = self.plugin.get_pattern_value(self.pattern, g, t, i, self.selection.end - 1)
            if (v1 != p.get_value_none()) and (v2 != p.get_value_none()):
                if (p.get_type() == 0 and (v1 == zzub.zzub_note_value_off or v2 == zzub.zzub_note_value_off)):
                    continue
                # Sometimes this might prevent the
                # interpolation from ever achieving
                # the final value (v2), but the fix
                # would be to add the final value to
                # the last row, and this would disrupt
                # the row_step-induced rhythm.
                if (r - self.selection.begin) % step != 0:
                    v = p.get_value_none()
                else:
                    f = float(r - self.selection.begin) / float(self.selection.end - self.selection.begin - 1)
                    if (p.get_type() == 0):
                        v1 = bn2mn(v1)
                        v2 = bn2mn(v2)
                        v = mn2bn(roundint((v2 - v1) * f + v1))
                    else:
                        v = roundint((v2 - v1) * f + v1)
                self.plugin.set_pattern_value(self.pattern,g,t,i,r,v)
        tmp_sel = self.selection
        player.history_commit("interpolate")
        self.selection = tmp_sel
        # if player.set_callback_state(True):
        #     eventbus = com.get('neil.core.eventbus')
        #     eventbus.document_loaded()

    def select_all(self, widget=None):
        """
        Select all contents of the open pattern.
        Invoked when user presses Ctrl+a.
        """
        if not self.selection:
            self.selection = self.Selection()
        self.selection.mode = SEL_ALL
        self.selection.begin = 0
        self.selection.end = self.plugin.get_pattern_length(self.pattern)
        self.redraw()

    def cut(self):
        """
        Cuts the current selection into the clipboard
        """
        if not self.selection:
            return
        self.copy()
        player = com.get('neil.core.player')
        #player.set_callback_state(False)
        for r, g, t, i in self.selection_range():
            if r > self.plugin.get_pattern_length(self.pattern) - 1:
                break
            if r < 0:
                continue
            p = self.plugin.get_parameter(g, t, i)
            self.plugin.set_pattern_value(self.pattern, g, t, i, r, 
                                          p.get_value_none())
        player = com.get('neil.core.player')
        player.history_commit("remove event")
        # if player.set_callback_state(True):
        #     eventbus = com.get('neil.core.eventbus')
        #     eventbus.document_loaded()

    def copy(self):
        """
        Copies the current selection into the clipboard
        """
        if not self.selection:
            return
        data = self.CLIPBOARD_MAGIC
        data += "%01x" % self.selection.mode
        for r, g, t, i in self.selection_range():
            if r > self.plugin.get_pattern_length(self.pattern) - 1:
                break
            if r < 0:
                continue
            data += "%04x%01x%02x%02x%04x" % (r - self.selection.begin,g,t,i,self.plugin.get_pattern_value(self.pattern,g,t,i,r))
        set_clipboard_text(data)

    def delete(self):
        """
        Deletes the current selection
        """
        player = com.get('neil.core.player')
        #player.set_callback_state(False)
        for r,g,t,i in self.selection_range():
            if r>self.plugin.get_pattern_length(self.pattern)-1:
                break
            if r<0:
                continue
            p = self.plugin.get_parameter(g,t,i)
            self.plugin.set_pattern_value(self.pattern,g,t,i,r,p.get_value_none())
        player = com.get('neil.core.player')
        player.history_commit("delete events")
        # if player.set_callback_state(True):
        #     eventbus = com.get('neil.core.eventbus')
        #     eventbus.document_loaded()

    def unpack_clipboard_data(self, d):
        """
        Unpacks clipboard data
        """
        magic, d = d[:len(self.CLIPBOARD_MAGIC)], d[len(self.CLIPBOARD_MAGIC):]
        assert magic == self.CLIPBOARD_MAGIC
        mode, d = int(d[:1], 16), d[1:]
        yield mode
        while d:
            r, d = int(d[:4], 16), d[4:]
            g, d = int(d[:1], 16), d[1:]
            t, d = int(d[:2], 16), d[2:]
            i, d = int(d[:2], 16), d[2:]
            v, d = int(d[:4], 16), d[4:]
            yield r, g, t, i, v

    def paste(self):
        """
        Pastes the clipboard data into the pattern view.

        The pasting mechanism looks a bit weird but its effective.
        you can serialize pattern data in a pasteable form,
        and upon deserialization the app tries to make parameters
        as valid as possible.

        Buzz used to not paste at all if the format wasnt right
        we still try to make some sense out of what we get.
        """
        player = com.get('neil.core.player')
        #player.set_callback_state(False)
        data = get_clipboard_text()
        try:
            gen = self.unpack_clipboard_data(data.strip())
            mode = gen.next()
            assert (mode >= 0) and (mode <= SEL_ALL)
            for r,g,t,i,v in gen:
                r = self.row + r
                assert (g >= 0) and (g <= 2)
                if (g < 0) or (g > 2):
                    continue
                if (r < 0) or (r >= self.row_count):
                    continue
                if mode == SEL_COLUMN: # am i in column paste mode?
                    i = self.index # so paste at cursor column
                elif (i < 0) or (i >= self.parameter_count[g]): # if not, skip if out of bounds
                    continue
                if mode in (SEL_TRACK, SEL_COLUMN): # am i pasting a track or a column?
                    t = self.track # paste at cursor track
                elif (t < 0) or (t >= self.group_track_count[g]): # if not, skip if out of bounds
                    continue
                p = self.plugin.get_parameter(g,t,i)
                ty = p.get_type()
                if v != p.get_value_none(): # if its not a none value
                    if ty == 0: # is our target a note?
                        v = fixbn(v) # fix it
                        if v != zzub.zzub_note_value_off:
                            v = min(max(v, p.get_value_min()),p.get_value_max()) # make sure it is properly clamped
                    elif ty == 1: # switch
                        v = v % 2 # even is zero, odd is one
                        v = min(max(v, p.get_value_min()),p.get_value_max()) # make sure it is properly clamped
                    elif ty == 2: # byte
                        v = v & 0xFF # mask out first 8 bytes
                        v = min(max(v, p.get_value_min()),p.get_value_max()) # make sure it is properly clamped
                self.plugin.set_pattern_value(self.pattern,g,t,i,r,v) # finally set it
            #Non Buzz-like behaviour (naughty naughty!) ;)  :
            #self.set_row(r+1)
            self.update_statusbar()
            player.history_commit("paste events")
        except:
            pass
        # if player.set_callback_state(True):
        #     eventbus = com.get('neil.core.eventbus')
        #     eventbus.document_loaded()

    def on_mousewheel(self, widget, event):
        """
        Callback that responds to mousewheeling in pattern view.
        """
        mask = event.state
        if mask & gtk.gdk.CONTROL_MASK:
            if event.direction == gtk.gdk.SCROLL_UP:
                self.change_resolution(True)
            elif event.direction == gtk.gdk.SCROLL_DOWN:
                self.change_resolution(False)
        else:
            if event.direction == gtk.gdk.SCROLL_UP:
                self.move_up(self.edit_step)
                self.adjust_scrollbars()
            elif event.direction == gtk.gdk.SCROLL_DOWN:
                self.move_down(self.edit_step)
                self.adjust_scrollbars()

    def on_button_down(self, widget, event):
        """
        Callback that responds to mouse click in pattern view.
        """
        if not self.selection:
            self.selection = self.Selection()
        self.grab_focus()
        if event.button == 3:
            self.on_context_menu(event)
        if self.pattern == -1:
            return
        if event.button == 1:
            x, y = int(event.x), int(event.y)
            row, group, track, index, subindex = self.pos_to_pattern((x, y))
            if event.type == gtk.gdk._2BUTTON_PRESS:
                self.selection.mode = SEL_COLUMN
                self.selection.begin = 0
                self.selection.end = self.row_count
                self.adjust_selection()
                self.redraw()
            else:
                self.set_row(row)
                self.set_group(group)
                self.set_track(track)
                self.set_index(index)
                self.set_subindex(subindex)
                self.update_statusbar()
                self.dragging = True
                self.selection.begin = row
                self.selection.end = row
                self.clickpos = self.pos_to_pattern((x, y))
                self.adjust_selection()
                self.redraw()

    def on_motion(self, widget, *args):
        """
        Callback that responds to mouse motion in sequence view.

        @param event: Mouse event
        @type event: wx.MouseEvent
        """
        x, y, state = self.window.get_pointer()
        x, y = int(x), int(y)
        row, group, track, index, subindex = self.pos_to_pattern((x, y))
        if self.dragging:
            row, group, track, index, subindex = self.pos_to_pattern((x, y))
            if group != self.clickpos[1]:
                self.selection.mode = SEL_ALL
            elif track != self.clickpos[2]:
                self.selection.mode = SEL_GROUP
            elif index != self.clickpos[3]:
                self.selection.mode = SEL_TRACK
            else:
                self.selection.mode = SEL_COLUMN
            self.show_row(row)
            if row < self.clickpos[0]:
                self.selection.end = self.clickpos[0] + 1
                self.selection.begin = row
            else:
                self.selection.begin = self.clickpos[0]
                self.selection.end = row + 1
            self.adjust_selection()
            self.redraw()

    def on_button_up(self, widget, event):
        """
        Callback that responds to mouse button release event in pattern view.
        """

        if event.button == 1:
            self.dragging = False

    def on_popup_remove_pattern(self, *args):
        """
        Callback that removes the current pattern.
        """
        player = com.get('neil.core.player')
        m = self.get_plugin()
        if self.pattern >= 0:
            m.remove_pattern(self.pattern)
            player.activate_pattern(-1) # go one back
            player.history_commit("remove pattern")

    def on_popup_create_pattern(self, widget=None, m=None):
        """
        Callback that creates a pattern.
        """
        if self.get_plugin() == None:
            return
        player = com.get('neil.core.player')
        name = self.get_new_pattern_name(m)
        result = show_pattern_dialog(self, name, self.patternsize, DLGMODE_NEW)
        if not result:
            return
        name, self.patternsize, switch = result
        if not m:
            m = self.get_plugin()
        p = m.create_pattern(self.patternsize)
        p.set_name(name)
        m.add_pattern(p)
        player.history_commit("new pattern")
        if switch:
            player.active_patterns = [(m, m.get_pattern_count() - 1)]

    def on_popup_double(self, *args):
        """
        Callback that doubles the length of the current pattern while
        keeping notes intact
        """
        player = com.get('neil.core.player')
        #player.set_callback_state(False)
        pattern_index=[]
        pattern_contents=[]
        for r,g,t,i in self.pattern_range():
            pattern_index.append((r,g,t,i))
            pattern_contents.append(self.plugin.get_pattern_value(self.pattern,g,t,i,r))
            param = self.plugin.get_parameter(g,t,i)
            self.plugin.set_pattern_value(self.pattern,g,t,i,r,param.get_value_none())
        item = 0
        self.plugin.set_pattern_length(self.pattern,self.plugin.get_pattern_length(self.pattern)*2)
        for r,g,t,i in pattern_index:
            self.plugin.set_pattern_value(self.pattern,g,t,i,r*2,pattern_contents[item])
            item+=1
        player = com.get('neil.core.player')
        player.history_commit("double length")
        # if player.set_callback_state(True):
        #     eventbus = com.get('neil.core.eventbus')
        #     eventbus.document_loaded()

    def on_popup_halve(self, *args):
        """
        Callback that halves the length of the current pattern while
        keeping notes intact
        """
        player = com.get('neil.core.player')
        #player.set_callback_state(False)
        if self.plugin.get_pattern_length(self.pattern)==1:
            return
        for r,g,t,i in self.pattern_range():
            if r%2:
                continue
            self.plugin.set_pattern_value(self.pattern,g,t,i,r/2,self.plugin.get_pattern_value(self.pattern,g,t,i,r))
        self.plugin.set_pattern_length(self.pattern,self.plugin.get_pattern_length(self.pattern)/2)
        player = com.get('neil.core.player')
        player.history_commit("halve length")
        # if player.set_callback_state(True):
        #     eventbus = com.get('neil.core.eventbus')
        #     eventbus.document_loaded()

    def on_popup_create_copy(self, *args):
        """
        Callback that creates a copy of the current pattern.
        """
        player = com.get('neil.core.player')
        name = self.get_new_pattern_name()
        result = show_pattern_dialog(self,name,self.row_count,DLGMODE_COPY)
        if not result:
            return
        name, self.patternsize, switch = result
        m = self.get_plugin()
        p = m.get_pattern(self.pattern)
        p.set_name(name)
        m.add_pattern(p)
        player.history_commit("clone pattern")
        if switch:
            player.active_patterns = [(m, m.get_pattern_count()-1)]

    def on_popup_solo(self, *args):
        """
        Callback that solos current plugin.
        """
        plugin = self.get_plugin()
        player = com.get('neil.core.player')
        player.solo(plugin)

    def on_popup_properties(self, *args):
        """
        Callback that shows the properties of the current pattern.
        """
        result = show_pattern_dialog(self,self.plugin.get_pattern_name(self.pattern),self.plugin.get_pattern_length(self.pattern),DLGMODE_CHANGE)
        if not result:
            return
        name, rc, switch = result
        self.patternsize = rc
        if self.plugin.get_pattern_name(self.pattern) != name:
            self.plugin.set_pattern_name(self.pattern,name)
        if self.plugin.get_pattern_length(self.pattern) != rc:
            self.plugin.set_pattern_length(self.pattern,rc)
        player = com.get('neil.core.player')
        player.history_commit("change pattern properties")

    def on_popup_add_track(self, *args):
        """
        Callback that adds a track.
        """
        player = com.get('neil.core.player')
        if self.plugin != None:
            pluginloader = self.plugin.get_pluginloader()
            self.plugin.set_track_count(min(pluginloader.get_tracks_max(), self.plugin.get_track_count()+1))
            player.history_commit("add pattern track")
            self.pattern_changed()
        else:
            error(self, "Please select or add a pattern first!")

    def on_popup_delete_track(self, *args):
        """
        Callback that deletes last track.
        """
        player = com.get('neil.core.player')
        pluginloader = self.plugin.get_pluginloader()
        self.plugin.set_track_count(max(pluginloader.get_tracks_min() , self.plugin.get_track_count()-1))
        player.history_commit("remove pattern track")
        self.pattern_changed()

    def on_popup_cut(self, *args):
        """
        Callback that cuts selection
        """
        self.cut()

    def on_popup_copy(self, *args):
        """
        Callback that copies selection
        """
        self.copy()

    def on_popup_paste(self, *args):
        """
        Callback that pastes selection
        """
        self.paste()

    def on_popup_delete(self, *args):
        """
        Callback that deletes selection
        """
        self.delete()

    def tab_left(self):
        if (self.index != 0 or self.subindex != 0) or self.move_track_left():
            # If not at start of track, go there; if at
            # start of track, move to previous track. Note
            # short-circuit evaluation of 'or'.
            self.set_index(0)
            self.set_subindex(0)
            self.show_cursor_left()
            self.refresh_view()

    def tab_right(self):
        # move to next track
        if self.move_track_right():
            self.set_index(0)
            self.set_subindex(0)
            self.show_cursor_right()
            self.refresh_view()

    def on_key_down(self, widget, event):
        """
        Callback that responds to key stroke in pattern view.
        """
        mask = event.state
        kv = event.keyval
        # convert keypad numbers
        if gtk.gdk.keyval_from_name('KP_0') <= kv <= \
               gtk.gdk.keyval_from_name('KP_9'):
            kv = kv - gtk.gdk.keyval_from_name('KP_0') + \
                 gtk.gdk.keyval_from_name('0')
        k = gtk.gdk.keyval_name(kv)
        player = com.get('neil.core.player')
        eventbus = com.get('neil.core.eventbus')
        shiftdown = mask & gtk.gdk.SHIFT_MASK
        ctrldown = mask & gtk.gdk.CONTROL_MASK
        print "Key pressed:", k
        if k == 'less':
            player.activate_wave(-1)
            pass
        elif k == 'greater':
            player.activate_wave(1)
        elif mask & gtk.gdk.SHIFT_MASK and k == 'Down':
            if not self.selection:
                self.selection = self.Selection()
            if self.shiftselect == None:
                self.shiftselect = self.row
            self.move_down(self.edit_step)
            if self.row < self.shiftselect:
                self.selection.end = self.shiftselect + 1
                self.selection.begin = self.row
            else:
                self.selection.begin = self.shiftselect
                self.selection.end = self.row + 1
            self.adjust_selection()
            self.redraw()
        elif mask & gtk.gdk.SHIFT_MASK and k == 'Up':
            if not self.selection:
                self.selection = self.Selection()
            if self.shiftselect == None:
                self.shiftselect = self.row
            self.move_up(self.edit_step)
            if self.row < self.shiftselect:
                self.selection.end = self.shiftselect+1
                self.selection.begin = self.row
            else:
                self.selection.begin = self.shiftselect
                self.selection.end = self.row + 1
            self.adjust_selection()
            self.redraw()
        elif mask & gtk.gdk.SHIFT_MASK and (k == 'Right' or k == 'Left'):
            if not self.selection:
                self.selection = self.Selection()
            if self.shiftselect == None:
                self.shiftselect = self.row
                self.selection.begin = self.shiftselect
                self.selection.end = self.row + 1
            self.selection.mode = (self.selection.mode + 1) % 4
            self.adjust_selection()
            self.redraw()
        elif (mask & gtk.gdk.CONTROL_MASK):
            if k == 'b':
                if not self.selection:
                    self.selection = self.Selection()
                if self.keystartselect:
                    self.selection.begin = self.keystartselect
                if self.keyendselect:
                    self.selection.end = self.keyendselect
                if self.selection.begin == self.row:
                    self.selection.mode = (self.selection.mode + 1) % 4
                self.selection.begin = self.row
                self.keystartselect = self.row
                self.selection.end =\
                    max(self.row+1, self.selection.end)
                self.adjust_selection()
                self.update_plugin_info()
                self.redraw()
            elif k == 'e':
                if not self.selection:
                    self.selection = self.Selection()
                if self.keystartselect:
                    self.selection.begin=self.keystartselect
                if self.keyendselect:
                    self.selection.end=self.keyendselect
                if self.selection.end == self.row + 1:
                    self.selection.mode = (self.selection.mode + 1) % 4
                self.selection.end = self.row+1
                self.keyendselect=self.row+1
                self.selection.begin =\
                    max(min(self.selection.end - 1,
                            self.selection.begin), 0)
                self.adjust_selection()
                self.redraw()
            elif k == 'u':
                self.selection = None
                self.update_plugin_info()
                self.redraw()
            elif k == 'Up':
                player.activate_plugin(-1)
            elif k == 'Down':
                player.activate_plugin(1)
            else:
                return False
        elif k == 'Left' or k == 'KP_Left':
            self.move_left()
            self.adjust_scrollbars()
        elif k == 'Right' or k == 'KP_Right':
            self.move_right()
            self.adjust_scrollbars()
        elif k == 'Up' or k == 'KP_Up':
            self.move_up(self.edit_step)
            self.shiftselect = None
            self.adjust_scrollbars()
        elif k == 'Down' or k == 'KP_Down':
            self.move_down(self.edit_step)
            self.shiftselect = None
            self.adjust_scrollbars()
        elif k == 'Page_Up' or k == 'KP_Page_Up':
            self.move_up(16)
            self.adjust_scrollbars()
        elif k == 'Page_Down' or k == 'KP_Page_Down':
            self.move_down(16)
            self.adjust_scrollbars()
        elif k == 'Home':
            # 1st: move to begin of track
            # 2nd: move to begin of group
            # 3rd: move to first group
            # 4th: move to first row
            if self.index != 0:
                self.set_index(0)
                self.set_subindex(0)
            elif self.track != 0:
                self.set_track(0)
                self.set_index(0)
                self.set_subindex(0)
            elif not self.set_group(1):
                self.set_row(0)
            self.show_cursor_left()
            self.adjust_scrollbars()
            self.refresh_view()
        elif k == 'Insert' or k == 'KP_Insert':
            indices = []
            for index in range(1):
                for i in xrange(self.plugin.get_parameter_count(self.group,
                                                                self.track)):
                    indices += [self.group, self.track, i]
                self.lines[self.group][self.track].insert(self.row + index, "")
                self.update_line(self.row + index)
            del self.lines[self.group][self.track][-1]
            self.plugin.insert_pattern_rows(self.pattern, indices,
                                            len(indices) / 3, self.row, 1)
            player.history_commit("insert row")
        elif k == 'Delete':
            del self.lines[self.group][self.track][self.row:self.row +\
                                                   1]
            indices = []
            for index in range(1):
                self.lines[self.group][self.track].append('')
                for i in xrange(self.plugin.get_parameter_count(self.group,
                                                                self.track)):
                    indices += [self.group, self.track, i]
                self.update_line(self.row_count-1+index-1)
            self.plugin.remove_pattern_rows(self.pattern, indices,
                                            len(indices) / 3, self.row, 1)
            player.history_commit("remove row")
        elif k == 'Return':
            eventbus.edit_sequence_request()
        elif k in ('KP_Add','plus'):
            player.activate_pattern(1)
        elif k in ('KP_Subtract','minus'):
            player.activate_pattern(-1)
        elif k in ('KP_Multiply', 'dead_acute'):
            player = com.get('neil.core.player')
            self.set_octave(player.octave + 1)
        elif k in ('KP_Divide', 'ssharp'):
            player = com.get('neil.core.player')
            self.set_octave(player.octave - 1)
        elif k == 'bracketleft':
            step_select = self.panel.toolbar.edit_step_box
            step_select.set_active((step_select.get_active() - 1) % 12)
        elif k == 'bracketright':
            step_select = self.panel.toolbar.edit_step_box
            step_select.set_active((step_select.get_active() + 1) % 12)
        elif k == 'Escape':
            self.selection = None
            self.shiftselect = None
            self.update_plugin_info()
            self.redraw()
        # A key to insert a note or a parameter value was pressed.
        elif self.plugin and (kv < 256):
            # Get the parameter that you are currently editing.
            p = self.plugin.get_parameter(self.group, self.track, self.index)
            # Get parameter type.
            param_type = p.get_type()
            playtrack = False
            # If parameter is a note.
            if (param_type == 0):
                # Is there a wavetable parameter?
                wi = None
                wp = None
                wdata = None
                # Iterate over all available parameters.
                for i in range(self.parameter_count[self.group]):
                    pwp = self.plugin.get_parameter(self.group, self.track, i)
                    if pwp.get_flags() &\
                           zzub.zzub_parameter_flag_wavetable_index:
                        wp = pwp
                        wi = i
                        break
                if self.subindex == 0:
                    on = key_to_note(kv)
                    if on:
                        o, n = on
                        player = com.get('neil.core.player')
                        data = (min(player.octave + o, 9) << 4) | (n + 1)
                        if (wp != None):
                            if player.active_waves:
                                wdata = player.active_waves[0].get_index() + 1
                            else:
                                wdata = 1
                        playtrack = True
                    elif k == 'period':
                        data = p.get_value_none()
                        if wp != None:
                            wdata = wp.get_value_none()
                    elif k == '1':
                        data = zzub.zzub_note_value_off
                        if wp != None:
                            wdata = wp.get_value_none()
                        playtrack = True
                    else:
                        return False
                    if wdata != None:
                        self.plugin.set_pattern_value(self.pattern,
                                                      self.group,
                                                      self.track, wi,
                                                      self.row, wdata)
                        player.history_commit("enter event")
                elif (self.subindex == 1) and (k >= '1') and (k <= '9'):
                    o = ord(k) - ord('1') + 1
                    data = (self.plugin.get_pattern_value(self.pattern,
                                                          self.group,
                                                          self.track,
                                                          self.index,
                                                          self.row) & 0xf) |\
                                                          (o << 4)
                else:
                    return False
            elif param_type == 1: # switch
                if k == '1' or k == '0':
                    data = {'1' : p.get_value_max(), '0' : p.get_value_min()}[k]
                elif k == 'period':
                    data = p.get_value_none()
                else:
                    return False
            elif param_type in (2, 3): # byte or word
                pw = self.parameter_width[self.group][self.index]
                if k >= '0' and k <= '9':
                    o = ord(k)-ord('0')
                # An attempt to enter hexadecimal value as a letter
                elif (kv >= ord('a') and kv <= ord('f')):
                    o = 10 + kv - ord('a')
                # Account for when caps lock or shift is pressed
                elif (kv >= ord('A') and kv <= ord('F')):
                    o = 10 + kv - ord('A')
                elif k == 'period':
                    o = None
                    data = p.get_value_none()
                else:
                    return False
                if o != None:
                    bofs = (pw - self.subindex - 1) * 4
                    data = self.plugin.get_pattern_value(self.pattern,
                                                         self.group,
                                                         self.track,
                                                         self.index,
                                                         self.row)
                    if data == p.get_value_none():
                        data = 0
                    # mask out old nibble, put in new nibble
                    data = (data ^ (data & (0xf << bofs))) | (o << bofs)
                    data = min(p.get_value_max(), max(p.get_value_min(), data))
                    if p.get_flags() & zzub.zzub_parameter_flag_wavetable_index:
                        player.active_waves = [player.get_wave(data - 1)]
            else:
                return False
            self.plugin.set_pattern_value(self.pattern, self.group,
                                          self.track, self.index,
                                          self.row, data)
            self.play_note(playtrack)
            player.history_commit("enter event")
        else:
            return False
        return True

    def play_note(self, playtrack):
        """
        Plays entered note
        """
        player = com.get('neil.core.player')
        if playtrack and self.play_notes:
            m = self.get_plugin()
            for index in range(self.parameter_count[self.group]):
                v = self.plugin.get_pattern_value(self.pattern, self.group,
                                                  self.track, index, self.row)
                p = self.plugin.get_parameter(self.group, self.track, index)
                if v != p.get_value_none():
                    m.set_parameter_value_direct(self.group, self.track,
                                                 index, v, 0)
        self.move_down(self.edit_step)

    def on_key_up(self, widget, event):
        """
        Callback that responds to key release
        """
        player = com.get('neil.core.player')
        if config.get_config().get_pattern_noteoff() == True:
            kv = event.keyval
            k = gtk.gdk.keyval_name(kv)
            if k == 'Shift_L' or k=='Shift_R':
                self.shiftselect = None
            if self.plugin:
                parameter = self.plugin.get_parameter(self.group,0, self.index)
                if parameter.get_description() == "Note" and kv < 256:
                    on = key_to_note(kv)
                    if on:
                        m = self.get_plugin()
                        m.set_parameter_value(self.group, self.track,
                                              self.index,
                                              zzub.zzub_note_value_off, 0)
                        player.history_commit("add event")

    def on_char(self, event):
        """
        Callback that responds to key stroke in pattern view.

        @param event: Key event
        @type event: wx.KeyEvent
        """
        event.Skip()
        k = event.GetKeyCode()
        if k == ord('<'):
            # TODO: switch to previous wave
            pass
        elif k == ord('>'):
            # TODO: switch to next wave
            pass

    def pattern_to_charpos(self, row, group, track=0, index=0, subindex=0):
        """
        Converts a pattern position into a (x,y) character coordinate.

        @param row: Pattern row
        @param group: Specific pattern group (Connection, Global or Tracks)
        @param track: Track of the group
        @param index: Parameter index of the track
        @param subindex: Subindex of the index
        @type row, group, track, index, subindex: int
        @return: (x,y) character coordinate
        @rtype: (int, int)
        """
        y = row
        x = self.group_position[group]
        if self.parameter_count[group]:
            x += self.parameter_position[group][index]
            x += self.subindex_offset[group][index][subindex]
        if group in (CONN, TRACK):
            x += self.track_width[group]*track
        return x,y

    def pattern_to_pos(self, row, group, track=0, index=0, subindex=0):
        """
        Converts a pattern position into a (x, y) pixel coordinate.

        @param row: Pattern row
        @param group: Specific pattern group (Connection, Global or Tracks)
        @param track: Track of the group
        @param index: Parameter index of the track
        @param subindex: Subindex of the index
        @type row, group, track, index, subindex: int
        @return: (x, y) pixel coordinate
        @rtype: (int, int)
        """
        x, y = self.pattern_to_charpos(row, group, track, index, subindex)
        return ((x - self.start_col) * self.column_width) + PATLEFTMARGIN + 4, self.top_margin + ((y - self.start_row) * self.row_height)

    def charpos_to_pattern(self, position):
        """
        Converts a (x,y) character coordinate into a pattern position.

        @param position: Character coordinate.
        @type position: (int, int)
        @return: (row, group, track, index, subindex) representing a pattern position.
        @rtype: (int, int, int, int, int)
        """
        x, y = position
        # find group
        if x < self.group_position[1]:
            group = 0
        elif x < self.group_position[2]:
            group = 1
        else:
            group = 2
        x -= self.group_position[group]
        # find track
        track = self.track
        out_of_bounds = False
        if self.track_width[group] != 0:
            track = x / self.track_width[group]
            x -= track*self.track_width[group]
            # bounds checking
            if track >= self.group_track_count[group] or track < 0:
                track = self.track
                out_of_bounds = True
        # find index, subindex
        index = self.index
        subindex = self.subindex
        if not out_of_bounds:
            if self.parameter_count[group]:
                for i, pos in enumerate(self.parameter_position[group]):
                    if x < pos:
                        index = i-1
                        break
                else:
                    # last index
                    index = i
                x -= self.parameter_position[group][index]
                # subindex is that what remains
                subindex = x
        # find row
        row = min(max(0, y), self.row_count-1)
        return (row, group, track, index, subindex)

    def pos_to_pattern(self, position):
        """
        Converts a (x,y) pixel coordinate into a pattern position.

        @param position: Pixel coordinate.
        @type position: (int, int)
        @return: (row, group, track, index, subindex) representing a pattern position.
        @rtype: (int, int, int, int, int)
        """
        x, y = position
        return self.charpos_to_pattern(((x - PATLEFTMARGIN - 4) / self.column_width + self.start_col, (y - self.top_margin) / self.row_height*1 + self.start_row))

    def get_charbounds(self):
        """
        Returns the outermost coordinates in characters.
        """
        w,h = self.get_client_size()
        w -= PATLEFTMARGIN + 4
        h -= self.top_margin
        return self.start_col + (w / self.column_width) - 1, self.start_row + (h / self.row_height) - 1

    def get_virtual_size(self):
        """
        Returns the size in characters of the virtual view area.
        """
        if self.pattern == -1:
            return 0,0
        h = self.plugin.get_pattern_length(self.pattern)
        w = 0
        for g in CONN,GLOBAL,TRACK:
            w += self.track_width[g] * self.group_track_count[g]
        return w,h

    def on_vscroll_window(self, widget, scroll, value):
        """
        Handles vertical window scrolling.
        """
        adj = widget.get_adjustment()
        minv = adj.get_property('lower')
        maxv = adj.get_property('upper')
        pagesize = adj.get_property('page-size')
        value = int(max(min(value, maxv - pagesize), minv) + 0.5)
        widget.set_value(value)
        if self.start_row != value / 1 * 1:
            self.start_row = value / 1 * 1
        self.redraw()
        return True

    def on_hscroll_window(self, widget, scroll, value):
        """
        Handles horizontal window scrolling.
        """
        adj = widget.get_adjustment()
        minv = adj.get_property('lower')
        maxv = adj.get_property('upper')
        pagesize = adj.get_property('page-size')
        value = int(max(min(value, maxv - pagesize), minv) + 0.5)
        widget.set_value(value)
        if self.start_col != value:
            self.start_col = value
            self.redraw()

    def update_plugin_info(self):
        plugin = self.get_plugin()
        pi = self.plugin_info.get(plugin)
        # store current position
        pi.pattern_position = (self.row, self.group, self.track, self.index, self.subindex)
        pi.selection = self.selection

    def update_statusbar(self):
        # update plugin info
        self.update_plugin_info();
        if not self.plugin:
            return
        if self.parameter_count[self.group] and self.group_track_count[self.group]:
            # change status bar
            if self.group == 0:
                try:
                    pl = self.get_plugin()
                    in_plugin = pl.get_input_connection_plugin(self.track)
                    in_machine_name = in_plugin.get_name()
                except:
                    in_machine_name = ""
                self.statuslabels[0].set_label('Row %s, Incoming %s (%s)' %
                                               (self.row,self.track,in_machine_name))
            elif self.group == 1:
                self.statuslabels[0].set_label('Row %s, Globals' % (self.row,))
            else:
                self.statuslabels[0].set_label('Row %s, Track %s' % (self.row,self.track))
            p = self.plugin.get_parameter(self.group,self.track,self.index)
            self.statuslabels[2].set_label(prepstr(p.get_description() or ""))
            v = self.plugin.get_pattern_value(self.pattern, self.group, self.track, self.index, self.row)
            if v != p.get_value_none():
                text = prepstr(self.get_plugin().describe_value(self.group,self.index,v))
                s = get_str_from_param(p,self.plugin.get_pattern_value(self.pattern, self.group, self.track, self.index, self.row))
                self.statuslabels[1].set_label("%s (%i) %s" % (s,v,text))
            else:
                self.statuslabels[1].set_label("")

    def update_all(self):
        if self.window and self.window.is_visible():
            self.prepare_textbuffer()
            self.refresh_view()

    def refresh_view(self):
        if not self.plugin:
            return
        self.update_statusbar()
        self.redraw()
        self.adjust_scrollbars()
        self.update_font()

    def create_xor_gc(self):
        if self.pattern == -1:
            return
        if not self.window:
            return
        gc = self.window.new_gc()
        cm = gc.get_colormap()
        w,h = self.get_client_size()
        bbrush = cm.alloc_color('#ffffff')
        gc.set_function(gtk.gdk.XOR)
        gc.set_foreground(bbrush)
        gc.set_background(bbrush)
        self.xor_gc = gc

    def draw_cursor_xor(self):
        if self.pattern == -1:
            return
        if not self.window:
            return
        cr = self.window.cairo_create()
        cx, cy = self.pattern_to_pos(self.row, self.group, self.track, 
                                     self.index, self.subindex)        
        if (cx >= (PATLEFTMARGIN + 4)) and (cy >= self.top_margin):
            # Note that you have to add 0.5 to coordinates for cairo to properly
            # display lines of width 1.
            cr.rectangle(cx + 0.5, cy + 0.5, self.column_width, self.row_height)
            cr.set_source_rgba(1.0, 0.0, 0.0, 1.0)
            cr.set_line_width(1)
            cr.stroke_preserve()
            cr.set_source_rgba(1.0, 0.0, 0.0, 0.3)
            cr.fill()

    def draw_playpos_xor(self):
        if self.pattern == -1:
            return
        if not self.window:
            return
        drawable = self.window
        if not hasattr(self, "xor_gc"):
            self.create_xor_gc()
        gc = self.xor_gc
        # draw play cursor
        player = com.get('neil.core.player')
        current_position = self.playpos
        seq = player.get_current_sequencer()
        for i in range(seq.get_sequence_track_count()):
            track = seq.get_sequence(i)
            track_plugin = track.get_plugin()
            plugin = self.get_plugin()
            if plugin == track_plugin and self.pattern != -1:
                row_count = self.plugin.get_pattern_length(self.pattern)
                events = list(track.get_event_list())
                for i, pair in enumerate(events):
                    pos, value = pair
                    # make sure event is a pattern
                    if value >= 0x10:
                        pattern = value - 0x10
                    else:
                        continue
                    # handle overlapping of patterns
                    if i < len(events)-1 and events[i+1][0] <= current_position:
                        continue
                    if self.pattern == pattern and pos < current_position \
                    and current_position < pos + row_count:
                        y = self.top_margin + (current_position - pos - self.start_row) * self.row_height
                        w,h = self.get_client_size()
                        drawable.draw_rectangle(gc, True,0, y, w, 2)
                        return

    def get_plugin(self):
        """
        Returns the plugin of the pattern in the pattern view.

        @return: zzub plugin plugin.
        @rtype: zzub.Plugin
        """
        player = com.get('neil.core.player')
        if player.get_plugin_count() == 0:
            return
        sel = player.active_plugins
        return sel and sel[0] or None

    def get_datasource(self):
        """
        Returns the plugin and the current pattern in the pattern view

        @return: A tuple holding the plugin and the current pattern
        @rtype: (zzub.Plugin, zzub.Pattern)
        """
        plugin = self.get_plugin()
        if not plugin:
            return
        player = com.get('neil.core.player')
        for selplugin, i in player.active_patterns:
            if selplugin == plugin:
                return plugin, i
        return None

    def update_line(self, row):
        """
        Updates a line of the pattern.

        @param row: Line that will be updated.
        @type row: int
        """
        for g in range(3):
            if self.lines[g]:
                tc = self.group_track_count[g]
                for t in range(tc):
                    s = ' '.join([get_str_from_param(self.plugin.get_parameter(g, t, i),
                                                     self.plugin.get_pattern_value(self.pattern, g, t, i, row))
                                                    for i in xrange(self.parameter_count[g])])
                    values = [self.plugin.get_pattern_value(self.pattern, g, t, i, row) != self.plugin.get_parameter(g, t, i).get_value_none()
                                                            for i in range(self.parameter_count[g])]
                    try:
                        self.lines[g][t][row] = s
                    except IndexError:
                        pass

    # This does the same job as update_line, but if we need to
    # update a lot of data at once, it's faster to use update_col.
    def update_col(self, group, track):
        count = self.parameter_count[group]
        cols = [None] * count
        col_vals = [None] * count
        for i in range(count):
            param = self.plugin.get_parameter(group, 0, i)
            cols[i] = [get_str_from_param(param, self.plugin.get_pattern_value(self.pattern, group, track, i, row))
                    for row in range(self.row_count)]
            col_vals[i] = [self.plugin.get_pattern_value(self.pattern, group, track, i, row) != param.get_value_none()
                    for row in range(self.row_count)]
        for row in range(self.row_count):
            try:
                self.lines[group][track][row] =  ' '.join([cols[i][row] for i in range(count)])
            except IndexError:
                pass


    def prepare_textbuffer(self):
        """
        Initializes a buffer to handle the current pattern data.
        """
        self.lines = [None] * 3
        for group in range(3):
            if self.parameter_count[group] > 0:
                tc = self.group_track_count[group]
                self.lines[group] = [None] * tc
                for track in range(tc):
                    self.lines[group][track] = [None] * self.row_count
                    self.update_col(group, track)
            else:
                self.lines[group] = []

    def get_line_pattern(self):
        player = com.get('neil.core.player')
        master = player.get_plugin(0)
        tpb = master.get_parameter_value(1, 0, 2)
        return {
            16: [64, 32, 16, 8, 4],
            12: [48, 24, 12, 4],
            8: [32, 16, 8, 4],
            6: [24, 12, 6],
            3: [12, 4],
            }.get(tpb, [16, 4])

    def draw_pattern_background(self, ctx, layout):
        """ Draw the background, lines, borders and row numbers """
        w, h = self.get_client_size()
        gc = self.window.new_gc()
        cm = gc.get_colormap()
        cfg = config.get_config()
        drawable = self.window
        background = cm.alloc_color(cfg.get_color('PE BG'))
        pen = cm.alloc_color(cfg.get_color('PE Text'))
        gc.set_foreground(background)
        drawable.draw_rectangle(gc, True, 0, 0, w, self.row_height)
        drawable.draw_rectangle(gc, True, 0, 0, PATLEFTMARGIN, h)
        gc.set_foreground(pen)
        #drawable.draw_rectangle(gc, False, 0, 0, w - 1, h - 1)
        x, y = PATLEFTMARGIN, self.row_height
        row = self.start_row
        rows = self.row_count
        # Draw the row numbers
        num_rows = min(rows - row, (h - self.row_height) / self.row_height + 1)
        s = '\n'. join([str(i) for i in xrange(row, row + num_rows)])
        layout.set_text(s)
        px, py = layout.get_pixel_size()
        drawable.draw_layout(gc, x - 5 - px, y, layout)
        # Draw a black vertical separator line
        drawable.draw_line(gc, x, 0, x, h)
        y = self.row_height - 1
        # Draw a black horizontal separator line
        drawable.draw_line(gc, PATLEFTMARGIN, y, w, y)
        # The color of text as specified in config.py
        text_color = cm.alloc_color(cfg.get_color('PE Text'))
        gc.set_foreground(text_color)
        # Display track numbers in the middle of each track column at the to
        # For each existing track:
        for track in range(self.group_track_count[TRACK]):
            # Get x and y positions in the drawable that correspond
            # to a position that's designated for a particular event in
            # the pattern, in this case first row, track group,
            # current track being processed and the index of the first
            # parameter (0).
            x, y = self.pattern_to_pos(self.start_row, TRACK, track, 0)
            # Convert track number to a string that will be drawn with Pango.
            s = str(track)
            # Get the width of the track being processed.
            width = self.track_width[TRACK] * self.column_width
            # Prepare the text.
            layout.set_text(s)
            # Get the size of the string when it will be displayed in pixels.
            px, py = layout.get_pixel_size()
            # And draw it so that it falls in the middle of the track column.
            drawable.draw_layout(gc, x + width / 2 - px / 2, 
                                 self.row_height / 2 - (py / 2), layout)

    def draw_bar_marks(self, ctx):
        "Draw the horizontal bars every each fourth and eighth bar."
        w, h = self.get_client_size()
        gc = self.window.new_gc()
        cm = gc.get_colormap()
        drawable = self.window
        def draw_bar(row, group, track, color):
            """Draw a horizontal bar for a specified row in a 
            specified group/track."""
            x, y = self.pattern_to_pos(row, group, track, 0)
            width = (self.track_width[group] - 1) * self.column_width
            height = self.row_height
            gc.set_foreground(color)
            drawable.draw_rectangle(gc, True, x, y, width, height)
        darkest = cm.alloc_color('#b0b0b0')
        lighter = cm.alloc_color('#d0d0d0')
        lightest = cm.alloc_color('#f0f0f0')
        def get_color(row):
            "What color to paint the bar with?"
            if row % 16 == 0:
                return darkest
            if row % 8 == 0:
                return lighter
            elif row % 4 == 0:
                return lightest
            else:
                return None
        num_rows = min(self.row_count - self.start_row, 
                       (h - self.row_height) / self.row_height + 1)
        if self.lines and self.lines[CONN]:
            for track in range(self.group_track_count[CONN]):
                for row in range(self.start_row, num_rows + self.start_row):
                    color = get_color(row)
                    if color != None:
                        draw_bar(row, CONN, track, color)
        if self.lines and self.lines[GLOBAL]:
            for row in range(self.start_row, num_rows + self.start_row):
                color = get_color(row)
                if color != None:
                    draw_bar(row, GLOBAL, 0, color)
        if self.lines and self.lines[TRACK]:
            for track in range(self.group_track_count[TRACK]):
                for row in range(self.start_row, num_rows + self.start_row):
                    color = get_color(row)
                    if color != None:
                        draw_bar(row, TRACK, track, color)

    def draw_parameter_values(self, ctx, layout):
        """ Draw the parameter values for all tracks, columns and rows."""
        w, h = self.get_client_size()
        gc = self.window.new_gc()
        cm = gc.get_colormap()
        cfg = config.get_config()
        drawable = self.window
        def draw_parameters_range(row, num_rows, group, track=0):
            """Draw the parameter values for a range of rows"""
            x, y = self.pattern_to_pos(row, group, track, 0)
            s = '\n'.join([self.lines[group][track][i] 
                           for i in xrange(row, row + num_rows)])
            w = self.column_width * len(self.lines[group][track][row])
            layout.set_text(s)
            px, py = layout.get_pixel_size()
            drawable.draw_layout(gc, x, y, layout)
            return x + px
        # Draw the parameter values
        #i = self.start_row
        #y = self.row_height
        row = self.start_row
        rows = self.row_count
        # Number of rows is calculated to be either the first row displayed
        # subtracted from all rows, or the height of the screen divided
        # by row height, whichever is smaller.
        num_rows = min(rows - row, (h - self.row_height) / self.row_height + 1)
        # out_of_bounds will be set to true if we have gone over the right
        # edge of the screen, which signifies that we don't have to process
        # the columns that are further to the right.
        out_of_bounds = False
        if self.lines != None:
            # Draw connection parameters (volume, pan, etc)
            for t in range(self.group_track_count[CONN]):
                connectiontype = self.get_plugin().get_input_connection_type(t)
                if connectiontype == zzub.zzub_connection_type_audio:
                    extent = draw_parameters_range(row, num_rows, CONN, t)
                    out_of_bounds = extent > w
            # Draw global parameters.
            if not out_of_bounds:
                if self.lines[GLOBAL]:
                    extent = draw_parameters_range(row, num_rows, GLOBAL, 0)
                    out_of_bounds = extent > w
            # Draw track parameters.
            if not out_of_bounds:
                if self.lines[TRACK]:
                    for t in range(self.group_track_count[TRACK]):
                        extent = draw_parameters_range(row, num_rows, TRACK, t)
                        if extent > w:
                            break

    def draw_selection(self, ctx):
        """ Draw selection box."""
        drawable = self.window
        gc = drawable.new_gc()
        cr = self.window.cairo_create()
        def draw_box(x, y, width, height):
            cr.rectangle(x + 0.5, y + 0.5, width, height)
            cr.set_source_rgba(0.0, 1.0, 0.0, 1.0)
            cr.set_line_width(1)
            cr.stroke_preserve()
            cr.set_source_rgba(0.0, 1.0, 0.0, 0.3)
            cr.fill()
        if self.selection:
            x, y1 = self.pattern_to_pos(self.selection.begin,
                                        self.selection.group, 
                                        self.selection.track, 
                                        self.selection.index)
            x, y2 = self.pattern_to_pos(self.selection.end,
                                        self.selection.group, 
                                        self.selection.track, 
                                        self.selection.index)
            clip_y = (self.row_height + 
                      ((self.row_count - self.start_row) * self.row_height))
            y1 = max(self.row_height, y1)
            y2 = min(clip_y, y2)
            if y2 > y1:
                if self.selection.mode == SEL_COLUMN:
                    sel_g = self.selection.group
                    sel_i = self.selection.index
                    width = self.column_width
                    x2 = self.parameter_width[sel_g][sel_i] * width
                    draw_box(x, y1, x2, y2 - y1)
                    self.statuslabels[3].set_label("Sel Column")
                elif self.selection.mode == SEL_TRACK:
                    x2 = ((self.track_width[self.selection.group] - 1) * 
                          self.column_width)
                    draw_box(x, y1, x2, y2 - y1)
                    self.statuslabels[3].set_label("Sel Track")
                elif self.selection.mode == SEL_GROUP:
                    track_count = self.group_track_count[self.selection.group]
                    x2 = ((self.track_width[self.selection.group] * 
                           track_count - 1) * self.column_width)
                    draw_box(x, y1, x2, y2 - y1)
                    self.statuslabels[3].set_label("Sel Group")
                elif self.selection.mode == SEL_ALL:
                    x2 = 0
                    for group in range(3):
                        track_count = self.group_track_count[group]
                        if self.track_width[group]:
                            x2 += ((self.track_width[group] * track_count) *
                                   self.column_width)
                    x2 -= self.column_width
                    draw_box(x, y1, x2, y2 - y1)
                    self.statuslabels[3].set_label("Sel All")
            else:
                self.statuslabels[3].set_label("")

    def draw_background(self, ctx):
        w, h = self.get_client_size()
        drawable = self.window
        gc = drawable.new_gc()
        cm = gc.get_colormap()
        cfg = config.get_config()
        background = cm.alloc_color(cfg.get_color('PE BG'))
        gc.set_foreground(cm.alloc_color(background))
        drawable.draw_rectangle(gc, True, 0, 0, w, h)

    def draw(self, ctx):
        """
        Overriding a L{Canvas} method that paints onto an offscreen buffer.
        Draws the pattern view graphics.
        """
        layout = pango.Layout(self.get_pango_context())
        layout.set_font_description(self.fontdesc)
        layout.set_width(-1)
        self.draw_background(ctx)
        self.draw_bar_marks(ctx)
        self.draw_parameter_values(ctx, layout)
        self.draw_selection(ctx)
        self.draw_cursor_xor()
        self.draw_pattern_background(ctx, layout)
        self.draw_playpos_xor()

__all__ = [
    'PatternDialog',
    'show_pattern_dialog',
    'PatternToolBar',
    'PatternPanel',
    'get_str_from_param',
    'get_length_from_param',
    'get_subindexcount_from_param',
    'get_subindexoffsets_from_param',
    'PatternView',
]

__neil__ = dict(
    classes = [
        PatternDialog,
        PatternToolBar,
        PatternPanel,
        PatternView,
        ],
    )

